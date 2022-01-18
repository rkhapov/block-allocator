#include <algorithm>

#include "entry_point.h"

#include "gtest/gtest.h"

namespace coverage {
#if !GTEST_HAS_TYPED_TEST
#error "there is no typed googletests"
#endif

    template<class T>
    class BasicAllocatorTests : public testing::Test {
    protected:
        using AllocatorType = T;

        void *memory;
        size_t size;

        T GetAllocator() {
            return T(memory, size);
        }

        void SetUp() override {
            size = 1024;
            memory = malloc(size);
        }

        void TearDown() override {
            free(memory);
        }

        auto AllocateAsMuchAsPossible(T &a, size_t blockSize) {
            auto blocks = std::vector<void *>();

            while (true) {
                try {
                    blocks.push_back(a.Allocate(blockSize));
                }
                catch (NotEnoughMemory &) {
                    break;
                }
            }

            return blocks;
        }
    };

    using AllocatorImplementations = testing::Types<SequentialAllocator, BuddyAllocator>;

    TYPED_TEST_SUITE(BasicAllocatorTests, AllocatorImplementations,);

    TYPED_TEST(BasicAllocatorTests, AllocatorAreSmall) {
        struct NearToMaxStruct {
            void *memoryField;
            size_t sizeField;
        };

        ASSERT_LE(sizeof(typename TestFixture::AllocatorType), sizeof(NearToMaxStruct));
    }

    TYPED_TEST(BasicAllocatorTests, CtorOfAllocatorDoesntThrow) {
        ASSERT_NO_THROW(this->GetAllocator());
    }

    TYPED_TEST(BasicAllocatorTests, AllocateDoesntThrow) {
        auto a = this->GetAllocator();

        ASSERT_NO_THROW(a.Allocate(512));
    }

    TYPED_TEST(BasicAllocatorTests, DeallocateDoentThrow) {
        auto a = this->GetAllocator();

        auto *p = a.Allocate(512);

        ASSERT_NO_THROW(a.Deallocate(p));
    }

    TYPED_TEST(BasicAllocatorTests, AllocateTooMuchShouldThrow) {
        auto a = this->GetAllocator();

        ASSERT_THROW(a.Allocate(1025), NotEnoughMemory);
    }

    TYPED_TEST(BasicAllocatorTests, AllocateTooMuchButWithSeveralQueriesShouldThrow) {
        auto a = this->GetAllocator();

        a.Allocate(300);
        a.Allocate(300);
        a.Allocate(270);

        //there is < 100 bytes in allocator
        ASSERT_THROW(a.Allocate(this->size - 300 * 3 + 1), NotEnoughMemory);
    }

    TYPED_TEST(BasicAllocatorTests, AllocatedBlocksDoNotIntersects) {
        auto a = this->GetAllocator();
        const size_t blockSize = 100;

        auto blocks = this->AllocateAsMuchAsPossible(a, blockSize);

        std::sort(blocks.begin(), blocks.end());

        for (size_t i = 1; i < blocks.size(); i++) {
            ASSERT_LE(static_cast<uint8_t *>(blocks[i - 1]) + blockSize, blocks[i]);
        }
    }

    TYPED_TEST(BasicAllocatorTests, DeallocatedBlocksAreMergedLeft) {
        auto a = this->GetAllocator();

        auto *p1 = a.Allocate(300);

        a.Deallocate(p1);

        ASSERT_NO_THROW(a.Allocate(900));
    }

    TYPED_TEST(BasicAllocatorTests, DeallocatedBlocksAreMergedRight) {
        auto a = this->GetAllocator();

        [[maybe_unused]]auto *p1 = a.Allocate(300);
        auto *p2 = a.Allocate(300);

        a.Deallocate(p2);

        ASSERT_NO_THROW(a.Allocate(600));
    }

    TYPED_TEST(BasicAllocatorTests, DeallocatedBlocksAreMergedFromLeftAndRight) {
        auto a = this->GetAllocator();

        auto *p1 = a.Allocate(300);
        auto *p2 = a.Allocate(300);
        auto *p3 = a.Allocate(270);

        a.Deallocate(p3);
        a.Deallocate(p1);
        a.Deallocate(p2);

        ASSERT_NO_THROW(a.Allocate(900));
    }

    TYPED_TEST(BasicAllocatorTests, AllocatedBlocksAreFromGivenToAllocatorMemory) {
        auto a = this->GetAllocator();
        size_t blockSize = 100;

        auto blocks = this->AllocateAsMuchAsPossible(a, blockSize);

        for (auto blockPointer : blocks) {
            ASSERT_GE(blockPointer, this->memory);
            ASSERT_LE(static_cast<uint8_t *>(blockPointer) + blockSize,
                      static_cast<uint8_t *>(this->memory) + this->size);
        }
    }
}
