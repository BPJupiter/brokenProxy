
#ifndef BASE_DYNAMIC_LIBRARIES_H
#define BASE_DYNAMIC_LIBRARIES_H

typedef struct Library Library;
struct Library {
    u64 u64[1];
};

////////////////////////////////////////////////
// @per_os_impl Dynamically Loaded Libraries

internal Library library_open(String8 path);
internal void    library_close(Library library);
internal Void_Proc *library_load_proc(Library lib, String8 name);

#endif // BASE_DYNAMIC_LIBRARIES_H
