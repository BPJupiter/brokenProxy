#ifndef BPLIBXX_ARRAY_H
#define BPLIBXX_ARRAY_H

#include "types.hpp"

template <typename T>
struct Array {
    T *data;
    u32 items;
    u32 capacity;

    Array() : Array(32) {}
    
    Array(u32 initial_capacity) {
        data = (T *)malloc(initial_capacity * sizeof(T));
        items = 0;
        capacity = initial_capacity;
    }

    ~Array() {
        free(data);
    }

    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;

    u32 append(T item) {
        if (items == capacity)
            make_space(capacity);
        data[items] = item;
        return items++;
    }

    u32 make_space(u32 capacity_to_add) {
        if (capacity_to_add == 0) capacity_to_add = 1;
        data = (T *)realloc(data, (capacity + capacity_to_add) * sizeof(T));
        capacity += capacity_to_add;
        return capacity;
    }

    u32 insert(u32 index, T item) {
        if (index > items)
            return (u32)-1;
        if (items == capacity)
            make_space(capacity);
        for (u32 i = items; i > index; i--) {
            data[i] = data[i - 1];
        }
        items++;
        data[index] = item;
        return index;
    }

    void clear() {
        items = 0;
    }

    void clear_and_zero() {
        Byte *bytes = (Byte *)data;
        for (u32 i = 0; i < items * sizeof(T); i++) {
            bytes[i] = 0;
        }
        items = 0;
    }

    void steal_from(Array *other) {
        if (this == other) return;

        free(data);

        data = other->data;
        items = other->items;
        capacity = other->capacity;

        other->data = nullptr;
        other->items = 0;
        other->capacity = 0;
    }

    T& operator[](u32 index) {
        assert(index < items);
        return data[index];
    }

    const T& operator[](u32 index) const {
        assert(index < items);
        return data[index];
    }
};


#endif // BPLIBXX_ARRAY_H
