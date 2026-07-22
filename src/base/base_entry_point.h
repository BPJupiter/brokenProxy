#ifndef BASE_ENTRY_POINT_H
#define BASE_ENTRY_POINT_H

internal void main_thread_base_entry_point(int argc, char **argv);
internal void supplement_thread_base_entry_point(void (*entry_point)(void *params), void *params);
internal u64  update_tick_idx(void);
internal bool32 update(void);
internal void async_thread_entry_point(void *params);

#endif // BASE_ENTRY_POINT_H
