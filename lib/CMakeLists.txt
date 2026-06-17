if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-w)
endif()

add_library(cjson STATIC cjson/cJSON.c)
target_include_directories(cjson PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(vedis STATIC vedis/vedis.c)
target_include_directories(vedis PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(vedis PUBLIC VEDIS_ENABLE_THREADS)