add_library(bpj_core STATIC
    "Clay/c_mem_debug.c"
    "Clay/c_text.c"
    "Clay/c_hashtable.c"
    "Europa/e_db_vedis.c"
    "Europa/e_dir.c"
    "Europa/e_threads.c"
    "Europa/e_time.c"
    "Europa/e_settings.c"
    "Styx/s_buffer.c"
    "Styx/s_network.c"
    "Styx/s_pack.c"
    "Styx/s_utils.c"
    "Talos/t_error.c"
 )

target_include_directories(bpj_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(bpj_core PUBLIC cjson vedis Threads::Threads)

target_compile_definitions(bpj_core PRIVATE $<$<CONFIG:Release>:NDEBUG>)

if (MSVC)
    target_compile_definitions(bpj_core PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(bpj_core PUBLIC Ws2_32)
else()
    target_compile_options(bpj_core PRIVATE -Wpedantic -Wall -Wextra)
    target_compile_options(bpj_core PRIVATE -Wno-comment)
    target_link_libraries(bpj_core PUBLIC m)

    target_compile_options(bpj_core PRIVATE $<$<CONFIG:Debug>:-g> $<$<CONFIG:Debug>:-static>)
    target_link_options(bpj_core PUBLIC $<$<CONFIG:Debug>:-static>)
endif()
