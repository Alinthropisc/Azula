#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <array>

namespace azula::crypto
{

    /**
     * @brief SipHash-2-4 – fast keyed hash (Jean-Philippe Aumasson / DJB).
     *
     * Used in Azula for:
     *  - deduplication of results
     *  - fast bucket hashing
     *  - lightweight message authentication
     */
    class SipHash24
    {
        public:
            using Key = std::array<std::uint64_t, 2>;

            explicit SipHash24(Key key) noexcept : key_(key)
            {

            }

            /** Hash arbitrary data. */
            [[nodiscard]]
            std::uint64_t hash(std::span<const std::uint8_t> data) const noexcept;

            /** Convenience overload. */
            [[nodiscard]]
            std::uint64_t hash(const void* data, std::size_t len) const noexcept
            {
                return hash(std::span{static_cast<const std::uint8_t*>(data), len});
            }

            /** Built-in self-test against official vectors. */
            [[nodiscard]]
            static bool selftest() noexcept;

        private:
            Key key_;
    };

}











