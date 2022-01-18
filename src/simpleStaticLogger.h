#pragma once

#include <ostream>

namespace {
    template<bool enabled>
    class SimpleStaticLogger;

    const char *EntrySeparator = "-----------------------------";

    template<>
    class SimpleStaticLogger<true> {
    public:
        constexpr explicit SimpleStaticLogger(const char *name, std::ostream &stream) : stream(stream), name(name) {
        }

        template<class... Args>
        void Log(Args &&... args) const {
            stream << "[" << name << "] : ";
            ((stream << " " << std::forward<Args>(args)), ...);
            stream << std::endl;
        }

        void Separator() const {
            stream << "[" << name << "] " << EntrySeparator << std::endl;
        }

    private:
        std::ostream &stream;
        const char *name;
    };


    template<>
    class SimpleStaticLogger<false> {
    public:
        constexpr explicit SimpleStaticLogger([[maybe_unused]] const char *_, [[maybe_unused]] std::ostream &__) {
        }

        template<class... Args>
        void Log([[maybe_unused]] Args &&... args) const {
        }

        void Separator() const {
        }
    };
}