

#include <stdio.h>
#include <signal.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <future>
#include <typeinfo>

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <earcut.hpp>
namespace mapbox {
    namespace util {
        template <>
        struct nth<0, SDL_FPoint> {
            inline static auto get(const SDL_FPoint &t) { return t.x; };
        };
        template <>
        struct nth<1, SDL_FPoint> {
            inline static auto get(const SDL_FPoint &t) { return t.y; };
        };
    }
}
// This is so disgusting because C++ is terrible
// All this does is let earcut know what an FPoint is. Even though its just two floats
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "portable-file-dialogs.h"

#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#  include <process.h>
#else
#  include <unistd.h>
#endif

#include "Clay/clay.h"
#include "Europa/europa.h"
#include "broken_proxy.h"

#include "bplibxx/types.hpp"
#include "bplibxx/array.hpp"
#include "bplibxx/string.hpp"
#include "bplibxx/utils.hpp"

namespace GeoJson {
    enum class GeometryType {
        Unknown,
        Point,
        MultiPoint,
        LineString,
        MultiLineString,
        Polygon,
        MultiPolygon,
        GeometryCollection
    };

    GeometryType parse_geometry_type(const std::string &s)
    {
        if      (s == "Point")              return GeometryType::Point;
        else if (s == "MultiPoint")         return GeometryType::MultiPoint;
        else if (s == "LineString")         return GeometryType::LineString;
        else if (s == "MultiLineString")    return GeometryType::MultiLineString;
        else if (s == "Polygon")            return GeometryType::Polygon;
        else if (s == "MultiPolygon")       return GeometryType::MultiPolygon;
        else if (s == "GeometryCollection") return GeometryType::GeometryCollection;
        else                                return GeometryType::Unknown;
    }

    bool validate_json(const json &j)
    {
        if (!j.is_object()) return false;
        if (!j.contains("type")) return false;
        if (!j["type"].is_string()) return false;
        if (!(j["type"] == "FeatureCollection")) return false;
        if (!j.contains("features")) return false;
        if (!j["features"].is_array()) return false;
        return true;
    }
}

struct MainMenuWindowData
{
    bool show_proxy_settings = true;
    bool show_geojson_info = false;
};

struct BBox { f32 x_min, x_max, y_min, y_max; };

struct {
    SDL_FColor color = {0xCE / 255.0, 0xCE / 255.0, 0xCE / 255.0, 0xFF / 255.0};
    Array<Array<SDL_FPoint> *> polygons;
    Array<BBox> bboxes;
} World;

struct {
    Array<SDL_Color> colors;
    Array<Array<SDL_FPoint> *> lines;
    Array<BBox> bboxes;
    Array<SDL_FPoint> landings;
} Cables;

namespace JsonPaths {
    String cables = "assets/telegeography/cable-geo.json";
    String landing_points = "assets/telegeography/landing-point-geo.json";

    namespace WorldMap {
        String low = "assets/world-map-low.json";
        String medium = "assets/world-map-medium.json";
        String high = "assets/world-map-high.json";
    }
}

int width = 1280;
int height = 800;
constexpr f32 WORLD_SIZE = 1000.0f;
constexpr ImVec4 background_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

f32 lat_to_y_norm(f32 lat)
{
    lat = (std::max)(-85.051129f, (std::min)(lat, 85.051129f));
    f64 lat_rad = lat * (PI / 180.0);
    f64 y = std::log(std::tan((PI / 4.0) + (lat_rad / 2.0)));
    return (f32)(0.5 - (y / (2.0 * PI)));
}

SDL_FPoint project_mercator(f32 longitude, f32 latitude)
{
    f64 x = (longitude + 180.0) / 360.0;
    f32 y = lat_to_y_norm(latitude);
    
    return {
        (f32)(x * WORLD_SIZE),
        (f32)(y * WORLD_SIZE)
    };
}

static bool load_geojson_data(String filename)
{
    char *c_str_filename = (char *)to_c_string(filename);
    defer { free(c_str_filename); };
    std::ifstream f(c_str_filename);
    if (!f.is_open()) {
        fprintf(stderr, "Error: Could not open file %s\n", c_str_filename);
        fprintf(stderr, "Reason: %s\n", std::strerror(errno));
        return false;
    }
    const auto j = json::parse(f);
    if (!GeoJson::validate_json(j)) {
        return false;
    }
    for (const auto &feature: j["features"]) {
        const auto geometry = feature["geometry"];
        GeoJson::GeometryType type = GeoJson::parse_geometry_type(geometry["type"].get<std::string>());
        switch (type) {
        case GeoJson::GeometryType::MultiLineString: {
            const auto properties = feature["properties"];
            String hex = to_string((u8 *)properties["color"].get<std::string>().c_str());
            replace_chars(hex, "#", ' ');
            hex = trim_left(hex);
            String r, g, b;
            if (hex.count == 6) {
                r = slice(hex, 0, 2);
                g = slice(hex, 2, 2);
                b = slice(hex, 4, 2);
            } else {
                u8 *temp = to_c_string(hex);
                defer { free(temp); };
                fprintf(stderr, "hex length: %d; Invalid RGB parameter! :%s\n", hex.count, temp);
                r = to_string((u8 *)"FF");
                g = to_string((u8 *)"00");
                b = to_string((u8 *)"00");
            }
            char *r2 = (char *)to_c_string(r);
            char *g2 = (char *)to_c_string(g);
            char *b2 = (char *)to_c_string(b);
            defer { free(r2); };
            defer { free(g2); };
            defer { free(b2); };
            SDL_Color current_color = {
                (Uint8)(std::stoi(std::string(r2), nullptr, 16)),
                (Uint8)(std::stoi(std::string(g2), nullptr, 16)),
                (Uint8)(std::stoi(std::string(b2), nullptr, 16)),
                SDL_ALPHA_OPAQUE};
            // Make points
            const auto coordinates = geometry["coordinates"];
            for (u32 line = 0; line < coordinates.size(); line++) {
                Array<SDL_FPoint> *current_fat_line = new Array<SDL_FPoint>();
                auto pos = coordinates[line][0].get<std::array<float, 2>>();
                SDL_FPoint curr_point = project_mercator(pos[0], pos[1]);
                SDL_FPoint prev_point = curr_point;
                for (u32 i = 1; i < coordinates[line].size(); i++) {
                    pos = coordinates[line][i].get<std::array<float, 2>>();
                    curr_point = project_mercator(pos[0], pos[1]);
                    SDL_FPoint direction  = { curr_point.x - prev_point.x,
                                              curr_point.y - prev_point.y };
                    f32 magnitude = sqrtf(direction.x * direction.x + direction.y * direction.y);
                    magnitude *= 4;
                    SDL_FPoint prev_left  = { prev_point.x - (direction.y / magnitude), prev_point.y + (direction.x / magnitude) };
                    SDL_FPoint prev_right = { prev_point.x + (direction.y / magnitude), prev_point.y - (direction.x / magnitude) };
                    SDL_FPoint curr_left  = { curr_point.x - (direction.y / magnitude), curr_point.y + (direction.x / magnitude) };
                    SDL_FPoint curr_right = { curr_point.x + (direction.y / magnitude), curr_point.y - (direction.x / magnitude) };
                    current_fat_line->append(prev_right);
                    current_fat_line->append(prev_left);
                    current_fat_line->append(curr_left);
                    current_fat_line->append(prev_right);
                    current_fat_line->append(curr_left);
                    current_fat_line->append(curr_right);
                    prev_point = curr_point;
                }

                BBox bbox = { (*current_fat_line)[0].x, (*current_fat_line)[0].x, (*current_fat_line)[0].y, (*current_fat_line)[0].y };
                for (u32 i = 0; i < current_fat_line->items; i++) {
                    SDL_FPoint pt = (*current_fat_line)[i];
                    bbox.x_min = (std::min)(bbox.x_min, pt.x);
                    bbox.x_max = (std::max)(bbox.x_max, pt.x);
                    bbox.y_min = (std::min)(bbox.y_min, pt.y);
                    bbox.y_max = (std::max)(bbox.y_max, pt.y);
                }
                Cables.colors.append(current_color);
                Cables.lines.append(current_fat_line);
                Cables.bboxes.append(bbox);
            }
        } break;
        case GeoJson::GeometryType::Polygon: {
            const auto coordinates = geometry["coordinates"];
            // auto properties = feature["properties"];
            // std::string name = properties["name"].get<std::string>();
            std::vector<std::vector<SDL_FPoint>> earcut_input; // TODO: Allow earcut input to use Array<T>
            for (int i = 0; i < coordinates.size(); i++) {
                std::vector<SDL_FPoint> ring; // sigh
                for (int j = 0; j < coordinates[i].size(); j++) {
                    const auto &coord = coordinates[i][j];
                    const auto pos = coord.get<std::array<float, 2>>();
                    ring.push_back(project_mercator(pos[0], pos[1])); // sigh
                }
                earcut_input.push_back(ring); // sigh
            }
            std::vector<uint32_t> temp = mapbox::earcut<uint32_t>(earcut_input);
            Array<u32> indices;
            for (u32 i = 0; i < temp.size(); i++) {
                indices.append(temp[i]);
            }
            Array<SDL_FPoint> points;
            for (u32 i = 0; i < earcut_input.size(); i++) {
                const auto &inner_ring = earcut_input[i];
                for (u32 j = 0; j < inner_ring.size(); j++) {
                    points.append(inner_ring[j]);
                }
            }

            if (indices.items == 0) {
                continue;
            }

            Array<SDL_FPoint> *current_polygon = new Array<SDL_FPoint>();
            current_polygon->make_space(indices.items);
            for (u32 i = 0; i < indices.items; i++) {
                current_polygon->append(points[indices[i]]);
            }

            BBox bbox = { (*current_polygon)[0].x, (*current_polygon)[0].x, (*current_polygon)[0].y, (*current_polygon)[0].y };
            for (u32 i = 0; i < current_polygon->items; i++) {
                SDL_FPoint pt = (*current_polygon)[i];
                bbox.x_min = (std::min)(bbox.x_min, pt.x);
                bbox.x_max = (std::max)(bbox.x_max, pt.x);
                bbox.y_min = (std::min)(bbox.y_min, pt.y);
                bbox.y_max = (std::max)(bbox.y_max, pt.y);
            }
            World.polygons.append(current_polygon);
            World.bboxes.append(bbox);
        } break;
        case GeoJson::GeometryType::MultiPolygon: {
            const auto coordinates = geometry["coordinates"];
            //auto properties = feature["properties"];
            //std::string name = properties["name"].get<std::string>();
            for (int i = 0; i < coordinates.size(); i++) {
                std::vector<std::vector<SDL_FPoint>> earcut_input;
                for (int j = 0; j < coordinates[i].size(); j++) {
                    std::vector<SDL_FPoint> ring;
                    for (const auto &coord: coordinates[i][j]) {
                        const auto pos = coord.get<std::array<float, 2>>();
                        ring.push_back(project_mercator(pos[0], pos[1]));
                    }
                    earcut_input.push_back(ring);
                }
                std::vector<uint32_t> temp = mapbox::earcut<uint32_t>(earcut_input);
                Array<u32> indices;
                for (u32 j = 0; j < temp.size(); j++) {
                    indices.append(temp[j]);
                }
                Array<SDL_FPoint> points;
                for (u32 j = 0; j < earcut_input.size(); j++) {
                    const auto &inner_ring = earcut_input[j];
                    for (u32 k = 0; k < inner_ring.size(); k++) {
                        points.append(inner_ring[k]);
                    }
                }

                if (indices.items == 0) {
                    continue;
                }

                Array<SDL_FPoint> *current_polygon = new Array<SDL_FPoint>();
                current_polygon->make_space(indices.items);
                for (u32 j = 0; j < indices.items; j++) {
                    current_polygon->append(points[indices[j]]);
                }

                BBox bbox = { (*current_polygon)[0].x, (*current_polygon)[0].x, (*current_polygon)[0].y, (*current_polygon)[0].y };
                for (u32 i = 0; i < current_polygon->items; i++) {
                    SDL_FPoint pt = (*current_polygon)[i];
                    bbox.x_min = (std::min)(bbox.x_min, pt.x);
                    bbox.x_max = (std::max)(bbox.x_max, pt.x);
                    bbox.y_min = (std::min)(bbox.y_min, pt.y);
                    bbox.y_max = (std::max)(bbox.y_max, pt.y);
                }
                World.polygons.append(current_polygon);
                World.bboxes.append(bbox);
            }
        } break;
        default: {
            fprintf(stderr, "Unknown geometry type! %s\n", geometry["type"].get<std::string>().c_str());
            continue;
        } break;
        }
    }
    return true;
}

static void show_geojson_info()
{
    ImGui::Begin("Cable Info");
    // Probably do some sort of cable list here.
    ImGui::End();
}

static void HelpMarker(const char *desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static void show_proxy_settings(BProxyHandle *proxy)
{
    static int max_rtt_ms = proxy->settings.max_rtt_ns / 1000000;
    static bool do_ping = proxy->settings.do_ping;
    static bool do_traceroute = proxy->settings.do_traceroute;
    static char *dns_items[] = { "Custom Iterative (Default)",
                                 "Local System Default" };
    static int dns_selected_idx = proxy->settings.use_local_dns ? 1 : 0;
    ImGui::Begin("Settings");
    ImGui::InputInt("Max Latency (ms)", &max_rtt_ms);
    ImGui::Checkbox("Enable Ping", &do_ping);
    ImGui::SameLine(); HelpMarker("The Proxy will ping each IP address it touches. Results are compared against Max Latency. (TODO) Cache results.");
    ImGui::Checkbox("Enable Traceroute", &do_traceroute);
    ImGui::SameLine(); HelpMarker("The Proxy will perform a traceroute on each IP address it touches. Results are compared against Max Latency. (TODO) Implement a list of blocked IPs; Cache results.");
    const char *dns_preview_value = dns_items[dns_selected_idx];
    if (ImGui::BeginCombo("DNS Resolution", dns_preview_value, 0)) {
        for (int n = 0; n > IM_COUNTOF(dns_items); n++) {
            const bool is_selected = (dns_selected_idx == n);
            if (ImGui::Selectable(dns_items[n], is_selected))
                dns_selected_idx = n;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    float cable_button_hue = 200 / 360.f;
    float save_button_hue = 120 / 360.0f;
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(cable_button_hue, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(cable_button_hue, 0.8f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(cable_button_hue, 0.9f, 0.9f));
    if (ImGui::Button("All Cables ON")) {
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(cable_button_hue, 0.0f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(cable_button_hue, 0.0f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(cable_button_hue, 0.0f, 0.7f));
    if (ImGui::Button("All Cables OFF")) {
    }
    ImGui::PopStyleColor(3);
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(save_button_hue, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(save_button_hue, 0.8f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(save_button_hue, 0.9f, 0.9f));
    if (ImGui::Button("Apply Settings", ImVec2(-1.0f, 0.0f))) {
        proxy->settings.max_rtt_ns = max_rtt_ms * 1000000;
        proxy->settings.do_ping = do_ping;
        proxy->settings.do_traceroute = do_traceroute;
        proxy->settings.use_local_dns = (dns_selected_idx == 1);
        bp_save_settings_to_disk(proxy);
    }
    ImGui::PopStyleColor(3);
    ImGui::End();
}

void show_gui_windows(BProxyHandle *proxy)
{
    IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing Dear ImGui context. Refer to examples app!");
    IMGUI_CHECKVERSION();

    static MainMenuWindowData menu_data;

    if (menu_data.show_proxy_settings) { show_proxy_settings(proxy); }
    if (menu_data.show_geojson_info) { show_geojson_info(); }
}

bool is_mouse_over_triangle(int px, int py, int ax, int ay, int bx, int by, int cx, int cy)
{
    auto sign = [](int p1x, int p1y, int p2x, int p2y, int p3x, int p3y) {
        return (p1x - p3x) * (p2y - p3y) - (p2x - p3x) * (p1y - p3y);
    };

    bool b1 = sign(px, py, ax, ay, bx, by) < 0.0f;
    bool b2 = sign(px, py, bx, by, cx, cy) < 0.0f;
    bool b3 = sign(px, py, cx, cy, ax, ay) < 0.0f;
    return ((b1 == b2) && (b2 == b3));
}

int main(int,char**)
{
    { // Load json data in a render-ready format
        bool loaded_init_files = true;
        if (!load_geojson_data(JsonPaths::cables)) loaded_init_files = false;
        if (!load_geojson_data(JsonPaths::WorldMap::low)) loaded_init_files = false;
        if (!loaded_init_files) {
            fprintf(stderr, "Could not load cable data!\n");
            return 1;
        }
    }
    
    SDL_Window *window;
    SDL_Renderer *renderer;
    float main_scale;
    { // Window stuff
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
            fprintf(stderr, "Error: SDL_Init(): %s\n", SDL_GetError());
            return 1;
        }
        main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        SDL_WindowFlags window_flags = SDL_WINDOW_MAXIMIZED
            | SDL_WINDOW_RESIZABLE
            | SDL_WINDOW_HIDDEN
            | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        window = SDL_CreateWindow("The Broken Proxy", width, height, window_flags);
        if (window == nullptr) {
            fprintf(stderr, "Error: SDL_CreateWindow(): %s\n", SDL_GetError());
            return 1;
        }
        renderer = SDL_CreateRenderer(window, nullptr);
        SDL_SetRenderVSync(renderer, 1);
        if (renderer == nullptr) {
            SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
            return 1;
        }
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(window);
    }
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    { // Gui stuff
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGui::StyleColorsDark();

        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(main_scale);
        style.FontScaleDpi = main_scale;

        ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer3_Init(renderer);
    }

    BProxyHandle *proxy;
    int proxy_port = 13407;

    c_debug_memory_init(europa_mutex_lock, europa_mutex_unlock, europa_mutex_create());
    proxy = bp_init(proxy_port, "settings.xml");
    proxy->settings.use_local_dns = true;

    struct {
        float x = 0.0f;
        float y = -75.0f;
        float zoom = 1.75f;
        bool is_panning = false;
        Array<Array<SDL_Vertex> *> cable_render_buffer;
        Array<Array<SDL_Vertex> *> world_render_buffer;
    } camera;

    for (int i = 0; i < World.polygons.items; i++) {
        Array<SDL_Vertex> *current_polygon = new Array<SDL_Vertex>();
        for (int j = 0; j < World.polygons[i]->items; j++) {
            SDL_Vertex vertex = { (*World.polygons[i])[j], World.color, {0} };
            current_polygon->append(vertex);
        }
        camera.world_render_buffer.append(current_polygon);
    }
    for (int i = 0; i < Cables.lines.items; i++) {
        Array<SDL_Vertex> *current_line = new Array<SDL_Vertex>();
        for (int j = 0; j < Cables.lines[i]->items; j++) {
            SDL_FColor color = { Cables.colors[i].r / 255.0f,
                                 Cables.colors[i].g / 255.0f,
                                 Cables.colors[i].b / 255.0f,
                                 Cables.colors[i].a / 255.0f };
            SDL_Vertex vertex = { (*Cables.lines[i])[j], color, {0} };
            current_line->append(vertex);
        }
        camera.cable_render_buffer.append(current_line);
    }
    
    SDL_SetRenderVSync(renderer, 0);
    const Uint32 target_frametime_ms = 1000 / 60;
    bool window_occluded = false;
    bool show_demo_window = false;
    bool show_gui = true;
    bool done = false;
    while (!done) {
        Uint32 frame_start = SDL_GetTicks();
        int hovered_cable_idx = -1;
        { // Event handling
            float mouse_x, mouse_y;
            SDL_GetMouseState(&mouse_x, &mouse_y);
            for (int i = 0; i < Cables.lines.items; i++) {
                // Detect cable hover
            }
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (!io.WantCaptureMouse) {
                    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                        camera.is_panning = true;
                    }
                    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
                        camera.is_panning = false;
                    }
                    if (event.type == SDL_EVENT_MOUSE_MOTION && camera.is_panning) {
                        camera.x += event.motion.xrel / camera.zoom;
                        camera.y += event.motion.yrel / camera.zoom;
                    }
                    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                        float factor = 1.1f;
                        float prev_zoom = camera.zoom;
                        if      (event.wheel.y > 0) camera.zoom *= factor;
                        else if (event.wheel.y < 0) camera.zoom /= factor;
                        
                        camera.x = (mouse_x / camera.zoom) - (mouse_x / prev_zoom) + camera.x;
                        camera.y = (mouse_y / camera.zoom) - (mouse_y / prev_zoom) + camera.y;
                    }
                }
                ImGui_ImplSDL3_ProcessEvent(&event);
                if (event.type == SDL_EVENT_QUIT) done = true;
                if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) done = true;
                if (event.type == SDL_EVENT_WINDOW_OCCLUDED) window_occluded = true;
                if (event.type == SDL_EVENT_WINDOW_EXPOSED) window_occluded = false;
            }
        }
        { // Window housekeeping
            SDL_GetWindowSize(window, &width, &height);

            bool should_skip_rendering = false;
            if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) should_skip_rendering = true;
            if (window_occluded) should_skip_rendering = true;
            if (should_skip_rendering) {
                SDL_Delay(10);
                continue;
            }
        }

        { // GUI rendering
            ImGui_ImplSDLRenderer3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            {
                if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
                if (show_gui)         show_gui_windows(proxy);
                if (hovered_cable_idx != -1) ImGui::SetTooltip("Debug");
            }
        
            ImGui::Render();
        }

        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, background_color.x, background_color.y, background_color.z, background_color.w);
        SDL_RenderClear(renderer);

        { // Simulate program state
            bp_update(proxy);
        }

        { // Rendering
            float view_left = -camera.x;
            float view_top  = -camera.y;
            float view_right = (width / camera.zoom) - camera.x;
            float view_bottom = (height / camera.zoom) - camera.y;
            int min_copy = static_cast<int>(std::floor(view_left / WORLD_SIZE)) - 1;
            int max_copy = static_cast<int>(std::ceil(view_right / WORLD_SIZE)) + 1;
            { // World map
                for (size_t i = 0; i < World.polygons.items; i++) {
                    const BBox &bbox = World.bboxes[i];
                    bool skip_polygon = false;
                    if (bbox.y_max < view_top)    skip_polygon = true;
                    if (bbox.y_min > view_bottom) skip_polygon = true;
                    if (skip_polygon) continue;

                    
                    for (int copy = min_copy; copy <= max_copy; copy++) {
                        float x_offset = copy * WORLD_SIZE;
                        bool skip_copy = false;
                        if (bbox.x_max + x_offset < view_left)  skip_copy = true;
                        if (bbox.x_min + x_offset > view_right) skip_copy = true;
                        if (skip_copy) continue;
                        
                        for (size_t v = 0; v < World.polygons[i]->items; v++) {
                            (*camera.world_render_buffer[i])[v].position.x = ((*World.polygons[i])[v].x + x_offset + camera.x) * camera.zoom;
                            (*camera.world_render_buffer[i])[v].position.y = ((*World.polygons[i])[v].y            + camera.y) * camera.zoom;
                        }
                        SDL_RenderGeometry(renderer,
                                           nullptr,
                                           camera.world_render_buffer[i]->data,
                                           (int)(camera.world_render_buffer[i]->items),
                                           nullptr,
                                           0);
                    }
                }
            }
            { // Cables
                for (size_t i = 0; i < Cables.lines.items; i++) {
                    const BBox &bbox = Cables.bboxes[i];
                    bool skip_line = false;
                    if (Cables.colors[i].r == 0x93 && Cables.colors[i].g == 0x95 && Cables.colors[i].b == 0x97) skip_line = true;
                    if (Cables.lines[i]->items < 2) skip_line = true;
                    if (bbox.y_max < view_top)      skip_line = true;
                    if (bbox.y_min > view_bottom)   skip_line = true;
                    if (skip_line) continue;

                    for (int copy = min_copy; copy <= max_copy; copy++) {
                        float x_offset = copy * WORLD_SIZE;
                        bool skip_copy = false;
                        if (bbox.x_max + x_offset < view_left)  skip_copy = true;
                        if (bbox.x_min + x_offset > view_right) skip_copy = true;
                        if (skip_copy) continue;
                        
                        for (size_t v = 0; v < Cables.lines[i]->items; v++) {
                            (*camera.cable_render_buffer[i])[v].position.x = ((*Cables.lines[i])[v].x + x_offset + camera.x) * camera.zoom;
                            (*camera.cable_render_buffer[i])[v].position.y = ((*Cables.lines[i])[v].y            + camera.y) * camera.zoom;
                        }
                        SDL_RenderGeometry(renderer,
                                           nullptr,
                                           camera.cable_render_buffer[i]->data,
                                           (int)(camera.cable_render_buffer[i]->items),
                                           nullptr,
                                           0);
                    }
                }
            }
        }
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        Uint32 elapsed = SDL_GetTicks() - frame_start;
        if (elapsed < target_frametime_ms) SDL_Delay(target_frametime_ms - elapsed);
    }

    bp_destroy(proxy);
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    c_debug_mem_print(0);

    return 0;
}
