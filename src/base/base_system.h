#ifndef BASE_SYSTEM_H
#define BASE_SYSTEM_H

typedef struct System_Info System_Info;
struct System_Info {
    u32 logical_processor_count;
    u64 page_size;
    u64 large_page_size;
    u64 allocation_granularity;
    String8 machine_name;
};

// @per_os_impl System Info
internal System_Info *get_system_info(void);

#endif // BASE_SYSTEM_H
