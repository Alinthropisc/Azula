#pragma once

#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <string_view>
#include <concepts>
#include <ctime>


namespace azula::crypto
{
    template <typename T> concept Permutation = requires(const T& p, std::uint64_t idx)
    {
        {
            p.range()
        }        -> std::convertible_to<std::uint64_t>;

        {
            p.shuffle(idx)
        }   -> std::same_as<std::uint64_t>;

        {
            p.unshuffle(idx)
        } -> std::same_as<std::uint64_t>;
    };


    class BlackRock2
    {
        public:
            /**
             * @brief Factory method (preferred).
             * @param range  Size of the domain [0, range). Must be > 0.
             * @param seed   64-bit key / seed.
             * @param rounds Number of Feistel rounds (recommended 4–8, default 6).
             * @throws std::invalid_argument if range == 0.
             */
            [[nodiscard]]
            static BlackRock2 create(std::uint64_t range,std::uint64_t seed,unsigned rounds = 6);

            /**
             * @brief Construct from already computed parameters (advanced / testing).
             */
            BlackRock2(std::uint64_t range,std::uint64_t a,std::uint64_t b,std::uint64_t seed,unsigned rounds) noexcept;

            // Rule of zero – defaulted special members are fine (trivially copyable).

            [[nodiscard]]
            std::uint64_t range()  const noexcept
            {
                return range_;
            }

            [[nodiscard]]
            std::uint64_t seed()   const noexcept
            {
                return seed_;
            }

            [[nodiscard]]
            unsigned      rounds() const noexcept
            {
                return rounds_;
            }

            /**
             * @brief Encrypt / shuffle an index into the same domain.
             * @param index Must be in [0, range()). Behaviour is undefined otherwise.
             * @return Unique value also in [0, range()).
             */
            [[nodiscard]]
            std::uint64_t shuffle(std::uint64_t index) const noexcept;

            /**
             * @brief Inverse of shuffle.
             */
            [[nodiscard]]
            std::uint64_t unshuffle(std::uint64_t value) const noexcept;

            /**
             * @brief Built-in regression self-test (same logic as masscan).
             * @return true on success.
             */
            [[nodiscard]]
            static bool selftest();

        private:
            std::uint64_t range_;
            std::uint64_t a_;
            std::uint64_t b_;
            std::uint64_t a_bits_;
            std::uint64_t a_mask_;
            std::uint64_t b_bits_;
            std::uint64_t b_mask_;
            std::uint64_t seed_;
            unsigned      rounds_;

            // Internal Feistel helpers
            [[nodiscard]]
            static std::uint64_t next_power_of_two(std::uint64_t n) noexcept;
            [[nodiscard]]
            static std::uint64_t bit_count(std::uint64_t n) noexcept;

            [[nodiscard]]
            std::uint64_t round(std::uint64_t r, std::uint64_t R) const noexcept;

            [[nodiscard]]
            std::uint64_t encrypt(std::uint64_t m) const noexcept;

            [[nodiscard]]
            std::uint64_t decrypt(std::uint64_t m) const noexcept;
    };

    static_assert(Permutation<BlackRock2>);

}











































