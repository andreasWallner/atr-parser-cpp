

#include "atr.hpp"

#include "helper.hpp"

#include "catch2/catch_all.hpp"

#include <algorithm>
#include <gsl/gsl_algorithm>
#include <gsl/span_ext>

struct fake_sender {
  const std::vector<std::byte> data_;
  const bool fail_on_underflow_;
  gsl::span<std::byte const> remaining_;

  fake_sender(std::vector<std::byte> to_send, bool fail_on_underflow = true)
      : data_(to_send), fail_on_underflow_(fail_on_underflow),
        remaining_(data_) {}
  bool operator()(gsl::span<std::byte> buffer) {
    if (buffer.size() > remaining_.size()) {
      if (fail_on_underflow_)
        FAIL("too many bytes read");
      return false;
    }
    copy(remaining_.first(buffer.size()), buffer);
    remaining_ = remaining_.subspan(buffer.size());
    return true;
  }
};

TEST_CASE("valid ATRs") {
  auto atr =
      GENERATE("3b00"_h2b,                                // minimal ATR
               "3b01 11"_h2b,                             // + 1 historical byte
               "3b0f 112233445566778899aabbccddeeff"_h2b, // + 15
                                                          // historical bytes
               "3b10 23"_h2b,                             // + TA1
               "3bF0 6677880f AA"_h2b,                    // + TA1:TD1 + TCK
               "3b80 F0 66778800"_h2b                     // + TD1 + TA2:TD2
      );
  CAPTURE(atr);
  fake_sender sender{atr};
  REQUIRE(atr::receive(sender) == atr);
}

TEST_CASE("invalid ATRs") {
  auto atr = GENERATE(""_h2b, "ff"_h2b, "3b10"_h2b,
                      "3bF0 112233F4 112233F4 112233F4 112233F4 112233F4 "
                      "112233F4 112233F4 112233F4"_h2b);
  CAPTURE(atr);
  fake_sender sender{atr, false};
  REQUIRE(atr::receive(sender) == ""_h2b);
}