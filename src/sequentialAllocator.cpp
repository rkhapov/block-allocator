#include <iostream>
#include <cstring>

#include "sequentialAllocator.h"
#include "simpleStaticLogger.h"
#include "common.h"

static constexpr SimpleStaticLogger<false> logger(__FILE__, std::cout);

struct BlockHeader final {
    size_t state;
    BlockHeader *next;

    // size must be aligned by AlignToWord, so lsb == 0
    void SetSizeBytes(size_t sizeInBytes) {
        state = (state & 1ul) | sizeInBytes;
    }

    [[nodiscard]] size_t GetSizeBytes() const {
        return state & ~1ul;
    }

    [[nodiscard]] bool IsUsed() const {
        return state & 1;
    }

    void SetUsed() {
        state |= 1;
    }

    void SetFree() {
        state &= ~1ul;
    }

    [[nodiscard]] bool CanFitOtherBlockAfterAllocation(size_t allocationSize) const {
        return GetSizeBytes() > allocationSize + sizeof(BlockHeader);
    }

    void SpawnNextOnOffset(size_t offset) {
        auto nextBlock = reinterpret_cast<BlockHeader *>(static_cast<uint8_t *>(GetDataPointer()) + offset);

        nextBlock->SetFree();
        nextBlock->SetSizeBytes(GetSizeBytes() - offset - sizeof(*this));
        nextBlock->next = next;

        SetSizeBytes(offset);
        next = nextBlock;
    }

    inline void *GetDataPointer() {
        return static_cast<void *>(this + 1);
    }

    BlockHeader *EatNextBlock() {
        SetSizeBytes(GetSizeBytes() + next->GetSizeBytes() + sizeof(*this));
        next = next->next;

        return this;
    }
};

std::ostream &operator<<(std::ostream &ostream, const BlockHeader &header) {
    auto current = &header;

    ostream << std::endl
            << "usage\tsize\tblock address\tnext"
            << std::endl;

    while (current != nullptr) {
        ostream << (current->IsUsed() ? "busy" : "free") << "\t"
                << current->GetSizeBytes() << "_B" << "\t"
                << current << "\t"
                << current->next
                << std::endl;

        current = current->next;
    }

    return ostream;
}

SequentialAllocator::SequentialAllocator(void *memory, size_t sizeBytes) : memory(memory) {
    logger.Log("Initializing sequential allocator on", memory, "of sizeBytes", sizeBytes);
    sizeBytes = AlignToWordDown(sizeBytes);
    logger.Log("Actual managed memory sizeBytes will be", sizeBytes);

    auto firstBlock = reinterpret_cast<BlockHeader *>(memory);

    firstBlock->next = nullptr;
    firstBlock->SetSizeBytes(sizeBytes - sizeof(BlockHeader));
    firstBlock->SetFree();

    logger.Log("Current blocks chain is:", *firstBlock);
    logger.Separator();
}

[[maybe_unused]] BlockHeader *FindBestFitBlock(BlockHeader *start, size_t sizeBytes) {
    auto current = start;
    BlockHeader *bestFit = nullptr;

    while (current != nullptr) {
        if (!current->IsUsed()
            && current->GetSizeBytes() >= sizeBytes
            && (bestFit == nullptr || bestFit->GetSizeBytes() > current->GetSizeBytes())) {
            bestFit = current;
        }

        current = current->next;
    }

    return bestFit;
}

[[maybe_unused]] BlockHeader *FindFirstFit(BlockHeader *start, size_t sizeBytes) {
    auto current = start;

    while (current != nullptr) {
        if (!current->IsUsed() && current->GetSizeBytes() >= sizeBytes) {
            break;
        }

        current = current->next;
    }

    return current;
}

BlockHeader *(*FindBlock)(BlockHeader *, size_t) = FindFirstFit;

BlockHeader *FindPrevious(BlockHeader *first, BlockHeader *target) {
    auto current = first;
    BlockHeader *previous = nullptr;

    while (current != target) {
        previous = current;
        current = current->next;
    }

    return previous;
}

BlockHeader *ConcatRightToLeft(BlockHeader *left, BlockHeader *right) {
    if (left == nullptr || left->IsUsed()) {
        return right;
    }

    left->next = right->next;
    left->SetSizeBytes(left->GetSizeBytes() + right->GetSizeBytes() + sizeof(BlockHeader));

    return left;
}

void *SequentialAllocator::Allocate(size_t sizeBytes) {
    logger.Log("Allocation query for", sizeBytes, "bytes");
    sizeBytes = AlignToWord(sizeBytes);
    logger.Log("Size was aligned to", sizeBytes);

    auto firstBlock = reinterpret_cast<BlockHeader *>(memory);
    auto blockToAllocate = FindBlock(firstBlock, sizeBytes);

    if (blockToAllocate == nullptr) {
        logger.Log("Cant find free block");
        throw NotEnoughMemory();
    }

    logger.Log("Found block to allocate", blockToAllocate);

    if (blockToAllocate->CanFitOtherBlockAfterAllocation(sizeBytes)) {
        logger.Log("Block can be split");
        blockToAllocate->SpawnNextOnOffset(sizeBytes);
        logger.Log("Spawned new block", blockToAllocate->next);
    }

    blockToAllocate->SetUsed();

    logger.Log("Blocks chain after allocation", *firstBlock);
    logger.Separator();

    // securiti :)
    std::memset(blockToAllocate->GetDataPointer(), 0, sizeBytes);

    return blockToAllocate->GetDataPointer();
}

void SequentialAllocator::Deallocate(void *ptr) {
    auto first = reinterpret_cast<BlockHeader *>(memory);
    auto block = reinterpret_cast<BlockHeader *>(ptr) - 1;

    logger.Log("Deallocating", ptr);
    logger.Log("Block address is", block);

    auto previous = FindPrevious(first, block);

    logger.Log("Previous block is", previous);

    block->SetFree();

    if (previous != nullptr && !previous->IsUsed()) {
        block = previous->EatNextBlock();
    }

    if (block->next != nullptr && !block->next->IsUsed()) {
        block->EatNextBlock();
    }

    logger.Log("Blocks chain after deallocate:", *first);
}
