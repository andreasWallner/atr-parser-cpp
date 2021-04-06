#include "atr.hpp"
#include "utility.hpp"

namespace atr {

std::vector<std::byte>
receive(std::function<bool(gsl::span<std::byte> buffer)> recv_func) {
  constexpr std::size_t max_atr_size = 32;
  std::array<std::byte, max_atr_size> memory;
  gsl::span<std::byte> remaining(memory);
  bool needs_tck = false;

  if (!recv_func(remaining.first(2)))
    return {};

  if (remaining[0] != 0x3B_b)
    return {};
  auto TDx = remaining[1];
  const auto K = std::to_integer<int>(remaining[1] & 0x0f_b);
  remaining = remaining.subspan(2);

  int next_block_len;
  while ((next_block_len = popcount(TDx & 0xf0_b))) {
    if (next_block_len > remaining.size())
      return {};

    if (!recv_func(remaining.first(next_block_len)))
      return {};

    if ((TDx & 0x80_b) != 0_b)
      TDx = remaining[next_block_len - 1];
    else
      TDx = 0_b;

    if ((TDx & 0x0f_b) != 0_b)
      needs_tck = true;

    remaining = remaining.subspan(next_block_len);
  }

  if (!recv_func(remaining.first(K)))
    return {};
  remaining = remaining.subspan(K);

  if (needs_tck) {
    if (!recv_func(remaining.first(1)))
      return {};
    remaining = remaining.subspan(1);
  }

  return {memory.begin(), memory.end() - remaining.size()};
}

} // namespace atr