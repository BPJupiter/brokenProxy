add_executable (proxy
    "context/context.c"
    "context/network_tools.c"
    "context/dns_resolution.c"

    "data/data.c"

    "verify/v_measurement.c"
    
    "proxy.c"
    "main.c"
 )

target_include_directories(proxy PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(proxy PRIVATE bpj_core)
