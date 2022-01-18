#pragma once

#include "exceptions.h"

class SequentialAllocator {
public:
    SequentialAllocator(void *memory, size_t sizeBytes);

    SequentialAllocator(const SequentialAllocator &other) = default;

    SequentialAllocator(SequentialAllocator &&other) = default;

    void *Allocate(size_t);

    void Deallocate(void *ptr);

private:
    void * const memory;
};