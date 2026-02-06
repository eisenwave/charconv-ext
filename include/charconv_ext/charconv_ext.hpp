#ifndef CHARCONV_EXT_HPP
#define CHARCONV_EXT_HPP

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <climits>
#include <cstdint>
#include <exception>
#include <system_error>

#ifdef BITINT_MAXWIDTH
#define CHARCONV_EXT_BITINT_MAXWIDTH BITINT_MAXWIDTH
#elif defined(__BITINT_MAXWIDTH__)
#define CHARCONV_EXT_BITINT_MAXWIDTH __BITINT_MAXWIDTH__
#endif

#ifndef CHARCONV_EXT_ASSERT
#include <cassert>
#define CHARCONV_EXT_ASSERT(...) assert(__VA_ARGS__)
#endif

#ifndef CHARCONV_EXT_UNREACHABLE
#ifdef NDEBUG
#define CHARCONV_EXT_UNREACHABLE() __builtin_unreachable()
#else
#define CHARCONV_EXT_UNREACHABLE() ::std::terminate()
#endif
#endif

#ifdef __GLIBCXX_BITSIZE_INT_N_0
#if __GLIBCXX_BITSIZE_INT_N_0 == 128
#define CHARCONV_EXT_128_BIT_PROVIDED_BY_STANDARD_LIBRARY 1
#endif
#endif

namespace charconv_ext {

// If the standard library already provides to_chars and from_chars for 128-bit
// and the user does not want to use that,
// we need to avoid using-declarations because this leads to conflicting declarations.
#if defined(CHARCONV_EXT_128_BIT_PROVIDED_BY_STANDARD_LIBRARY)                                     \
    && !defined(CHARCONV_EXT_DONT_USE_STANDARD_LIBRARY)
using std::from_chars;
using std::to_chars;
#endif

#if defined(__GNUC__) || defined(__clang__)
__extension__ typedef signed __int128 int128_t; // NOLINT modernize-use-using
__extension__ typedef unsigned __int128 uint128_t; // NOLINT modernize-use-using
#else
#error "Only Clang and GCC are supported."
#endif

// Recent versions of GCC and Clang (~2025) already provide support for __int128
// in to_chars and from_chars, so we should avoid
#if !defined(CHARCONV_EXT_128_BIT_PROVIDED_BY_STANDARD_LIBRARY)                                    \
    || defined(CHARCONV_EXT_DONT_USE_STANDARD_LIBRARY)
#define CHARCONV_EXT_128_BIT_IMPLEMENTATION 1

namespace detail {

[[nodiscard]]
consteval int u64_max_representable_digits_naive(const int base)
{
    CHARCONV_EXT_ASSERT(base >= 2);

    const auto max = int128_t { 1 } << 64;
    int128_t x = 1;
    int result = 0;
    while (x <= max) {
        x *= unsigned(base);
        ++result;
        if (x == 0) {
            break;
        }
    }
    return result - 1;
}

inline constexpr auto u64_max_representable_digits_table = [] {
    std::array<signed char, 37> result {};
    for (std::size_t i = 2; i < result.size(); ++i) {
        result[i] = static_cast<signed char>(u64_max_representable_digits_naive(int(i)));
    }
    return result;
}();

/// @brief Returns the amount of digits that `std::uint64_t` can represent
/// in the given base.
/// Mathematically, this is `floor(log(pow(2, 64)) / log(base))`.
[[nodiscard]]
constexpr int u64_max_representable_digits(const int base)
{
    CHARCONV_EXT_ASSERT(base >= 2);
    CHARCONV_EXT_ASSERT(base <= 36);

    return u64_max_representable_digits_table[std::size_t(base)];
}

[[nodiscard]]
consteval std::uint64_t u64_pow_naive(const std::uint64_t x, const int y)
{
    std::uint64_t result = 1;
    for (int i = 0; i < y; ++i) {
        result *= x;
    }
    return result;
}

inline constexpr auto u64_max_power_table = [] {
    std::array<std::uint64_t, 37> result {};
    for (std::size_t i = 2; i < result.size(); ++i) {
        const int max_exponent = u64_max_representable_digits(int(i));
        result[i] = u64_pow_naive(i, max_exponent);
    }
    return result;
}();

/// @brief Returns the greatest power of `base` representable in `std::uint64_t`,
/// or zero if the next greater power is exactly `pow(2, 64)`.
///
/// A result of zero communicates that no bit of `std::uint64_t` is wasted,
/// such as in the base-2 or base-16 case.
[[nodiscard]]
constexpr std::uint64_t u64_max_power(const int base)
{
    CHARCONV_EXT_ASSERT(base >= 2);
    CHARCONV_EXT_ASSERT(base <= 36);

    return u64_max_power_table[std::size_t(base)];
}

/// @brief Computes `out = x + y` and returns `true`
/// if the result could not be exactly represented .
[[nodiscard]]
constexpr bool add_overflow(uint128_t& out, const uint128_t x, const uint128_t y) noexcept
{
    return __builtin_add_overflow(x, y, &out);
}

/// @brief Computes `out = x * y` and returns `true`
/// if the result could not be exactly represented .
[[nodiscard]]
constexpr bool mul_overflow(uint128_t& out, const uint128_t x, const uint128_t y) noexcept
{
    return __builtin_mul_overflow(x, y, &out);
}

[[nodiscard]]
constexpr int digit_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    }
    return -1;
}

[[nodiscard]]
constexpr std::size_t
pattern_length(const char* const first, const char* const last, const int base)
{
    std::size_t result;
    for (result = 0; first + result < last; ++result) {
        const int value = digit_value(first[result]);
        if (value < 0 || value >= base) {
            break;
        }
    }
    return result;
}

} // namespace detail

/// @brief Implements the interface of `to_chars` for decimal input of 128-bit integers.
/// In the "happy case" of having at most 19 digits,
/// this simply calls `std::from_chars` for 64-bit integers.
/// In the worst case, three such 64-bit calls are needed,
/// handling 19 digits at a time, with 39 decimal digits being the maximum for 128-bit.
constexpr std::from_chars_result
from_chars(const char* const first, const char* const last, uint128_t& out, int base = 10)
{
    CHARCONV_EXT_ASSERT(base >= 2);
    CHARCONV_EXT_ASSERT(base <= 36);
    CHARCONV_EXT_ASSERT(first);
    CHARCONV_EXT_ASSERT(last);

    if (first == last) {
        return { last, std::errc::invalid_argument };
    }

    const char* const initial_last = first + detail::pattern_length(first, last, base);

    uint128_t result = 0;
    const char* current_last = initial_last;

    const std::uint64_t max_pow = detail::u64_max_power(base);
    const std::ptrdiff_t max_lower_length = detail::u64_max_representable_digits(base);
    const bool is_pow_2 = (base & (base - 1)) == 0;
    CHARCONV_EXT_ASSERT(max_pow != 0 || is_pow_2);

    if (is_pow_2) {
        const int bits_per_iteration = std::countr_zero(max_pow);
        int shift = 0;

        while (true) {
            const auto lower_length = std::min(current_last - first, max_lower_length);
            const char* const current_first = current_last - lower_length;

            std::uint64_t digits {};
            const std::from_chars_result partial_result
                = std::from_chars(current_first, current_last, digits, base);
            if (partial_result.ec != std::errc {}) {
                // Since we only handle as many digits as can fit into a 64-bit integer,
                // the only possible failure should be an invalid string.
                CHARCONV_EXT_ASSERT(partial_result.ec == std::errc::invalid_argument);
                return partial_result;
            }

            const int added_digits = 64 - std::countl_zero(digits);
            if (shift + added_digits > 128) {
                return { initial_last, std::errc::result_out_of_range };
            }

            result |= uint128_t(digits) << shift;
            shift += bits_per_iteration;

            if (current_last - first <= max_lower_length || partial_result.ec != std::errc {}) {
                out = result;
                return { initial_last, partial_result.ec };
            }
            current_last -= lower_length;
            CHARCONV_EXT_ASSERT(current_last >= first);
        }
    }
    else {
        uint128_t factor = 1;

        while (true) {
            const auto lower_length = std::min(current_last - first, max_lower_length);
            const char* const current_first = current_last - lower_length;

            std::uint64_t digits {};
            const std::from_chars_result partial_result
                = std::from_chars(current_first, current_last, digits, base);

            uint128_t summand;
            if (detail::mul_overflow(summand, factor, digits)) {
                return { initial_last, std::errc::result_out_of_range };
            }
            if (detail::add_overflow(result, result, summand)) {
                return { initial_last, std::errc::result_out_of_range };
            }

            if (current_last - first <= max_lower_length || partial_result.ec != std::errc {}) {
                out = result;
                return { initial_last, partial_result.ec };
            }
            factor *= max_pow;
            current_last -= lower_length;
            CHARCONV_EXT_ASSERT(current_last >= first);
        }
    }
}

constexpr std::from_chars_result
from_chars(const char* const first, const char* const last, int128_t& out, const int base = 10)
{
    CHARCONV_EXT_ASSERT(first);
    CHARCONV_EXT_ASSERT(last);

    if (first == last) {
        return { last, std::errc::invalid_argument };
    }

    if (*first != '-') {
        uint128_t x {};
        const std::from_chars_result result = from_chars(first, last, x, base);
        if (x >> 127) {
            return { result.ptr, std::errc::result_out_of_range };
        }
        out = int128_t(x);
        return result;
    }
    const std::ptrdiff_t max_lower_length = detail::u64_max_representable_digits(base);
    if (last - first + 1 <= max_lower_length) {
        std::int64_t x {};
        const std::from_chars_result result = std::from_chars(first, last, x, base);
        out = x;
        return result;
    }
    constexpr auto max_u128 = uint128_t { 1 } << 127;
    uint128_t x {};
    const std::from_chars_result result = from_chars(first + 1, last, x, base);
    if (x > max_u128) {
        return { result.ptr, std::errc::result_out_of_range };
    }
    out = int128_t(-x);
    return result;
}

constexpr std::to_chars_result
to_chars(char* const first, char* const last, const uint128_t x, const int base = 10)
{
    CHARCONV_EXT_ASSERT(first);
    CHARCONV_EXT_ASSERT(last);
    CHARCONV_EXT_ASSERT(base >= 2);
    CHARCONV_EXT_ASSERT(base <= 36);

    if (x <= std::uint64_t(-1)) {
        return std::to_chars(first, last, std::uint64_t(x), base);
    }
    if (first == last) {
        return { last, std::errc::value_too_large };
    }

    const std::uint64_t max_pow = detail::u64_max_power(base);
    const bool is_pow_2 = (base & (base - 1)) == 0;
    const int piece_max_digits = detail::u64_max_representable_digits(base);

    if (is_pow_2) {
        const int bits_per_iteration = std::countr_zero(max_pow);
        CHARCONV_EXT_ASSERT(bits_per_iteration != 0);
        const int leading_bits = 128 % bits_per_iteration;

        char* current_first = first;
        bool first_digit = true;

        // First, we need to take care of the leading "head" bits.
        // For example, for octal, we operate on 63 bits at a time,
        // and 2 leading bits are left over.
        if (leading_bits != 0) {
            const auto head_mask = (std::uint64_t(1) << leading_bits) - 1;
            const auto head = std::uint64_t(x >> (128 - leading_bits)) & head_mask;
            if (head != 0) {
                first_digit = false;
                const std::to_chars_result head_result = std::to_chars(first, last, head, base);
                if (head_result.ec != std::errc {}) {
                    return head_result;
                }
                current_first = head_result.ptr;
            }
        }

        int shift = 128 - leading_bits - bits_per_iteration;
        CHARCONV_EXT_ASSERT(bits_per_iteration <= 64);
        const auto mask = //
            (bits_per_iteration == 64 ? std::uint64_t(0) : (std::uint64_t(1) << bits_per_iteration))
            - 1;

        // Once the head digits are printed out, every subsequent block of bits
        // has exactly the same amount of digits.
        // For example, for octal, there are 126 bits left,
        // handled exactly 63 bits at a time.
        while (true) {
            const auto piece = std::uint64_t(x >> shift) & mask;
            const std::to_chars_result piece_result
                = std::to_chars(current_first, last, piece, base);
            if (piece_result.ec != std::errc {}) {
                return piece_result;
            }
            const auto zero_pad = piece_max_digits - int(piece_result.ptr - current_first);
            if (!first_digit && zero_pad != 0) {
                std::ranges::copy(current_first, piece_result.ptr, current_first + zero_pad);
                std::ranges::fill_n(current_first, zero_pad, '0');
                current_first += piece_max_digits;
            }
            else {
                current_first = piece_result.ptr;
            }

            if (shift <= 0) {
                CHARCONV_EXT_ASSERT(shift == 0);
                return { current_first, std::errc {} };
            }
            shift -= bits_per_iteration;
            first_digit = false;
        }
    }
    // NOLINTNEXTLINE(readability-else-after-return)
    else {
        const std::to_chars_result upper_result = to_chars(first, last, x / max_pow, base);
        if (upper_result.ec != std::errc {}) {
            return upper_result;
        }

        const std::to_chars_result lower_result
            = std::to_chars(upper_result.ptr, last, std::uint64_t(x % max_pow), base);
        if (lower_result.ec != std::errc {}) {
            return lower_result;
        }
        const auto lower_length = lower_result.ptr - upper_result.ptr;

        // The remainder (lower part) is mathematically exactly 19 digits long,
        // and we have to zero-pad to the left if it is shorter
        // (because std::to_chars wouldn't give us the leading zeros we need).
        char* const result_end = upper_result.ptr + piece_max_digits;
        std::ranges::copy(
            upper_result.ptr, upper_result.ptr + lower_length, result_end - lower_length
        );
        std::ranges::fill_n(upper_result.ptr, piece_max_digits - lower_length, '0');

        return { result_end, std::errc {} };
    }
    CHARCONV_EXT_UNREACHABLE();
}

constexpr std::to_chars_result
to_chars(char* const first, char* const last, const int128_t x, const int base = 10)
{
    CHARCONV_EXT_ASSERT(first);
    CHARCONV_EXT_ASSERT(last);
    CHARCONV_EXT_ASSERT(base >= 2);
    CHARCONV_EXT_ASSERT(base <= 36);

    if (x >= 0) {
        return to_chars(first, last, uint128_t(x), base);
    }
    if (x == std::int64_t(x)) {
        return std::to_chars(first, last, std::int64_t(x), base);
    }
    if (last - first < 2) {
        return { last, std::errc::value_too_large };
    }
    *first = '-';
    return to_chars(first + 1, last, -uint128_t(x), base);
}

#endif

#ifdef CHARCONV_EXT_BITINT_MAXWIDTH
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wbit-int-extension"
#endif

template <std::size_t N>
using bit_int = _BitInt(N);
template <std::size_t N>
using bit_uint = unsigned _BitInt(N);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace detail {

template <std::size_t N>
consteval auto int_least()
{
    if constexpr (N <= 8) {
        return std::int_least8_t {};
    }
    else if constexpr (N <= 16) {
        return std::int_least16_t {};
    }
    else if constexpr (N <= 32) {
        return std::int_least32_t {};
    }
    else if constexpr (N <= 64) {
        return std::int_least64_t {};
    }
    else if constexpr (N <= 128) {
        return int128_t {};
    }
}

template <std::size_t N>
consteval auto uint_least()
{
    if constexpr (N <= 8) {
        return std::uint_least8_t {};
    }
    else if constexpr (N <= 16) {
        return std::uint_least16_t {};
    }
    else if constexpr (N <= 32) {
        return std::uint_least32_t {};
    }
    else if constexpr (N <= 64) {
        return std::uint_least64_t {};
    }
    else if constexpr (N <= 128) {
        return uint128_t {};
    }
}

template <std::size_t N>
using int_leastN_t = decltype(int_least<N>());

template <std::size_t N>
using uint_leastN_t = decltype(uint_least<N>());

} // namespace detail

template <std::size_t N>
constexpr std::to_chars_result to_chars(
    char* const first, //
    char* const last,
    const bit_int<N> x,
    const int base = 10
)
{
    using std::to_chars;
    static_assert(N <= 128, "Sorry, to_chars for _BitInt(129) and wider not implemented :(");
    return to_chars(first, last, detail::int_leastN_t<N> { x }, base);
}

template <std::size_t N>
constexpr std::to_chars_result to_chars(
    char* const first, //
    char* const last,
    const bit_uint<N> x,
    const int base = 10
)
{
    using std::to_chars;
    static_assert(N <= 128, "Sorry, to_chars for _BitInt(129) and wider not implemented :(");
    return to_chars(first, last, detail::uint_leastN_t<N> { x }, base);
}

template <std::size_t N>
constexpr std::from_chars_result from_chars(
    const char* const first, //
    const char* const last,
    bit_int<N>& x,
    const int base = 10
)
{
    using std::from_chars;
    static_assert(N <= 128, "Sorry, from_chars for _BitInt(129) and wider not implemented :(");
    detail::int_leastN_t<N> value {};
    auto result = from_chars(first, last, value, base);
    x = static_cast<bit_int<N>>(value);
    if (result.ec == std::errc {} && x != value) {
        // This detects the case where parsing succeeded,
        // but the result is not exactly representable as a bit-precise integer.
        result.ec = std::errc::result_out_of_range;
    }
    return result;
}

template <std::size_t N>
constexpr std::from_chars_result from_chars(
    const char* const first, //
    const char* const last,
    bit_uint<N>& x,
    const int base = 10
)
{
    using std::from_chars;
    static_assert(
        N <= 128, "Sorry, from_chars for unsigned _BitInt(129) and wider not implemented :("
    );
    detail::uint_leastN_t<N> value {};
    auto result = from_chars(first, last, value, base);
    x = static_cast<bit_uint<N>>(value);
    if (result.ec == std::errc {} && x != value) {
        // This detects the case where parsing succeeded,
        // but the result is not exactly representable as a bit-precise integer.
        result.ec = std::errc::result_out_of_range;
    }
    return result;
}
#endif

} // namespace charconv_ext

#endif
