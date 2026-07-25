#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <array>
#include <type_traits>

namespace azula::crypto
{

    /**
     * @brief Fixed-size lock-free ring buffer (SPSC / simple MPMC-friendly).
     *
     * Port of masscan xring with modern C++23 semantics.
     * Stores non-zero values of type T (default uint64_t).
     * Zero is reserved as "empty" marker (same as original).
     *
     * Power-of-two capacity required.
     */
    template <typename T = std::uint64_t, std::size_t Capacity = 16> requires (std::is_trivially_copyable_v<T> && (Capacity & (Capacity - 1)) == 0 && Capacity >= 2)

    class XRing
    {
        public:
            static constexpr std::size_t capacity = Capacity;
            static constexpr std::size_t mask     = Capacity - 1;

            XRing() noexcept
            {
                for (auto& slot : ring_)
                {
                    slot.store(T{}, std::memory_order_relaxed);
                }
                head_.store(0, std::memory_order_relaxed);
                tail_.store(0, std::memory_order_relaxed);
            }
            // Non-copyable, non-movable (contains atomics)
            XRing(const XRing&) = delete;
            XRing& operator=(const XRing&) = delete;

            /**
             * @brief Try to push a value.
             * @return true on success, false if full or value is "empty" marker.
             */
            [[nodiscard]]
            bool try_push(T value) noexcept
            {
                if (value == T{})          // zero is reserved
                {
                    return false;
                }
                const auto head = head_.load(std::memory_order_relaxed);
                const auto tail = tail_.load(std::memory_order_acquire);

                if (head >= tail + Capacity)
                {
                    return false;          // full
                }
                auto& slot = ring_[head & mask];

                if (slot.load(std::memory_order_relaxed) != T{})
                {
                    return false;          // slot still occupied (rare race)
                }
                slot.store(value, std::memory_order_release);
                head_.store(head + 1, std::memory_order_release);
                return true;
            }

            /**
             * @brief Try to pop a value.
             * @return value or std::nullopt if empty.
             */
            [[nodiscard]]
            std::optional<T> try_pop() noexcept
            {
                const auto tail = tail_.load(std::memory_order_relaxed);
                const auto head = head_.load(std::memory_order_acquire);

                if (tail >= head)
                {
                    return std::nullopt;   // empty
                }
                auto& slot = ring_[tail & mask];
                T value = slot.load(std::memory_order_acquire);

                if (value == T{})
                {
                    return std::nullopt;
                }
                slot.store(T{}, std::memory_order_release);
                tail_.store(tail + 1, std::memory_order_release);
                return value;
            }

            [[nodiscard]] bool empty() const noexcept
            {
                return tail_.load(std::memory_order_acquire) >= head_.load(std::memory_order_acquire);
            }

            [[nodiscard]] std::size_t size_approx() const noexcept
            {
                const auto h = head_.load(std::memory_order_relaxed);
                const auto t = tail_.load(std::memory_order_relaxed);
                return (h >= t) ? (h - t) : 0;
            }

            /** Built-in self-test (same logic as masscan). */
            [[nodiscard]]
            static bool selftest() noexcept;

        private:
            alignas(64) std::atomic<std::uint64_t> head_{0};
            alignas(64) std::atomic<std::uint64_t> tail_{0};
            std::array<std::atomic<T>, Capacity>   ring_;
    };

}









