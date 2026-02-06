# `charconv-ext`

`charconv-ext` is a single-header library
which provides `to_chars` and `from_chars` functions that operate on 128-bit integers.
These have the same interface as `std::to_chars` and `std::from_chars`.
It also provides support for `_BitInt` (Clang compiler extension) for up to `_BitInt(128)`.

For example, it can be used like:

```cpp
#include <array>
#include <print>
#include "charconv_ext/charconv_ext.hpp"

int main() {
  std::array<char, 256> buffer;
  auto value = __int128(1) << 127;
  const auto [p, ec] =
    charconv_ext::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  std::println(std::string_view(buffer.data(), p));
}
```
This prints:
```
-170141183460469231731687303715884105728
```

The example could have also been written to use `_BitInt(128)`,
with the same output produced.

> [!NOTE]
> `charconv_ext::to_chars` and `charconv_ext::from_chars` also accept
> any other integer type because they pull in the `std::` versions via *using-declaration*.
> This means you don't need to special-case `__int128` in generic code
> and always call `charconv_ext::to_chars` for integers.


## Interface

```cpp
namespace charconv_ext {

template <std::size_t N>
  using bit_int = _BitInt(N); // optional
template <std::size_t N>
  using bit_uint = unsigned _BitInt(N); // optional

}
```

The `bit_int` and `bit_uint` alias templates provide access to the compiler's
`_BitInt` type, without triggering warnings such as `-Wbit-int-extension`
(due to use of compiler extensions).
These alias templates are only defined if the compiler supports `_BitInt`.

```cpp
std::to_chars_result charconv_ext::to_chars(
  char* first,
  char* last,
  /* integer-type */ value,
  int base = 10
);
```
*Effects*:
Equivalent to `std::to_chars(first, last, value, base)`,
except that `__int128` and `unsigned __int128` are also supported.

```cpp
template <std::size_t N>
std::to_chars_result charconv_ext::to_chars( // optional
  char* first,
  char* last,
  bit_int<N> value,
  int base = 10
);
```
*Mandates*:
`N` ≤ 128.

*Effects*:
Equivalent to `to_chars(first, last, int_type(value), base)`,
where `to_chars` is `std::to_chars` or `charconv_ext::to_chars`,
and where `int_type` is
`int_least8_t`, `int_least16_t`, `int_least32_t`, `int_least64_t`, or `int128_t`,
whichever first is at least `N` bits wide.

```cpp
template <std::size_t N>
std::to_chars_result charconv_ext::to_chars( // optional
  char* first,
  char* last,
  bit_uint<N> value,
  int base = 10
);
```
*Mandates*:
`N` ≤ 128.

*Effects*:
Equivalent to `to_chars(first, last, int_type(value), base)`,
where `to_chars` is `std::to_chars` or `charconv_ext::to_chars`,
and where `int_type` is
`uint_least8_t`, `uint_least16_t`, `uint_least32_t`, `uint_least64_t`, or `uint128_t`,
whichever first is at least `N` bits wide.

```cpp
std::from_chars_result charconv_ext::from_chars(
  const char* first,
  const char* last,
  /* integer-type */& value,
  int base = 10
);
```
*Effects*:
Equivalent to `std::from_chars(first, last, value, base)`,
except that `__int128` and `unsigned __int128` are also supported.

```cpp
template <std::size_t N>
std::to_chars_result charconv_ext::from_chars( // optional
  char* first,
  char* last,
  bit_int<N>& value,
  int base = 10
);
```
*Mandates*:
`N` ≤ 128.

*Effects*:
Equivalent to `from_chars(first, last, int_type(value), base)`,
where `from_chars` is `std::from_chars` or `charconv_ext::from_chars`,
and where `int_type` is
`int_least8_t`, `int_least16_t`, `int_least32_t`, `int_least64_t`, or `int128_t`,
whichever first is at least `N` bits wide.

```cpp
template <std::size_t N>
std::to_chars_result charconv_ext::from_chars( // optional
  char* first,
  char* last,
  bit_uint<N>& value,
  int base = 10
);
```
*Mandates*:
`N` ≤ 128.

*Effects*:
Equivalent to `from_chars(first, last, int_type(value), base)`,
where `from_chars` is `std::from_chars` or `charconv_ext::from_chars`,
and where `int_type` is
`uint_least8_t`, `uint_least16_t`, `uint_least32_t`, `uint_least64_t`, or `uint128_t`,
whichever first is at least `N` bits wide.
