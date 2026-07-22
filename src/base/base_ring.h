// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_RING_H
#define BASE_RING_H

typedef struct Ring Ring;
struct Ring
{
  u8 *base;
  u64 size;
  u64 write_pos;
  u64 read_pos;
};

typedef struct Guarded_Ring Guarded_Ring;
struct Guarded_Ring
{
  Ring *ring;
  Mutex mutex;
  CondVar cv;
};

typedef struct Ring_Guard Ring_Guard;
struct Ring_Guard
{
  Guarded_Ring *r;
};

////////////////////////////////
//~ rjf: Ring Functions

internal Ring *make_ring(Arena *arena, u64 size);
internal bool32 ring_try_write(Ring *ring, u64 size, void *ptr);
internal bool32 ring_try_read(Ring *ring, u64 size, void *ptr);
#define ring_try_write_struct(ring, ptr) ring_try_write((ring), sizeof(*(ptr)), (ptr))
#define ring_try_read_struct(ring, ptr) ring_try_read((ring), sizeof(*(ptr)), (ptr))

////////////////////////////////
//~ rjf: Guarded Ring Functions

internal Guarded_Ring *guarded_ring_alloc(Arena *arena, u64 size);
internal void guarded_ring_release(Guarded_Ring *ring);
internal Ring_Guard guarded_ring_open(Guarded_Ring *ring);
internal void guarded_ring_close(Ring_Guard *guard);
internal bool32 guarded_ring_try_write(Ring_Guard *guard, u64 size, void *ptr);
internal bool32 guarded_ring_try_read(Ring_Guard *guard, u64 size, void *ptr);
#define guarded_ring_try_write_struct(ring, ptr) guarded_ring_try_write((ring), sizeof(*(ptr)), (ptr))
#define guarded_ring_try_read_struct(ring, ptr) guarded_ring_try_read((ring), sizeof(*(ptr)), (ptr))
internal bool32 guarded_ring_write_or_wait(Ring_Guard *guard, u64 size, void *ptr, u64 endt_us);
internal bool32 guarded_ring_read_or_wait(Ring_Guard *guard, u64 size, void *ptr, u64 endt_us);
#define guarded_ring_write_struct_or_wait(ring, ptr, endt_us) guarded_ring_write_or_wait((ring), sizeof(*(ptr)), (ptr), (endt_us))
#define guarded_ring_read_struct_or_wait(ring, ptr, endt_us) guarded_ring_read_or_wait((ring), sizeof(*(ptr)), (ptr), (endt_us))
#define guarded_ring_write_string_or_wait(ring, string, endt_us) guarded_ring_write_or_wait((ring), (string).size, (string).str, (endt_us))

#endif // BASE_RING_H
