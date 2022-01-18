#pragma once

#include <cstddef>

constexpr auto WordSize = sizeof(size_t);

static_assert((WordSize & (WordSize - 1)) == 0, "size of word must be power of 2");

constexpr inline size_t AlignToWord(size_t n) {
    return (n + WordSize - 1) & ~(WordSize - 1);
}

constexpr inline size_t AlignToWordDown(size_t n) {
    return n & ~(WordSize - 1);
}
