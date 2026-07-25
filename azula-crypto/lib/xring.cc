#include <thread>
#include <vector>
#include <atomic>

#include "xring.hh"


using namespace azula::crypto;

template<> bool XRing<std::uint64_t, 16>::selftest() noexcept
{
    {
        XRing<std::uint64_t, 16> ring;
        for (std::uint64_t i = 1; i <= 10; ++i)
        {
            if (!ring.try_push(i))
            {
                return false;
            }
        }

        std::uint64_t sum = 0;
        while (auto v = ring.try_pop())
        {
            sum += *v;
        }

        if (sum != 55)
        {
            return false;
        }

    }

    {
        XRing<std::uint64_t, 16> ring;
        std::atomic<bool> done{false};
        std::atomic<std::uint64_t> total{0};

        std::thread producer([&] {
            for (std::uint64_t i = 1000; i >= 1; --i)
            {
                while (!ring.try_push(i))
                {
                    std::this_thread::yield();
                }
            }
            done = true;
        });

        std::thread consumer([&] {
            while (!done || !ring.empty())
            {
                if (auto v = ring.try_pop())
                {
                    total += *v;
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        });
        producer.join();
        consumer.join();

        // 1000 + 999 + ... + 1 = 500500
        if (total != 500500ULL)
        {
            return false;
        }
    }

    return true;
}







