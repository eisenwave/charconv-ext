#include <random>
#include <string_view>
#include <system_error>

#include "charconv_ext/charconv_ext.hpp"

namespace charconv_ext {
namespace {

#ifdef CHARCONV_EXT_128_BIT_IMPLEMENTATION
static_assert(detail::u64_max_representable_digits_naive(2) == 64);
static_assert(detail::u64_max_representable_digits(2) == 64);

static_assert(detail::u64_max_representable_digits_naive(8) == 21);
static_assert(detail::u64_max_representable_digits(8) == 21);

static_assert(detail::u64_max_representable_digits_naive(10) == 19);
static_assert(detail::u64_max_representable_digits(10) == 19);

static_assert(detail::u64_max_representable_digits_naive(16) == 16);
static_assert(detail::u64_max_representable_digits(16) == 16);

static_assert(detail::u64_max_power(2) == 0);
static_assert(detail::u64_max_power(8) == 0x8000000000000000ull);
static_assert(detail::u64_max_power(10) == 10000000000000000000ull);
static_assert(detail::u64_max_power(16) == 0);
#endif

template <typename T>
struct test_case {
    T value;
    std::string_view str;
    int base;
};

constexpr auto u128_test = (uint128_t(1) << 100) / 10;
constexpr auto u128_max = uint128_t(-1);

constexpr auto i128_min = int128_t(1) << 127;

// clang-format off
static constexpr test_case<uint128_t> test_cases_u128[] {
    { 0, "0", 2 },
    { 0, "0", 5 },
    { 0, "0", 8 },
    { 0, "0", 10 },
    { 0, "0", 16 },
    { 0, "0", 32 },

    { 255, "11111111", 2 },
    { 255, "2010", 5 },
    { 255, "377", 8 },
    { 255, "255", 10 },
    { 255, "ff", 16 },
    { 255, "7v", 32 },

    { u128_test, "1100110011001100110011001100110011001100110011001100110011001100110011001100110011001100110011001", 2 },
    { u128_test, "234321103241341010413041402403011100224122", 5 },
    { u128_test, "146314631463146314631463146314631", 8 },
    { u128_test, "126765060022822940149670320537", 10 },
    { u128_test, "1999999999999999999999999", 16 },
    { u128_test, "36cpj6cpj6cpj6cpj6cp", 32 },

    { u128_max, "11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111", 2 },
    { u128_max, "11031110441201303134210404233413032443021130230130231310", 5 },
    { u128_max, "3777777777777777777777777777777777777777777", 8 },
    { u128_max, "340282366920938463463374607431768211455", 10 },
    { u128_max, "ffffffffffffffffffffffffffffffff", 16 },
    { u128_max, "7vvvvvvvvvvvvvvvvvvvvvvvvv", 32 },
};

static constexpr test_case<int128_t> test_cases_i128[] {
    { 0, "0", 2 },
    { 0, "0", 5 },
    { 0, "0", 8 },
    { 0, "0", 10 },
    { 0, "0", 16 },
    { 0, "0", 32 },

    { 255, "11111111", 2 },
    { 255, "2010", 5 },
    { 255, "377", 8 },
    { 255, "255", 10 },
    { 255, "ff", 16 },
    { 255, "7v", 32 },

    { u128_test, "1100110011001100110011001100110011001100110011001100110011001100110011001100110011001100110011001", 2 },
    { u128_test, "234321103241341010413041402403011100224122", 5 },
    { u128_test, "146314631463146314631463146314631", 8 },
    { u128_test, "126765060022822940149670320537", 10 },
    { u128_test, "1999999999999999999999999", 16 },
    { u128_test, "36cpj6cpj6cpj6cpj6cp", 32 },

    { i128_min, "-10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 2 },
    { i128_min, "-3013030220323124042102424341431241221233040112312340403", 5 },
    { i128_min, "-2000000000000000000000000000000000000000000", 8 },
    { i128_min, "-170141183460469231731687303715884105728", 10 },
    { i128_min, "-80000000000000000000000000000000", 16 },
    { i128_min, "-40000000000000000000000000", 32 },
};
// clang-format on

void run_manual_tests()
{
    char buffer[1024];
    for (const auto& test : test_cases_u128) {
        const auto [p, ec] = to_chars(buffer, std::end(buffer), test.value, test.base);
        assert(ec == std::errc {});
        assert(std::size_t(p - buffer) == test.str.size());

        const std::string_view result = { buffer, p };
        assert(std::ranges::equal(result, test.str));

        uint128_t value;
        const auto [from_p, from_ec] = from_chars(buffer, p, value, test.base);
        assert(from_ec == std::errc {});
        assert(p == from_p);
        assert(value == test.value);
    }

    for (const auto& test : test_cases_i128) {
        const auto [p, ec] = to_chars(buffer, std::end(buffer), test.value, test.base);
        assert(ec == std::errc {});
        assert(std::size_t(p - buffer) == test.str.size());

        const std::string_view result = { buffer, p };
        assert(std::ranges::equal(result, test.str));

        int128_t value;
        const auto [from_p, from_ec] = from_chars(buffer, p, value, test.base);
        assert(from_ec == std::errc {});
        assert(p == from_p);
        assert(value == test.value);
    }
}

void run_fuzz_tests()
{
    constexpr int iterations = 1'000'000;

    char buffer[1024];
    std::default_random_engine rng { 12345 };
    std::uniform_int_distribution<int> base_distr { 2, 36 };
    std::uniform_int_distribution<uint64_t> u64_distr;

    for (int i = 0; i < iterations; ++i) {
        const int base = base_distr(rng);
        const auto lo = u64_distr(rng);
        const auto hi = u64_distr(rng);

        const auto u128 = (uint128_t(hi) << 64) | lo;
        const auto i128 = int128_t(u128);

        {
            const auto [p1, ec1] = to_chars(buffer, std::end(buffer), u128, base);
            assert(ec1 == std::errc {});
            *p1 = 0;

            uint128_t u128_parsed;
            const auto [p2, ec2] = from_chars(buffer, std::end(buffer), u128_parsed, base);
            assert(ec2 == std::errc {});
            assert(p1 == p2);
            assert(u128 == u128_parsed);
        }

        {
            const auto [p1, ec1] = to_chars(buffer, std::end(buffer), i128, base);
            assert(ec1 == std::errc {});
            *p1 = 0;

            int128_t i128_parsed;
            const auto [p2, ec2] = from_chars(buffer, std::end(buffer), i128_parsed, base);
            assert(ec2 == std::errc {});
            assert(p1 == p2);
            assert(i128 == i128_parsed);
        }
    }
}

} // namespace

#ifdef CHARCONV_EXT_BITINT_MAXWIDTH
template std::to_chars_result to_chars(char*, char*, bit_int<2>, int);
template std::to_chars_result to_chars(char*, char*, bit_int<8>, int);
template std::to_chars_result to_chars(char*, char*, bit_int<16>, int);
template std::to_chars_result to_chars(char*, char*, bit_int<32>, int);
template std::to_chars_result to_chars(char*, char*, bit_int<64>, int);

template std::to_chars_result to_chars(char*, char*, bit_uint<1>, int);
template std::to_chars_result to_chars(char*, char*, bit_uint<8>, int);
template std::to_chars_result to_chars(char*, char*, bit_uint<16>, int);
template std::to_chars_result to_chars(char*, char*, bit_uint<32>, int);
template std::to_chars_result to_chars(char*, char*, bit_uint<64>, int);

template std::from_chars_result from_chars(const char*, const char*, bit_int<2>&, int);
template std::from_chars_result from_chars(const char*, const char*, bit_int<8>&, int);
template std::from_chars_result from_chars(const char*, const char*, bit_int<16>&, int);
template std::from_chars_result from_chars(const char*, const char*, bit_int<32>&, int);
template std::from_chars_result from_chars(const char*, const char*, bit_int<64>&, int);

template std::from_chars_result from_chars(const char*, const char*, bit_uint<1>&, int);
template std::from_chars_result from_chars(const char*, const char*, bit_uint<8>&, int);
template std::from_chars_result from_chars(const char*, const char*, bit_uint<16>&, int);
template std::from_chars_result from_chars(const char*, const char*, bit_uint<32>&, int);
template std::from_chars_result from_chars(const char*, const char*, bit_uint<64>&, int);

#if CHARCONV_EXT_BITINT_MAXWIDTH >= 128
template std::to_chars_result to_chars(char*, char*, bit_int<100>, int);
template std::to_chars_result to_chars(char*, char*, bit_int<128>, int);
template std::to_chars_result to_chars(char*, char*, bit_uint<100>, int);
template std::to_chars_result to_chars(char*, char*, bit_uint<128>, int);
template std::from_chars_result from_chars(const char*, const char*, bit_int<100>&, int);
template std::from_chars_result from_chars(const char*, const char*, bit_int<128>&, int);
template std::from_chars_result from_chars(const char*, const char*, bit_uint<100>&, int);
template std::from_chars_result from_chars(const char*, const char*, bit_uint<128>&, int);
#endif

#endif

} // namespace charconv_ext

int main()
{
    charconv_ext::run_manual_tests();
    charconv_ext::run_fuzz_tests();
}
