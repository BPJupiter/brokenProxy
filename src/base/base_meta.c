// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Type Info Lookups

internal Member *
member_from_name(Type *type, String8 name)
{
  Member *member = &member_nil;
  if(type->members != 0 && name.size != 0)
  {
    for(u64 idx = 0; idx < type->count; idx += 1)
    {
      if(str8_match(type->members[idx].name, name, 0))
      {
        member = &type->members[idx];
        break;
      }
    }
  }
  return member;
}

////////////////////////////////
//~ rjf: Type Info * Instance Operations

internal void
typed_data_rebase_ptrs(Type *type, String8 data, void *base_ptr)
{
  Temp scratch = scratch_begin(0, 0);
  typedef struct Rebase_Type_Task Rebase_Type_Task;
  struct Rebase_Type_Task
  {
    Rebase_Type_Task *next;
    Type *type;
    U8 *ptr;
  };
  Rebase_Type_Task start_task = {0, type, data.str};
  Rebase_Type_Task *first_task = &start_task;
  Rebase_Type_Task *last_task = first_task;
  for(Rebase_Type_Task *t = first_task; t != 0; t = t->next)
  {
    switch(t->type->kind)
    {
      default:{}break;
      case TypeKind_Ptr:
      {
        *(u64 *)t->ptr = ((u64)(*(u8 **)t->ptr - (u8 *)base_ptr));
      }break;
      case TypeKind_Array:
      {
        for(u64 idx = 0; idx < t->type->count; idx += 1)
        {
          Rebase_Type_Task *task = push_array(scratch.arena, Rebase_Type_Task, 1);
          task->type = t->type->direct;
          task->ptr  = t->ptr + t->type->direct->size * idx;
          SLLQueuePush(first_task, last_task, task);
        }
      }break;
      case TypeKind_Struct:
      {
        for(u64 idx = 0; idx < t->type->count; idx += 1)
        {
          Member *member = &t->type->members[idx];
          Rebase_Type_Task *task = push_array(scratch.arena, Rebase_Type_Task, 1);
          task->type = member->type;
          task->ptr  = t->ptr + member->value;
          SLLQueuePush(first_task, last_task, task);
        }
      }break;
    }
  }
  scratch_end(scratch);
}

internal String8
serialized_from_typed_data(Arena *arena, Type *type, String8 data, Type_Serialize_Params *params)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8_List strings = {0};
  str8_serial_begin(scratch.arena, &strings);
  {
    typedef struct Serialize_Type_Task Serialize_Type_Task;
    struct Serialize_Type_Task
    {
      Serialize_Type_Task *next;
      Type *type;
      u64 count;
      u8 *src;
      Type *containing_type;
      u8 *containing_ptr;
      bool32 is_post_header;
    };
    Serialize_Type_Task start_task = {0, type, 1, data.str};
    Serialize_Type_Task *first_task = &start_task;
    Serialize_Type_Task *last_task = first_task;
    for(Serialize_Type_Task *t = first_task; t != 0; t = t->next)
    {
      switch(t->type->kind)
      {
        //- rjf: leaf -> just copy the data directly
        default:
        if(TypeKind_FirstLeaf <= t->type->kind && t->type->kind <= TypeKind_LastLeaf)
        {
          str8_serial_push_string(scratch.arena, &strings, str8(t->src, t->type->size*t->count));
        }break;
        
        //- rjf: pointers -> try to interpret/understand pointer & read/write, otherwise just write as plain data
        case TypeKind_Ptr:
        {
          // rjf: unpack info about this pointer
          Type_Serialize_Ptr_Ref_Info *ptr_ref_info = 0;
          for(u64 idx = 0; idx < params->ptr_ref_infos_count; idx += 1)
          {
            if(params->ptr_ref_infos[idx].type == t->type->direct)
            {
              ptr_ref_info = &params->ptr_ref_infos[idx];
              break;
            }
          }
          
          // rjf: indexification -> subtract base, divide direct size, write index
          if(ptr_ref_info != 0 && ptr_ref_info->indexify_base != 0)
          {
            u64 ptr_value = 0;
            MemoryCopy(&ptr_value, t->src, sizeof(ptr_value));
            u64 ptr_write_value = ((u64)((u8 *)ptr_value - (u8 *)ptr_ref_info->indexify_base)/t->type->direct->size);
            str8_serial_push_struct(scratch.arena, &strings, &ptr_write_value);
          }
          
          // rjf: offsetification -> subtract base, write offsets
          else if(ptr_ref_info != 0 && ptr_ref_info->offsetify_base != 0)
          {
            u64 ptr_value = 0;
            MemoryCopy(&ptr_value, t->src, sizeof(ptr_value));
            u64 ptr_write_value = (u64)((u8 *)ptr_value - (u8 *)ptr_ref_info->offsetify_base);
            str8_serial_push_struct(scratch.arena, &strings, &ptr_write_value);
          }
          
          // rjf: size-by-member (pre-header): still potentially dependent on other members which
          // delimit our size, so push a new post-header task for pointer.
          else if(t->type->count_delimiter_name.size != 0 && !t->is_post_header)
          {
            Serialize_Type_Task *task = push_array(scratch.arena, Serialize_Type_Task, 1);
            task->type  = t->type;
            task->count = t->count;
            task->src   = t->src;
            task->containing_type = t->containing_type;
            task->containing_ptr  = t->containing_ptr;
            task->is_post_header = 1;
            SLLQueuePush(first_task, last_task, task);
          }
          
          // rjf: size-by-member (post-header): all flat parts of containing struct have been
          // iterated, so now we can read the size, & descend to new task to read pointer
          // destination contents
          else if(t->type->count_delimiter_name.size != 0 && t->is_post_header)
          {
            // rjf: determine count of this pointer
            u64 count = 0;
            {
              Member *count_member = member_from_name(t->containing_type, t->type->count_delimiter_name);
              MemoryCopy(&count, t->containing_ptr + count_member->value, count_member->type->size);
            }
            
            // rjf: push task
            Serialize_Type_Task *task = push_array(scratch.arena, Serialize_Type_Task, 1);
            task->type                 = t->type->direct;
            task->count                = count;
            task->src                  = *(void **)t->src;
            task->containing_type      = t->containing_type;
            task->containing_ptr       = t->containing_ptr;
            SLLQueuePush(first_task, last_task, task);
          }
          
          // rjf: catch-all: write pointer value
          else
          {
            str8_serial_push_string(scratch.arena, &strings, str8(t->src, t->type->size*t->count));
          }
        }break;
        
        //- rjf: arrays -> descend to underlying type, + count
        case TypeKind_Array:
        {
          Serialize_Type_Task *task = push_array(scratch.arena, Serialize_Type_Task, 1);
          task->type  = t->type->direct;
          task->count = t->type->count;
          task->src   = t->src;
          task->containing_type = t->containing_type;
          task->containing_ptr  = t->containing_ptr;
          SLLQueuePush(first_task, last_task, task);
        }break;
        
        //- rjf: struct -> descend to members
        case TypeKind_Struct:
        {
          u64 off = 0;
          for(u64 idx = 0; idx < t->count; idx += 1)
          {
            for(u64 member_idx = 0; member_idx < t->type->count; member_idx += 1)
            {
              if(t->type->members[member_idx].flags & MemberFlag_DoNotSerialize)
              {
                continue;
              }
              Serialize_Type_Task *task = push_array(scratch.arena, Serialize_Type_Task, 1);
              task->type            = t->type->members[member_idx].type;
              task->count           = 1;
              task->src             = t->src + idx*t->type->size + t->type->members[member_idx].value;
              task->containing_type = t->type;
              task->containing_ptr  = t->src;
              SLLQueuePush(first_task, last_task, task);
            }
          }
        }break;
        
        //- rjf: enum -> descend to basic type interpretation
        case TypeKind_Enum:
        {
          Serialize_Type_Task *task = push_array(scratch.arena, Serialize_Type_Task, 1);
          task->type  = t->type->direct;
          task->count = t->count;
          task->src   = t->src;
          task->containing_type = t->containing_type;
          task->containing_ptr  = t->containing_ptr;
          SLLQueuePush(first_task, last_task, task);
        }break;
      }
    }
  }
  String8 result = str8_serial_end(arena, &strings);
  scratch_end(scratch);
  return result;
}

internal String8
deserialized_from_typed_data(Arena *arena, Type *type, String8 data, Type_Serialize_Params *params)
{
  String8 result = {0};
  result.size = type->size;
  result.str  = push_array(arena, u8, result.size);
  {
    Temp scratch = scratch_begin(&arena, 1);
    typedef struct Deserialize_Type_Task Deserialize_Type_Task;
    struct Deserialize_Type_Task
    {
      Deserialize_Type_Task *next;
      Type *type;
      u64 count;
      u8 *dst;
      Type *containing_type;
      u8 *containing_ptr;
      bool32 is_post_header;
    };
    u64 read_off = 0;
    Deserialize_Type_Task start_task = {0, type, 1, result.str};
    Deserialize_Type_Task *first_task = &start_task;
    Deserialize_Type_Task *last_task = first_task;
    for(Deserialize_Type_Task *t = first_task; t != 0; t = t->next)
    {
      u8 *t_src = data.str + read_off;
      switch(t->type->kind)
      {
        //- rjf: leaf -> copy the data directly
        default:
        if(TypeKind_FirstLeaf <= t->type->kind && t->type->kind <= TypeKind_LastLeaf)
        {
          MemoryCopy(t->dst, t_src, t->type->size*t->count);
          read_off += t->type->size*t->count;
        }break;
        
        //- rjf: pointers -> try to interpret/understand pointer & read/write, otherwise skip
        case TypeKind_Ptr:
        {
          // rjf: unpack info about this pointer
          Type_Serialize_Ptr_Ref_Info *ptr_ref_info = 0;
          for(u64 idx = 0; idx < params->ptr_ref_infos_count; idx += 1)
          {
            if(params->ptr_ref_infos[idx].type == t->type->direct)
            {
              ptr_ref_info = &params->ptr_ref_infos[idx];
              break;
            }
          }
          
          // rjf: indexification -> add base, multiply direct size
          if(ptr_ref_info != 0 && ptr_ref_info->indexify_base != 0)
          {
            u64 ptr_value = 0;
            MemoryCopy(&ptr_value, t_src, sizeof(ptr_value));
            u64 ptr_write_value = (ptr_value + (u64)ptr_ref_info->indexify_base) * t->type->direct->size;
            MemoryCopy(t->dst, &ptr_write_value, sizeof(ptr_write_value));
            read_off += sizeof(ptr_value);
          }
          
          // rjf: offsetification -> subtract base, write offsets
          else if(ptr_ref_info != 0 && ptr_ref_info->offsetify_base != 0)
          {
            u64 ptr_value = 0;
            MemoryCopy(&ptr_value, t_src, sizeof(ptr_value));
            u64 ptr_write_value = ptr_value + (u64)ptr_ref_info->offsetify_base;
            MemoryCopy(t->dst, &ptr_write_value, sizeof(ptr_write_value));
            read_off += sizeof(ptr_value);
          }
          
          // rjf: size-by-member (pre-header): still potentially dependent on other members which
          // delimit our size, so push a new post-header task for pointer.
          else if(t->type->count_delimiter_name.size != 0 && !t->is_post_header)
          {
            Deserialize_Type_Task *task = push_array(scratch.arena, Deserialize_Type_Task, 1);
            task->type  = t->type;
            task->count = t->count;
            task->dst   = t->dst;
            task->containing_type = t->containing_type;
            task->containing_ptr = t->containing_ptr;
            task->is_post_header = 1;
            SLLQueuePush(first_task, last_task, task);
          }
          
          // rjf: size-by-member (post-header): all flat parts of containing struct have been
          // iterated, so now we can read the size, & descend to new task to read pointer
          // destination contents
          else if(t->type->count_delimiter_name.size != 0 && t->is_post_header)
          {
            // rjf: determine count of this pointer
            u64 count = 0;
            {
              Member *count_member = member_from_name(t->containing_type, t->type->count_delimiter_name);
              MemoryCopy(&count, t->containing_ptr + count_member->value, count_member->type->size);
            }
            
            // rjf: allocate buffer for pointer destination; write address into pointer value slot
            u64 ptr_dest_buffer_size = (count+1)*t->type->direct->size;
            u8 *ptr_dest_buffer = push_array(arena, u8, ptr_dest_buffer_size);
            MemoryCopy(t->dst, &ptr_dest_buffer, sizeof(ptr_dest_buffer));
            
            // rjf: push task
            Deserialize_Type_Task *task = push_array(scratch.arena, Deserialize_Type_Task, 1);
            task->type                 = t->type->direct;
            task->count                = count;
            task->dst                  = ptr_dest_buffer;
            task->containing_type      = t->containing_type;
            task->containing_ptr       = t->containing_ptr;
            SLLQueuePush(first_task, last_task, task);
          }
          
          // rjf: catch-all: read pointer value
          else
          {
            MemoryCopy(t->dst, t_src, t->type->size*t->count);
            read_off += t->type->size*t->count;
          }
        }break;
        
        //- rjf: arrays -> descend to underlying type, + count
        case TypeKind_Array:
        {
          Deserialize_Type_Task *task = push_array(scratch.arena, Deserialize_Type_Task, 1);
          task->type  = t->type->direct;
          task->count = t->type->count;
          task->dst   = t->dst;
          task->containing_type = t->containing_type;
          task->containing_ptr  = t->containing_ptr;
          SLLQueuePush(first_task, last_task, task);
        }break;
        
        //- rjf: struct -> descend to members
        case TypeKind_Struct:
        {
          for(u64 idx = 0; idx < t->count; idx += 1)
          {
            for(u64 member_idx = 0; member_idx < t->type->count; member_idx += 1)
            {
              if(t->type->members[member_idx].flags & MemberFlag_DoNotSerialize)
              {
                continue;
              }
              Deserialize_Type_Task *task = push_array(scratch.arena, Deserialize_Type_Task, 1);
              task->type            = t->type->members[member_idx].type;
              task->count           = 1;
              task->dst             = t->dst + idx*t->type->size + t->type->members[member_idx].value;
              task->containing_type = t->type;
              task->containing_ptr  = t->dst;
              SLLQueuePush(first_task, last_task, task);
            }
          }
        }break;
        
        //- rjf: enum -> descend to basic type interpretation
        case TypeKind_Enum:
        {
          Deserialize_Type_Task *task = push_array(scratch.arena, Deserialize_Type_Task, 1);
          task->type  = t->type->direct;
          task->count = t->count;
          task->dst   = t->dst;
          task->containing_type = t->containing_type;
          task->containing_ptr  = t->containing_ptr;
          SLLQueuePush(first_task, last_task, task);
        }break;
      }
    }
    if(params->advance_out != 0)
    {
      params->advance_out[0] = read_off;
    }
    scratch_end(scratch);
  }
  return result;
}

internal String8
deep_copy_from_typed_data(Arena *arena, Type *type, String8 data, Type_Serialize_Params *params)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 data_srlz = serialized_from_typed_data(scratch.arena, type, data, params);
  String8 data_copy = deserialized_from_typed_data(arena, type, data_srlz, params);
  scratch_end(scratch);
  return data_copy;
}
