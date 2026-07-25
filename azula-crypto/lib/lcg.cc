#include <cmath>
#include <vector>
#include <cstdint>

#include "lcg.hh"


namespace azula::crypto
{

    namespace
    {

        std::vector<std::uint64_t> factorize(std::uint64_t n)
        {
            std::vector<std::uint64_t> factors;

            if (n % 2 == 0)
            {
                factors.push_back(2);
                while (n % 2 == 0) n /= 2;
            }
            for (std::uint64_t p = 3; p * p <= n; p += 2)
            {
                if (n % p == 0)
                {
                    factors.push_back(p);
                    while (n % p == 0) n /= p;
                }
            }
            if (n > 1)
            {
                factors.push_back(n);
            }
            return factors;
        }

        bool has_common_factor(std::uint64_t c, const std::vector<std::uint64_t>& factors)
        {
            for (auto f : factors)
            {
                if (c % f == 0)
                {
                    return true;
                }
            }
            return false;
        }

    }
    std::pair<std::uint64_t, std::uint64_t>LCG::calculate_constants(std::uint64_t m, std::uint64_t preferred_c)
    {
        auto factors = factorize(m);
        std::uint64_t a = 1;

        if (factors.size() == 1 && factors[0] == m)
        {
            a = 5;
        }
        else
        {
            for (auto f : factors)
            {
                a *= f;
            }
            if (m % 4 == 0)
            {
                a *= 2;
            }
        }
        a += 1;
        std::uint64_t c = preferred_c ? preferred_c : 2531011ULL;

        while (has_common_factor(c, factors))
        {
            ++c;
        }
        return {a, c};
    }

    LCG::LCG(std::uint64_t range, std::uint64_t seed): range_(range == 0 ? 1 : range)
    {
        auto [a, c] = calculate_constants(range_, seed);
        a_ = a;
        c_ = c;
    }

    bool LCG::selftest() noexcept
    {
        for (int t = 0; t < 5; ++t)
        {
            std::uint64_t m = 3015ULL * 3 + 10 + t;
            LCG lcg(m);
            std::vector<std::uint8_t> seen(static_cast<std::size_t>(m), 0);

            for (std::uint64_t i = 0; i < m; ++i)
            {
                auto x = lcg(i);

                if (x >= m || seen[x])
                {
                    return false;
                }
                seen[x] = 1;
            }
        }
        return true;
    }

}






