// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Ring Functions

internal Ring *
make_ring(Arena *arena, u64 size)
{
  Ring *ring = push_array(arena, Ring, 1);
  ring->size = size;
  ring->base = push_array(arena, u8, ring->size);
  return ring;
}

internal bool32
ring_try_write(Ring *ring, u64 size, void *ptr)
{
  u64 bytes_unconsumed = (ring->write_pos - ring->read_pos);
  u64 bytes_available = ring->size - bytes_unconsumed;
  bool32 result = 0;
  if(bytes_available >= size)
  {
    result = 1;
    ring->write_pos += wrapped_write(ring->base, ring->size, ring->write_pos, ptr, size);
  }
  return result;
}

internal bool32
ring_try_read(Ring *ring, u64 size, void *ptr)
{
  u64 bytes_unconsumed = (ring->write_pos - ring->read_pos);
  bool32 result = 0;
  if(bytes_unconsumed >= size)
  {
    result = 1;
    ring->read_pos += wrapped_read(ring->base, ring->size, ring->read_pos, ptr, size);
  }
  return result;
}

////////////////////////////////
//~ rjf: Guarded Ring Functions

internal Guarded_Ring *
guarded_ring_alloc(Arena *arena, u64 size)
{
  Guarded_Ring *gr = push_array(arena, Guarded_Ring, 1);
  gr->ring = make_ring(arena, size);
  gr->mutex = mutex_alloc();
  gr->cv = cond_var_alloc();
  return gr;
}

internal void
guarded_ring_release(Guarded_Ring *ring)
{
  mutex_release(ring->mutex);
  cond_var_release(ring->cv);
}

internal Ring_Guard
guarded_ring_open(Guarded_Ring *ring)
{
  Ring_Guard guard = {ring};
  mutex_take(ring->mutex);
  return guard;
}

internal void
guarded_ring_close(Ring_Guard *guard)
{
  mutex_drop(guard->r->mutex);
}

internal bool32
guarded_ring_try_write(Ring_Guard *guard, u64 size, void *ptr)
{
  bool32 result = ring_try_write(guard->r->ring, size, ptr);
  if(result)
  {
    cond_var_broadcast(guard->r->cv);
  }
  return result;
}

internal bool32
guarded_ring_try_read(Ring_Guard *guard, u64 size, void *ptr)
{
  bool32 result = ring_try_read(guard->r->ring, size, ptr);
  if(result)
  {
    cond_var_broadcast(guard->r->cv);
  }
  return result;
}

internal bool32
guarded_ring_write_or_wait(Ring_Guard *guard, u64 size, void *ptr, u64 endt_us)
{
  bool32 write_good = 0;
  for(;!write_good;)
  {
    write_good = guarded_ring_try_write(guard, size, ptr);
    if(now_time_us() >= endt_us)
    {
      break;
    }
    if(!write_good)
    {
      cond_var_wait(guard->r->cv, guard->r->mutex, endt_us);
    }
  }
  return write_good;
}

internal bool32
guarded_ring_read_or_wait(Ring_Guard *guard, u64 size, void *ptr, u64 endt_us)
{
  bool32 read_good = 0;
  for(;!read_good;)
  {
    read_good = guarded_ring_try_read(guard, size, ptr);
    if(now_time_us() >= endt_us)
    {
      break;
    }
    if(!read_good)
    {
      cond_var_wait(guard->r->cv, guard->r->mutex, endt_us);
    }
  }
  return read_good;
}
