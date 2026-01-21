# CMakeList.txt : CMake project for brokenProxy, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("brokenProxy")

set(CMAKE_C_STANDARD 90)

# Add source to this project's executable.
add_executable (${PROJECT_NAME}
	lib/cjson/cJSON.c
	lib/cjson/cJSON.h

	"src/Clay/clay.h"
	"src/Clay/c_mem_debug.c"
	"src/Clay/c_text.c"

	"src/Europa/europa.h"
	"src/Europa/e_threads.c"
	"src/Europa/e_time.c"

	"src/Styx/styx.h"
	"src/Styx/s_network.c"

	src/dnslookup.c
	src/dnslookup.h
	src/memory_usage.h
	src/proxy.c
	src/proxy.h
	src/termcolor.h
	
	src/traceroute.c
	src/traceroute.h
	src/shared_context.c
	src/shared_context.h
	src/main.c
 "src/Talos/talos.h" "src/Talos/t_error.c" "src/Europa/e_file.c")

target_include_directories(${PROJECT_NAME} PRIVATE src lib)

target_compile_definitions(${PROJECT_NAME} PRIVATE PING)

if(MSVC)
	target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
	target_link_libraries(${PROJECT_NAME} PRIVATE Ws2_32)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -pedantic -Wall -Wextra)
	target_link_libraries(${PROJECT_NAME} PRIVATE m)

    target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:-g> $<$<CONFIG:Debug>:-static>)
    target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:-static>)
endif()

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:NDEBUG>)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
