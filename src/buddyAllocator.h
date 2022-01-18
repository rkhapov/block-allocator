#pragma once

#include <cstddef>

class BuddyAllocator {
public:
    BuddyAllocator(void *memory, size_t size);

    BuddyAllocator(const BuddyAllocator &other) = default;

    BuddyAllocator(BuddyAllocator &&other) = default;

    void *Allocate(size_t size);

    void Deallocate(void *ptr);

private:
    void *const memory;
};