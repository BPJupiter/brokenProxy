

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
#include "bproxy.h"

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
    bool show_main_menu_bar = false;
    bool show_geojson_info = false;
};

struct BBox { float x_min, x_max, y_min, y_max; };

struct {
    SDL_FColor color = {0xCE / 255.0, 0xCE / 255.0, 0xCE / 255.0, 0xFF / 255.0};
    std::vector<SDL_FPoint> polygons;
    std::vector<uint32_t> polygon_indices;
} World;

struct {
    std::vector<SDL_Color> colors;
    std::vector<std::vector<SDL_FPoint>> lines;
    std::vector<BBox> bboxes;
    std::vector<SDL_FPoint> landings;
} Cables;

namespace JsonPaths {
    const std::string cables("assets/telegeography/cable-geo.json");
    const std::string landing_points("assets/telegeography/landing-point-geo.json");

    namespace WorldMap {
        const std::string low("assets/world-map-low.json");
        const std::string medium("assets/world-map-medium.json");
        const std::string high("assets/world-map-high.json");
    }
}

int width = 1280;
int height = 800;
constexpr float WORLD_SIZE = 1000.0f;
constexpr ImVec4 background_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

float lat_to_y_norm(float lat)
{
    lat = (std::max)(-85.051129f, (std::min)(lat, 85.051129f));
    double lat_rad = lat * (PI / 180.0);
    double y = std::log(std::tan((PI / 4.0) + (lat_rad / 2.0)));
    return static_cast<float>(0.5 - (y / (2.0 * PI)));
}

SDL_FPoint project_mercator(float longitude, float latitude)
{
    double x = (longitude + 180.0) / 360.0;
    float y = lat_to_y_norm(latitude);
    
    return {
        static_cast<float>(x * WORLD_SIZE),
        static_cast<float>(y * WORLD_SIZE)
    };
}

static bool load_geojson_data(const std::string &filename)
{
    std::ifstream f(filename);
    if (!f.is_open()) {
        fprintf(stderr, "Error: Could not open file %s\n", filename.c_str());
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
            std::string hex = properties["color"].get<std::string>();
            hex = hex.substr(1); // Remove leading '#'
            std::string r, g, b;
            if (hex.length() == 6) {
                r = hex.substr(0, 2);
                g = hex.substr(2, 2);
                b = hex.substr(4, 2);
            } else {
                fprintf(stderr, "Invalid RGB parameter! %s\n", hex.c_str());
                r = "FF";
                g = "00";
                b = "00";
            }
            SDL_Color current_color = {
                static_cast<Uint8>(std::stoi(r, nullptr, 16)),
                static_cast<Uint8>(std::stoi(g, nullptr, 16)),
                static_cast<Uint8>(std::stoi(b, nullptr, 16)),
                SDL_ALPHA_OPAQUE};
            // Make points
            const auto coordinates = geometry["coordinates"];
            for (int line = 0; line < coordinates.size(); line++) {
                std::vector<SDL_FPoint> current_line;
                for (const auto &coord: coordinates[line]) {
                    const auto pos = coord.get<std::array<float, 2>>();
                    current_line.push_back(project_mercator(pos[0], pos[1]));
                }

                BBox bbox = { current_line[0].x, current_line[0].x, current_line[0].y, current_line[0].y };
                for (const auto &pt: current_line) {
                    bbox.x_min = (std::min)(bbox.x_min, pt.x);
                    bbox.x_max = (std::max)(bbox.x_max, pt.x);
                    bbox.y_min = (std::min)(bbox.y_min, pt.y);
                    bbox.y_max = (std::max)(bbox.y_max, pt.y);
                }
                Cables.colors.push_back(current_color);
                Cables.lines.push_back(current_line);
                Cables.bboxes.push_back(bbox);
            }
        } break;
        case GeoJson::GeometryType::Polygon: {
            const auto coordinates = geometry["coordinates"];
            // auto properties = feature["properties"];
            // std::string name = properties["name"].get<std::string>();
            std::vector<std::vector<SDL_FPoint>> earcut_input;
            for (int i = 0; i < coordinates.size(); i++) {
                std::vector<SDL_FPoint> ring;
                for (const auto &coord: coordinates[i]) {
                    const auto pos = coord.get<std::array<float, 2>>();
                    ring.push_back(project_mercator(pos[0], pos[1]));
                }
                earcut_input.push_back(ring);
            }
            std::vector<uint32_t> indices = mapbox::earcut<uint32_t>(earcut_input);
            std::vector<SDL_FPoint> points;
            for (const auto &ring: earcut_input) {
                points.insert(points.end(), ring.begin(), ring.end());
            }

            std::vector<SDL_FPoint> current_polygon;
            current_polygon.reserve(indices.size());
            for (uint32_t idx: indices) {
                current_polygon.push_back(points[idx]);
            }
            
            for (const auto &point: current_polygon) {
                World.polygons.push_back(point);
            }
        } break;
        case GeoJson::GeometryType::MultiPolygon: {
            const auto coordinates = geometry["coordinates"];
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
                std::vector<uint32_t> indices = mapbox::earcut<uint32_t>(earcut_input);
                std::vector<SDL_FPoint> points;
                for (const auto &ring: earcut_input) {
                    points.insert(points.end(), ring.begin(), ring.end());
                }

                std::vector<SDL_FPoint> current_polygon;
                current_polygon.reserve(indices.size());
                for (uint32_t idx: indices) {
                    current_polygon.push_back(points[idx]);
                }

                for (const auto &point: current_polygon) {
                    World.polygons.push_back(point);
                }
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

void show_main_menu_window(bool *p_open)
{
    IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing Dear ImGui context. Refer to examples app!");
    IMGUI_CHECKVERSION();

    static MainMenuWindowData menu_data;

    if (menu_data.show_main_menu_bar) { }
    if (menu_data.show_geojson_info) { show_geojson_info(); }
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
        SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE
            | SDL_WINDOW_HIDDEN
            | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        window = SDL_CreateWindow("Dear ImGUI SDL3+SDL_Renderer example", width, height, window_flags);
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
    europa_settings_load("settings.xml");
    europa_setting_boolean_set("USE_LOCAL_DNS", TRUE, NULL);
    europa_settings_save("settings.xml");
    bp_reload_settings(proxy);

    struct {
        float x = 0.0f;
        float y = 0.0f;
        float zoom = 1.0f;
        bool is_panning = false;
        std::vector<SDL_FPoint> cable_render_buffer;
        std::vector<SDL_Vertex> world_render_buffer;
    } camera;

    for (int i = 0; i < World.polygons.size(); i++) {
        SDL_Vertex vertex = { World.polygons[i], World.color, {0} };
        camera.world_render_buffer.push_back(vertex);
    }
    
    SDL_SetRenderVSync(renderer, 0);
    const Uint32 target_frametime_ms = 1000 / 60;
    bool window_occluded = false;
    bool show_demo_window = false;
    bool show_main_window = true;
    bool done = false;
    while (!done) {
        Uint32 frame_start = SDL_GetTicks();

        { // Event handling
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
                        if (event.wheel.y > 0) camera.zoom *= factor;
                        else if (event.wheel.y < 0) camera.zoom /= factor;
                        float mouse_x, mouse_y;
                        SDL_GetMouseState(&mouse_x, &mouse_y);
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
                if (show_main_window) show_main_menu_window(&show_main_window);
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
                for (int copy = min_copy; copy <= max_copy; copy++) {
                    float x_offset = copy * WORLD_SIZE;
                    for (size_t i = 0; i < World.polygons.size(); i++) {
                        camera.world_render_buffer[i].position.x =
                            (World.polygons[i].x + x_offset + camera.x) * camera.zoom;
                        camera.world_render_buffer[i].position.y =
                            (World.polygons[i].y + camera.y) * camera.zoom;
                    }
                    SDL_RenderGeometry(renderer, nullptr, camera.world_render_buffer.data(), static_cast<int>(camera.world_render_buffer.size()), nullptr, 0);
                }
            }
            { // Cables
                for (size_t i = 0; i < Cables.lines.size(); i++) {
                    const BBox &bbox = Cables.bboxes[i];
                    bool skip_line = false;
                    if (Cables.colors[i].r == 0x93 && Cables.colors[i].g == 0x95 && Cables.colors[i].b == 0x97) skip_line = true;
                    if (Cables.lines[i].size() < 2) skip_line = true;
                    if (bbox.y_max < view_top) skip_line = true;
                    if (bbox.y_min > view_bottom) skip_line = true;
                    if (skip_line) continue;

                    SDL_SetRenderDrawColor(renderer, Cables.colors[i].r, Cables.colors[i].g, Cables.colors[i].b, Cables.colors[i].a);
                    camera.cable_render_buffer.resize(Cables.lines[i].size());
                    for (int copy = min_copy; copy <= max_copy; copy++) {
                        float x_offset = copy * WORLD_SIZE;
                        bool skip_copy = false;
                        if (bbox.x_max + x_offset < view_left) skip_copy = true;
                        if (bbox.x_min + x_offset > view_right) skip_copy = true;
                        if (skip_copy) continue;
                        
                        for (size_t p = 0; p < Cables.lines[i].size(); p++) {
                            camera.cable_render_buffer[p].x = (Cables.lines[i][p].x + x_offset + camera.x) * camera.zoom;
                            camera.cable_render_buffer[p].y = (Cables.lines[i][p].y + camera.y) * camera.zoom;
                        }
                        SDL_RenderLines(renderer, camera.cable_render_buffer.data(), static_cast<int>(Cables.lines[i].size()));
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
