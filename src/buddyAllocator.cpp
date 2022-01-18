#include <iostream>
#include <cstring>

#include "common.h"
#include "exceptions.h"
#include "simpleStaticLogger.h"
#include "buddyAllocator.h"

static constexpr SimpleStaticLogger<true> logger(__FILE__, std::cout);

static inline size_t GetMostSignificantBit(size_t n) {
    static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8);

    if constexpr (sizeof(size_t) == 4) {
        size_t msb;
        asm("bsrl %1, %0" : "=r"(msb) : "r"(n));

        return msb;
    }

    if constexpr (sizeof(size_t) == 8) {
        size_t msb;
        asm("bsrq %1, %0" : "=r"(msb) : "r"(n));

        return msb;
    }

    // unreachable
    return size_t(0);
}

static inline size_t GetOnlyMostSignificantBit(size_t n) {
    return (size_t(1) << GetMostSignificantBit(n));
}

static constexpr size_t minManagedMemorySize = 64;

static inline size_t GetNextBuddyPartSize(size_t n, size_t previousSize) {
    static_assert(minManagedMemorySize != 0);

    if (previousSize == 0) {
        return GetOnlyMostSignificantBit(n);
    }

    auto next = previousSize >> 1;
    while (next >= minManagedMemorySize) {
        if (next & n) {
            return next;
        }

        next >>= 1;
    }

    return size_t(0);
}

static void InitializeSubBuddyOn(void *memory, size_t size) {
    logger.Log("Initializing sub buddy allocator of size", size, "on", memory);
}


BuddyAllocator::BuddyAllocator(void *memory, size_t size) : memory(memory) {
    logger.Log("Initializing on", memory, "with size", size);

    std::memset(memory, 0, size);

    size_t sum = 0;
    size_t nextPart = 0;
    while ((nextPart = GetNextBuddyPartSize(size, nextPart)) != 0) {
        void *begin = static_cast<uint8_t *>(memory) + sum;

        InitializeSubBuddyOn(begin, nextPart);

        sum += nextPart;
    }

    logger.Log("Waste of memory will be", size - sum, "bytes", "which is", static_cast<double>(size - sum) / size);

    logger.Separator();
}

void *BuddyAllocator::Allocate(size_t size) {
    logger.Log(memory);
    throw NotEnoughMemory();
}

void BuddyAllocator::Deallocate(void *ptr) {
}
