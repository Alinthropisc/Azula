#include <cstring>
#include <vector>
#include <string>

#include "base64.hh"


using namespace azula::crypto;

namespace
{

    constexpr char B64_TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    constexpr unsigned char DECODE_TABLE[256] = {
        // 0-31
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        // 32-63
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,62,0xFF,0xFF,0xFF,63,
        52,53,54,55,56,57,58,59,60,61,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        // 64-95
        0xFF, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,0xFF,0xFF,0xFF,0xFF,0xFF,
        // 96-127
        0xFF,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,0xFF,0xFF,0xFF,0xFF,0xFF,
        // rest
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    };

}

std::size_t Base64::encode(std::span<char> dst,std::span<const std::uint8_t> src) noexcept
{
    std::size_t i = 0, d = 0;
    const auto* s = src.data();
    auto* out = dst.data();
    const std::size_t src_len = src.size();
    const std::size_t dst_len = dst.size();

    while (i + 3 <= src_len)
    {
        if (d + 4 > dst_len)
        {
            return d;
        }
        unsigned n = (s[i] << 16) | (s[i+1] << 8) | s[i+2];
        out[d+0] = B64_TABLE[(n >> 18) & 0x3F];
        out[d+1] = B64_TABLE[(n >> 12) & 0x3F];
        out[d+2] = B64_TABLE[(n >>  6) & 0x3F];
        out[d+3] = B64_TABLE[(n >>  0) & 0x3F];
        i += 3; d += 4;
    }

    if (i + 2 <= src_len && d + 4 <= dst_len)
    {
        unsigned n = (s[i] << 16) | (s[i+1] << 8);
        out[d+0] = B64_TABLE[(n >> 18) & 0x3F];
        out[d+1] = B64_TABLE[(n >> 12) & 0x3F];
        out[d+2] = B64_TABLE[(n >>  6) & 0x3F];
        out[d+3] = '=';
        d += 4;
    }
    else if (i + 1 <= src_len && d + 4 <= dst_len)
    {
        unsigned n = s[i] << 16;
        out[d+0] = B64_TABLE[(n >> 18) & 0x3F];
        out[d+1] = B64_TABLE[(n >> 12) & 0x3F];
        out[d+2] = '=';
        out[d+3] = '=';
        d += 4;
    }
    return d;
}

std::size_t Base64::decode(std::span<std::uint8_t> dst,std::span<const char> src) noexcept
{
    std::size_t i = 0, d = 0;
    const auto* s = reinterpret_cast<const unsigned char*>(src.data());
    auto* out = dst.data();
    const std::size_t src_len = src.size();
    const std::size_t dst_len = dst.size();

    while (i < src_len)
    {
        unsigned c = 0, b = 0;

        while (i < src_len && (c = DECODE_TABLE[s[i]]) > 64)
        {
            ++i;
        }

        if (i >= src_len || s[i] == '=')
        {
            break;
        }
        ++i;
        b = (c << 2) & 0xFC;

        while (i < src_len && (c = DECODE_TABLE[s[i]]) > 64)
        {
            ++i;
        }

        if (i >= src_len || s[i] == '=')
        {
            break;
        }
        ++i;
        b |= (c >> 4) & 0x03;
        if (d < dst_len)
        {
            out[d++] = static_cast<std::uint8_t>(b);
        }
        if (i >= src_len)
        {
            break;
        }
        b = (c << 4) & 0xF0;

        while (i < src_len && s[i] != '=' && (c = DECODE_TABLE[s[i]]) > 64)
        {
            ++i;
        }
        if (i >= src_len || s[i] == '=')
        {
            break;
        }
        ++i;
        b |= (c >> 2) & 0x0F;
        if (d < dst_len)
        {
            out[d++] = static_cast<std::uint8_t>(b);
        }
        if (i >= src_len)
        {
            break;
        }

        b = (c << 6) & 0xC0;

        while (i < src_len && s[i] != '=' && (c = DECODE_TABLE[s[i]]) > 64)
        {
            ++i;
        }
        if (i >= src_len || s[i] == '=')
        {
            break;
        }
        ++i;
        b |= c;

        if (d < dst_len)
        {
            out[d++] = static_cast<std::uint8_t>(b);
        }
    }
    return d;
}

std::string Base64::encode(std::span<const std::uint8_t> src)
{
    std::string result((src.size() + 2) / 3 * 4, '\0');
    const auto written = encode(result, src);
    result.resize(written);
    return result;
}

std::vector<std::uint8_t> Base64::decode(std::string_view src)
{
    std::vector<std::uint8_t> result(src.size() * 3 / 4 + 4);
    const auto written = decode(result, src);
    result.resize(written);
    return result;
}


bool Base64::selftest() noexcept
{
    const char* hello = "hello";
    char buf[32]{};
    char buf2[32]{};
    auto len = encode(buf, {reinterpret_cast<const std::uint8_t*>(hello), 5});
    auto len2 = decode({reinterpret_cast<std::uint8_t*>(buf2), sizeof(buf2)},{buf, len});
    return len2 == 5 && std::memcmp(buf2, "hello", 5) == 0;
}

