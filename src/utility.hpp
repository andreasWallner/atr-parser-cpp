#ifndef header_atr_utility_h_
#define header_atr_utility_h_

#include <cstddef>

namespace atr {

constexpr std::byte operator""_b(unsigned long long x) {
  return static_cast<std::byte>(x);
}

constexpr int popcount(std::byte b) noexcept {
  return std::to_integer<int>((b >> 0) & 1_b) +
         std::to_integer<int>((b >> 1) & 1_b) +
         std::to_integer<int>((b >> 2) & 1_b) +
         std::to_integer<int>((b >> 3) & 1_b) +
         std::to_integer<int>((b >> 4) & 1_b) +
         std::to_integer<int>((b >> 5) & 1_b) +
         std::to_integer<int>((b >> 6) & 1_b) +
         std::to_integer<int>((b >> 7) & 1_b);
}

} // namespace atr

#endif