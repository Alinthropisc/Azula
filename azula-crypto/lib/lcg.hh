#pragma once

#include <cstdint>
#include <utility>

namespace azula::crypto
{

    class LCG
    {
        public:
            /**
             * @param range  Domain size (m).
             * @param seed   Optional preferred 'c'. 0 = auto.
             */
            explicit LCG(std::uint64_t range, std::uint64_t seed = 0);

            [[nodiscard]]
            std::uint64_t range() const noexcept
            {
                return range_;
            }

            [[nodiscard]]
            std::uint64_t a() const noexcept
            {
                return a_;
            }

            [[nodiscard]]
            std::uint64_t c()  const noexcept
            {
                return c_;
            }

            [[nodiscard]]
            std::uint64_t operator()(std::uint64_t index) const noexcept
            {
                return (index * a_ + c_) % range_;
            }

            [[nodiscard]]
            static bool selftest() noexcept;

        private:
            std::uint64_t range_;
            std::uint64_t a_;
            std::uint64_t c_;

            static std::pair<std::uint64_t, std::uint64_t>
            calculate_constants(std::uint64_t m, std::uint64_t preferred_c);
    };

}




