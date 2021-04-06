#ifndef atr_header_
#define atr_header_

#include <chrono>
#include <cstddef>
#include <functional>
#include <gsl/span>
#include <vector>

namespace atr {

class invalid_atr : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

enum class if_char { A = 0x10, B = 0x20, C = 0x40, D = 0x80 };
enum class protocol_state { PPS, T0, T1 };
enum class clockstop_indicator {
  not_supported = 0,
  low = 1,
  high = 2,
  no_preference = 3
};
enum class operating_condition : uint8_t { A = 0x01, B = 0x02, C = 0x04 };
enum class redundancy_code { CRC, LRC };

constexpr inline char to_char(if_char c) {
  switch (c) {
  case if_char::A:
    return 'A';
  case if_char::B:
    return 'B';
  case if_char::C:
    return 'C';
  case if_char::D:
    return 'D';
  default:
    return '?';
  }
}

constexpr inline operating_condition operator&(operating_condition a,
                                               operating_condition b) noexcept {
  return static_cast<operating_condition>(static_cast<uint8_t>(a) &
                                          static_cast<uint8_t>(b));
}
constexpr inline operating_condition operator|(operating_condition a,
                                               operating_condition b) noexcept {
  return static_cast<operating_condition>(static_cast<uint8_t>(a) |
                                          static_cast<uint8_t>(b));
}

// TODO EMVco mode
class atr {
  std::vector<std::byte> bytes_;
  std::vector<std::byte> historical_bytes_;
  int Fi_;
  int FMax_;
  int Di_;

public:
  using duration = std::chrono::duration<double, std::ratio<1>>;

  explicit atr(std::vector<std::byte> bytes);

  const std::vector<std::byte> &bytes() const { return bytes_; }
  std::optional<std::byte> intf_char(if_char c, int idx) const noexcept;
  std::optional<std::byte> first(if_char c, int T) const noexcept;

  bool T_present(int i) const noexcept;
  const std::vector<std::byte> &historical_bytes() const noexcept;

  int Fi() const noexcept;
  int FMax() const noexcept;
  int Di() const noexcept;

  uint8_t N() const noexcept;
  duration gt(int F, int D, int freq) const noexcept;

  // TODO optional?
  bool specific_mode() const noexcept;
  int specific_mode_T() const noexcept;
  bool specific_change_capable() const noexcept;
  bool implicit_divider() const noexcept;

  clockstop_indicator clockstop() const noexcept;
  operating_condition classes() const noexcept;

  duration wt(int freq) const noexcept;

  std::size_t ifsc() const noexcept;
  duration cgt(int F, int D, int freq) const noexcept;
  duration bgt(int F, int D, int freq) const noexcept;
  duration cwt(int F, int D, int freq) const noexcept;
  duration bwt(int F, int D, int freq) const noexcept;
  redundancy_code code() const noexcept;

private:
  constexpr std::size_t offset(std::byte tdx, if_char c) const noexcept;
  constexpr double etu(int F, int D, int freq) const noexcept;
};

std::vector<std::byte>
receive(std::function<bool(gsl::span<std::byte> buffer)> recv_func);

bool iterate(gsl::span<const std::byte> &atr,
             std::function<void(if_char, std::size_t, std::byte)> func);
bool iterate(
    gsl::span<const std::byte> &atr,
    std::function<void(if_char, std::size_t, std::byte)> global,
    std::function<void(std::byte T, if_char c, std::size_t n, std::byte b)>
        specific);

} // namespace atr

#endif
