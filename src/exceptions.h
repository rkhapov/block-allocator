#pragma once

#include <stdexcept>

class NotEnoughMemory : public std::runtime_error {
public:
    NotEnoughMemory() : std::runtime_error("there is no enough memory to allocate") {
    }
};
