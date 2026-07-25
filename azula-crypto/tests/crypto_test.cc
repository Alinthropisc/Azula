#include <gtest/gtest.h>

#include <vector>
#include <string>
#include <cstring>
#include <thread>
#include <atomic>

#include "../lib/blackrock.hh"
#include "../lib/siphash.hh"
#include "../lib/base64.hh"
#include "../lib/lcg.hh"
#include "../lib/xring.hh"

using namespace azula::crypto;


TEST(BlackRock2, RoundTripSmall)
{
    auto br = BlackRock2::create(1000, 0xDEADBEEF, 6);

    for (std::uint64_t i = 0; i < 1000; ++i)
    {
        auto s = br.shuffle(i);
        EXPECT_EQ(br.unshuffle(s), i);
        EXPECT_LT(s, 1000u);
    }
}

TEST(BlackRock2, Bijective)
{
    constexpr std::uint64_t N = 5000;
    auto br = BlackRock2::create(N, 12345, 6);
    std::vector<bool> seen(N, false);

    for (std::uint64_t i = 0; i < N; ++i)
    {
        auto x = br.shuffle(i);
        ASSERT_LT(x, N);
        EXPECT_FALSE(seen[x]) << "collision at " << x;
        seen[x] = true;
    }
}

TEST(BlackRock2, SelfTest)
{
    EXPECT_TRUE(BlackRock2::selftest());
}

TEST(SipHash24, OfficialVectors)
{
    EXPECT_TRUE(SipHash24::selftest());
}

TEST(SipHash24, Deterministic)
{
    SipHash24::Key key{0x0123456789abcdefULL, 0xfedcba9876543210ULL};
    SipHash24 h(key);
    const char* msg = "azula-crypto-test";
    auto a = h.hash(msg, std::strlen(msg));
    auto b = h.hash(msg, std::strlen(msg));
    EXPECT_EQ(a, b);
}

TEST(SipHash24, DifferentKeys)
{
    SipHash24::Key k1{1, 2};
    SipHash24::Key k2{3, 4};
    SipHash24 h1(k1);
    SipHash24 h2(k2);
    const char* msg = "test";
    EXPECT_NE(h1.hash(msg, 4), h2.hash(msg, 4));
}


TEST(Base64, RoundTrip)
{
    const std::string original = "Hello, Azula! 12345";

    auto encoded = Base64::encode({
        reinterpret_cast<const std::uint8_t*>(original.data()),
        original.size()
    });
    auto decoded = Base64::decode(encoded);
    EXPECT_EQ(std::string(decoded.begin(), decoded.end()), original);
}

TEST(Base64, Empty)
{
    auto encoded = Base64::encode({});
    EXPECT_TRUE(encoded.empty());
    auto decoded = Base64::decode("");
    EXPECT_TRUE(decoded.empty());
}

TEST(Base64, SelfTest)
{
    EXPECT_TRUE(Base64::selftest());
}

TEST(LCG, BijectiveSmall)
{
    LCG lcg(10007);
    std::vector<bool> seen(10007, false);

    for (std::uint64_t i = 0; i < 10007; ++i)
    {
        auto x = lcg(i);
        ASSERT_LT(x, 10007u);
        EXPECT_FALSE(seen[x]);
        seen[x] = true;
    }
}

TEST(LCG, SelfTest)
{
    EXPECT_TRUE(LCG::selftest());
}

TEST(XRing, BasicPushPop)
{
    XRing<std::uint64_t, 16> ring;
    EXPECT_TRUE(ring.empty());
    EXPECT_TRUE(ring.try_push(42));
    EXPECT_FALSE(ring.empty());
    auto v = ring.try_pop();
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 42u);
    EXPECT_TRUE(ring.empty());
}

TEST(XRing, RejectZero)
{
    XRing<std::uint64_t, 16> ring;
    EXPECT_FALSE(ring.try_push(0));   // zero is reserved
}

TEST(XRing, FullAndEmpty)
{
    XRing<std::uint64_t, 8> ring;

    for (std::uint64_t i = 1; i <= 8; ++i)
    {
        EXPECT_TRUE(ring.try_push(i));
    }
    EXPECT_FALSE(ring.try_push(9));   // full

    for (std::uint64_t i = 1; i <= 8; ++i)
    {
        auto v = ring.try_pop();
        ASSERT_TRUE(v.has_value());
        EXPECT_EQ(*v, i);
    }
    EXPECT_FALSE(ring.try_pop().has_value());
}

TEST(XRing, SelfTest)
{
    EXPECT_TRUE(XRing<>::selftest());
}

TEST(XRing, MultiThreadStress)
{
    XRing<std::uint64_t, 32> ring;
    std::atomic<bool> done{false};
    std::atomic<std::uint64_t> total{0};

    std::thread producer([&] {
        for (std::uint64_t i = 500; i >= 1; --i)
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
    EXPECT_EQ(total.load(), 125250ULL);
}


