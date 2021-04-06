#include "atr.hpp"

#include <map>
#include <numeric>
#include <string>

#include "utility.hpp"

namespace atr {
namespace {
int Fi_lookup[] = {372, 372, 558, 744,  1116, 1488, 1860, 0,
                   0,   512, 768, 1024, 1536, 2048, 0,    0};
int FMax_lookup[] = {4'000'000,  5'000'000,  6'000'000,  8'000'000,
                     12'000'000, 16'000'000, 20'000'000, 0,
                     0,          5'000'000,  7'500'000,  10'000'000,
                     15'000'000, 20'000'000, 0,          0};
int Di_lookup[] = {0, 1, 2, 4, 8, 16, 32, 64, 12, 20, 0, 0, 0, 0, 0, 0};
} // namespace

atr::atr(std::vector<std::byte> bytes) : bytes_(std::move(bytes)) {
  gsl::span<const std::byte> buffer(bytes_);
  bool tck_present = false;
  bool valid = iterate(
      buffer,
      [&](if_char c, std::size_t i, std::byte b) {
        if (c == if_char::D && (b & 0x0f_b) != 0_b)
          tck_present = true;

        switch (i) {
        case 0:
          switch (c) {
          case if_char::A: // TA1
            if (Fi() == 0)
              throw invalid_atr("invalid Fi");
            if (Di() == 0)
              throw invalid_atr("invalid Di");
            break;
          case if_char::B: // TB1 -> deprecated, ignore
          case if_char::C: // TC1 -> all valid
          case if_char::D: // TD1
            break;
          }
          break;
        case 1:
          switch (c) {
          case if_char::A: // TA2
            if ((b & 0x60_b) != 0x0_b)
              throw invalid_atr("TA2 RFU bits set");
            break;
          case if_char::B: // TB2
            break;
          case if_char::C: // TC2
            if (b == 0_b)
              throw invalid_atr("invalid TC2/WI");
            break;
          case if_char::D:
            break;
          }
        default:
          break;
        }
      },
      [&](std::byte T, if_char c, std::size_t n, std::byte b) {
        if (n > 0)
          return;
        switch (T) {
        case 1_b:
          switch (c) {
          case if_char::A: // TA T=1
            if (b == 0_b || b == 0xff_b)
              throw invalid_atr("invalid TA for T=1 (IFSC)");
            break;
          case if_char::B: // TB T=1
            if ((b & 0xf0_b) > 0x90_b)
              throw invalid_atr("invalid TB for T=1, BWI too big");
            break;
          case if_char::C: // TC T=1
            if ((b & 0xFE_b) != 0_b)
              throw invalid_atr("invalid TC for T=1");
            break;
          case if_char::D:
            break;
          }
          break;
        case 15_b:
          switch (c) {
          case if_char::A:
            if ((b & 0b00111000_b) != 0_b || (b & 0x7_b) == 0_b)
              throw invalid_atr("invalid classes of operating conditions");
          case if_char::B:
            break;
          default:
            break;
          }
        default:
          break;
        }
      });

  if (!valid)
    throw invalid_atr("structural bytes seem invalid");

  auto K = static_cast<std::size_t>(bytes_[1] & 0x0f_b);
  if (buffer.size() < K)
    throw invalid_atr("not enough bytes for stated historical byte length");
  historical_bytes_ =
      std::vector<std::byte>(buffer.begin(), buffer.begin() + K);
  buffer = buffer.subspan(K);

  if (tck_present) {
    if (buffer.size() < 1)
      throw invalid_atr("necessary TCK absent");
    auto check =
        std::accumulate(bytes_.cbegin() + 1, bytes_.cend(), 0_b,
                        [](std::byte a, std::byte b) { return a ^ b; });
    if (check != 0_b)
      throw invalid_atr(std::string("invalid TCK ") +
                        std::to_string(static_cast<int>(check)));
    buffer = buffer.subspan(1);
  }

  if (buffer.size())
    throw invalid_atr("too many bytes in ATR");
}

std::optional<std::byte> atr::intf_char(if_char c, int idx) const noexcept {
  if (idx <= 0)
    return {};

  std::size_t tdx_offset = 1;
  while (--idx) {
    std::byte tdx = bytes_[tdx_offset];
    if ((tdx & static_cast<std::byte>(if_char::D)) == 0_b)
      return {};
    tdx_offset += offset(tdx, if_char::D);

    if (tdx_offset > bytes_.size())
      return {};
  }

  if ((bytes_[tdx_offset] & static_cast<std::byte>(c)) == 0_b)
    return {};

  std::size_t txx_offset = tdx_offset + offset(bytes_[tdx_offset], c);
  if (txx_offset > bytes_.size())
    return {};
  return bytes_[txx_offset];
}

std::optional<std::byte> atr::first(if_char c, int T) const noexcept {
  std::size_t tdx_offset = 1;
  int block = 1;
  while (true) {
    std::byte tdx = bytes_[tdx_offset];
    if (block > 2 && (static_cast<int>(tdx & 0x0f_b) == T) &&
        ((tdx & static_cast<std::byte>(c)) != 0_b)) {
      std::size_t txx_offset = tdx_offset + offset(tdx, c);
      if (txx_offset > bytes_.size())
        return {}; // TODO not really needed?
      return bytes_[txx_offset];
    }

    if ((tdx & static_cast<std::byte>(if_char::D)) == 0_b)
      return {};
    tdx_offset += offset(tdx, if_char::D);
    block++;

    if (tdx_offset > bytes_.size())
      return {};
  };
}

bool atr::T_present(int i) const noexcept {
  if (i < 0)
    return false;

  std::size_t tdx_offset = 1;
  while (true) {
    std::byte tdx = bytes_[tdx_offset];
    if ((tdx & 0x0f_b) == std::byte(i))
      return true;
    if ((tdx & static_cast<std::byte>(if_char::D)) == 0_b)
      return false;
    tdx_offset += offset(tdx, if_char::D);

    if (tdx_offset > bytes_.size())
      return false;
  }
}

const std::vector<std::byte> &atr::historical_bytes() const noexcept {
  return historical_bytes_;
}

constexpr std::byte default_TA1 = 0x11_b;

int atr::Fi() const noexcept {
  auto TA1 = intf_char(if_char::A, 1).value_or(default_TA1);
  return Fi_lookup[std::to_integer<std::size_t>(TA1 >> 4)];
}

int atr::FMax() const noexcept {
  auto TA1 = intf_char(if_char::A, 1).value_or(default_TA1);
  return FMax_lookup[std::to_integer<std::size_t>(TA1 >> 4)];
}

int atr::Di() const noexcept {
  auto TA1 = intf_char(if_char::A, 1).value_or(default_TA1);
  return Di_lookup[std::to_integer<std::size_t>(TA1 & 0x0f_b)];
}

uint8_t atr::N() const noexcept {
  const auto TC1 = intf_char(if_char::C, 1).value_or(0x00_b);
  return (TC1 != 255_b) ? std::to_integer<int>(TC1) : 0;
}

atr::duration atr::gt(int F, int D, int freq) const noexcept {
  // see ISO7816-3:2006, 8.3 Global interface bytes, TC1, p. 19
  // since the calcuation (possibly) mixes ETUs and "indicated ETUs", return
  // result as time
  // NOTE: GT is only used for PPS & T=0, T=1 uses CGT & BGT
  const auto TC1 = intf_char(if_char::C, 1).value_or(0x00_b);
  const auto N = (TC1 != 255_b) ? std::to_integer<int>(TC1) : 0;
  const double actual_etu = etu(F, D, freq);
  const double indicated_etu = etu(Fi(), Di(), freq);
  const double base_etu = T_present(15) ? indicated_etu : actual_etu;
  return duration(12 * actual_etu + N * base_etu);
}

bool atr::specific_mode() const noexcept {
  const auto TA2 = intf_char(if_char::A, 2);
  return bool(TA2);
}

int atr::specific_mode_T() const noexcept {
  const auto TA2 = intf_char(if_char::A, 2).value_or(0x00_b);
  return static_cast<int>(TA2 & 0x0f_b);
}

bool atr::specific_change_capable() const noexcept {
  const auto TA2 = intf_char(if_char::A, 2).value_or(0x00_b);
  return (TA2 & 0x80_b) != 0_b;
}

bool atr::implicit_divider() const noexcept {
  const auto TA2 = intf_char(if_char::A, 2).value_or(0x00_b);
  return (TA2 & 0x10_b) != 0_b;
}

clockstop_indicator atr::clockstop() const noexcept {
  auto TA = first(if_char::A, 15).value_or(0x01_b);
  return static_cast<clockstop_indicator>(TA >> 6);
}

operating_condition atr::classes() const noexcept {
  auto TA = first(if_char::A, 15).value_or(0x01_b);
  return static_cast<operating_condition>(TA & 0x07_b);
}

atr::duration atr::wt(int freq) const noexcept {
  const auto TC2 = intf_char(if_char::C, 2).value_or(10_b);
  const auto WI = static_cast<double>(TC2);
  const auto Fi = static_cast<double>(this->Fi());
  return duration(WI * 960.0 * Fi / freq);
}

std::size_t atr::ifsc() const noexcept {
  // ISO7816-3:2006, 11.4.2 Information field sizes, p. 27
  return static_cast<std::size_t>(first(if_char::A, 1).value_or(32_b));
}

atr::duration atr::cgt(int F, int D, int freq) const noexcept {
  // ISO7816-3:2006, 11.2 Character frame, p. 24
  const auto N = intf_char(if_char::C, 1).value_or(0x00_b);
  return (N != 255_b) ? gt(F, D, freq) : duration{11.0 * etu(F, D, freq)};
}

atr::duration atr::bgt(int F, int D, int freq) const noexcept {
  return duration{22.0 * etu(F, D, freq)};
}

atr::duration atr::cwt(int F, int D, int freq) const noexcept {
  auto TB = first(if_char::B, 1).value_or(0x4D_b);
  const auto CWI = static_cast<int>(TB & 0x0f_b);
  const auto etus = 11.0 + static_cast<double>(1 << CWI);
  return duration{etus * etu(F, D, freq)};
}

atr::duration atr::bwt(int F, int D, int freq) const noexcept {
  auto TB = first(if_char::B, 1).value_or(0x4D_b);
  const auto BWI = static_cast<int>((TB >> 4) & 0x0f_b);
  const auto additional = static_cast<double>(1 << BWI) * 960.0 * 372 / freq;
  return duration{11.0 * etu(F, D, freq) + additional};
}

redundancy_code atr::code() const noexcept {
  auto TC = first(if_char::C, 1).value_or(0x00_b);
  return (TC & 0x01_b) == 0_b ? redundancy_code::LRC : redundancy_code::CRC;
}

constexpr std::size_t atr::offset(std::byte tdx, if_char c) const noexcept {
  std::byte offset_mask = [c]() {
    switch (c) {
    case if_char::A:
      return 0_b;
    case if_char::B:
      return (std::byte)if_char::A;
    case if_char::C:
      return (std::byte)if_char::A | (std::byte)if_char::B;
    case if_char::D:
      return (std::byte)if_char::A | (std::byte)if_char::B |
             (std::byte)if_char::C;
    }
  }();
  return popcount(tdx & offset_mask) + 1;
};

constexpr double atr::etu(int F, int D, int freq) const noexcept {
  return static_cast<double>(F) / D / freq;
}

bool iterate(gsl::span<const std::byte> &atr,
             std::function<void(if_char, std::size_t, std::byte)> func) {
  if (atr.size() < 2)
    return false;
  atr = atr.subspan(1);
  std::size_t i = 0;
  while (true) {
    std::byte td = atr[0];
    for (auto c : {if_char::A, if_char::B, if_char::C, if_char::D}) {
      if ((td & static_cast<std::byte>(c)) == 0_b)
        continue;
      if (atr.size() < 1)
        return false;
      atr = atr.subspan(1);
      func(c, i, atr[0]);
    }
    if ((td & static_cast<std::byte>(if_char::D)) == 0_b) {
      atr = atr.subspan(1);
      return true;
    }
    i++;
  }
}

bool iterate(
    gsl::span<const std::byte> &atr,
    std::function<void(if_char, std::size_t, std::byte)> global,
    std::function<void(std::byte T, if_char c, std::size_t n, std::byte b)>
        specific) {
  std::map<std::byte, std::size_t> cnts;
  std::byte T = 0_b;
  return iterate(atr, [&](auto c, auto i, auto b) {
    if (c == if_char::D)
      T = b & 0x0f_b;
    if (c == if_char::D || i < 2) {
      global(c, i, b);
    } else {
      std::byte id = static_cast<std::byte>(c) | T;
      auto cnt = cnts[id]++;
      specific(T, c, cnt, b);
    }
  });
}

} // namespace atr