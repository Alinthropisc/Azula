#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace azula::crypto
{

    class Base64
    {
        public:
            [[nodiscard]]
            static std::size_t encode(std::span<char> dst,std::span<const std::uint8_t> src) noexcept;

            [[nodiscard]]
            static std::size_t decode(std::span<std::uint8_t> dst,std::span<const char> src) noexcept;

            [[nodiscard]]
            static std::string encode(std::span<const std::uint8_t> src);

            [[nodiscard]]
            static std::vector<std::uint8_t> decode(std::string_view src);

            [[nodiscard]]
            static bool selftest() noexcept;
    };

}