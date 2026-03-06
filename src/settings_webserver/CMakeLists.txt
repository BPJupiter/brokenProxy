add_executable(settings_webserver
	"main.c"
 "response_codes.h" "webserver.c" "webserver.h")

target_include_directories(settings_webserver PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(settings_webserver PRIVATE bpj_core)