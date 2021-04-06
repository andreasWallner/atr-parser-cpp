#include "atr.hpp"
#include "helper.hpp"

#include <catch2/catch_all.hpp>

using namespace Catch::Matchers;

TEST_CASE("minimal T=0") {
  atr::atr atr("3b00"_h2b);

  REQUIRE(atr.Fi() == 372);
  REQUIRE(atr.FMax() == 5'000'000);
  REQUIRE(atr.Di() == 1);
  REQUIRE(atr.gt(372, 1, 5'000'000).count() ==
          (12.0 * 372.0 / 1.0 / 5'000'000.0));
  REQUIRE(atr.specific_mode() == false);
  REQUIRE(atr.specific_change_capable() == false);
  REQUIRE(atr.implicit_divider() == false);
  REQUIRE(atr.clockstop() == atr::clockstop_indicator::not_supported);
  REQUIRE(atr.classes() == atr::operating_condition::A);
  REQUIRE(atr.wt(5'000'000.0).count() == (10.0 * 960.0 * 372.0 / 5'000'000.0));
  REQUIRE(atr.historical_bytes() == ""_h2b);
}

TEST_CASE("max T=0 (w/o T=15)") {
  atr::atr atr("3BFF A5BB1160 BB05 010203040506070809101112131415"_h2b);

  REQUIRE(atr.Fi() == 768);
  REQUIRE(atr.FMax() == 7'500'000);
  REQUIRE(atr.Di() == 16);
  REQUIRE(atr.gt(512, 4, 1'234'567).count() ==
          ((12.0 * 512 / 4 / 1'234'567) + (1.0 * 0x11 * 768 / 16 / 1'234'567)));
  REQUIRE(atr.specific_mode() == false);
  REQUIRE(atr.clockstop() == atr::clockstop_indicator::not_supported);
  REQUIRE(atr.classes() == atr::operating_condition::A);
  REQUIRE(atr.wt(1'234'567).count() == (5.0 * 960 * 768 / 1'234'567));
  REQUIRE(atr.historical_bytes() == "010203040506070809101112131415"_h2b);
}

TEST_CASE("minimal T=1") {
  atr::atr atr("3b01 01"_h2b);

  const auto etu = 372.0 / 1.0 / 5'000'000.0;

  REQUIRE(atr.Fi() == 372);
  REQUIRE(atr.FMax() == 5'000'000);
  REQUIRE(atr.Di() == 1);
  REQUIRE(atr.gt(372, 1, 5'000'000).count() == (12.0 * etu));
  REQUIRE(atr.specific_mode() == false);
  REQUIRE(atr.specific_change_capable() == false);
  REQUIRE(atr.implicit_divider() == false);
  REQUIRE(atr.clockstop() == atr::clockstop_indicator::not_supported);
  REQUIRE(atr.classes() == atr::operating_condition::A);

  REQUIRE(atr.ifsc() == 32);

  REQUIRE(atr.cgt(372, 1, 5'000'000).count() == (12.0 * etu));
  REQUIRE(atr.bgt(372, 1, 5'000'000).count() == (22.0 * etu));
  REQUIRE(atr.cwt(372, 1, 5'000'000).count() ==
          ((11.0 + std::pow(2, 13)) * etu));
  REQUIRE(atr.bwt(372, 1, 5'000'000).count() ==
          (11.0 * etu + std::pow(2, 4) * 960 * 372 / 5'000'000.0));
  REQUIRE(atr.code() == atr::redundancy_code::LRC);
}

TEST_CASE("max T=1 (w/o T=15)") {
  atr::atr atr("3BFF 11BB0081 71 EF1200 151413121110090807060504030201 58"_h2b);

  REQUIRE(atr.Fi() == 372);
  REQUIRE(atr.FMax() == 5'000'000);
  REQUIRE(atr.Di() == 1);
  REQUIRE(atr.gt(372, 1, 3'760'000).count() == (12.0 * 372 / 1 / 3'760'000));
  REQUIRE(atr.specific_mode() == false);
  REQUIRE(atr.clockstop() == atr::clockstop_indicator::not_supported);
  REQUIRE(atr.classes() == atr::operating_condition::A);

  REQUIRE(atr.ifsc() == 0xef);

  REQUIRE(atr.cgt(372, 1, 2'500'000).count() == (12.0 * 372 / 1 / 2'500'000));
  REQUIRE(atr.bgt(372, 1, 2'500'000).count() == (22.0 * 372 / 1 / 2'500'000));
  REQUIRE(atr.cwt(372, 1, 2'500'000).count() ==
          ((11.0 + std::pow(2, 2)) * 372 / 1 / 2'500'000));
  REQUIRE(
      atr.bwt(372, 1, 2'500'000).count() ==
      (11.0 * 372 / 1 / 2'500'000 + std::pow(2, 1) * 960 * 372 / 2'500'000.0));
  REQUIRE(atr.code() == atr::redundancy_code::LRC);
  REQUIRE(atr.historical_bytes() == "151413121110090807060504030201"_h2b);
}

TEST_CASE("all settings, negotiable") {
  atr::atr atr(
      "3bff 34ffafe0 ff20F1 ef23011f 87 112233445566778899aabbccddeeff 00"_h2b);
  //        A B C D  B C D  A B C D  A
  //        1        2      3        4

  REQUIRE(atr.Fi() == 744);
  REQUIRE(atr.FMax() == 8'000'000);
  REQUIRE(atr.Di() == 8);

  REQUIRE(atr.N() == 0xaf);
  REQUIRE_THAT(
      atr.gt(558, 2, 7'000'000).count(),
      WithinRel((12.0 * 558 / 2 / 7'000'000 + (744.0 / 8) * 0xaf / 7'000'000)));
  REQUIRE(atr.specific_mode() == false);
  REQUIRE(atr.specific_change_capable() == false);
  REQUIRE(atr.implicit_divider() == false);
  REQUIRE(atr.clockstop() == atr::clockstop_indicator::high);
  REQUIRE(atr.classes() ==
          (atr::operating_condition::A | atr::operating_condition::B |
           atr::operating_condition::C));

  REQUIRE_THAT(atr.wt(7'000'000).count(),
               WithinRel(1.0 * 0x20 * 960 * 744 / 7'000'000));

  REQUIRE(atr.ifsc() == 0xef);
  REQUIRE_THAT(
      atr.cgt(372, 2, 7'000'000).count(),
      WithinRel(12.0 * 372 / 2 / 7'000'000 + 175.0 * 744 / 8 / 7'000'000));
  REQUIRE_THAT(atr.bgt(372, 2, 7'000'000).count(),
               WithinRel(22.0 * 372 / 2 / 7'000'000));
  REQUIRE_THAT(atr.cwt(372, 2, 7'000'000).count(),
               WithinRel((11 + std::pow(2, 0x03)) * (372.0 / 2.0 / 7'000'000)));
  REQUIRE_THAT(atr.bwt(372, 2, 7'000'000).count(),
               WithinRel(11.0 * 372 / 2 / 7'000'000 +
                         std::pow(2, 0x02) * 960 * 372 / 7'000'000));
  REQUIRE(atr.code() == atr::redundancy_code::CRC);
}

TEST_CASE("TA1") {
  SECTION("valid, min") {
    atr::atr atr("3B10 01"_h2b);
    REQUIRE(atr.Fi() == 372);
    REQUIRE(atr.FMax() == 4'000'000);
    REQUIRE(atr.Di() == 1);
  }
  SECTION("valid, max") {
    atr::atr atr("3B10 D9"_h2b);
    REQUIRE(atr.Fi() == 2048);
    REQUIRE(atr.FMax() == 20'000'000);
    REQUIRE(atr.Di() == 20);
  }
  SECTION("invalid, RFU") {
    REQUIRE_THROWS(atr::atr("3B10 71"_h2b));
    REQUIRE_THROWS(atr::atr("3B10 00"_h2b));
  }
}

TEST_CASE("TA2") {
  SECTION("absent, defaults") {
    atr::atr atr("3B00"_h2b);
    REQUIRE(atr.specific_mode() == false);
    REQUIRE(atr.specific_change_capable() == false);
    REQUIRE(atr.implicit_divider() == false);
  }
  SECTION("all off") {
    atr::atr atr("3B80 10 00"_h2b);
    REQUIRE(atr.specific_mode() == true);
    REQUIRE(atr.specific_change_capable() == false);
    REQUIRE(atr.implicit_divider() == false);
    REQUIRE(atr.specific_mode_T() == 0);
  }
  SECTION("w specific mode changable") {
    atr::atr atr("3B80 10 80"_h2b);
    REQUIRE(atr.specific_mode() == true);
    REQUIRE(atr.specific_change_capable() == true);
    REQUIRE(atr.implicit_divider() == false);
    REQUIRE(atr.specific_mode_T() == 0);
  }
  SECTION("w implicit divider") {
    atr::atr atr("3B80 10 10"_h2b);
    REQUIRE(atr.specific_mode() == true);
    REQUIRE(atr.specific_change_capable() == false);
    REQUIRE(atr.implicit_divider() == true);
    REQUIRE(atr.specific_mode_T() == 0);
  }
  SECTION("T=1 specific mode") {
    atr::atr atr("3B80 10 01"_h2b);
    REQUIRE(atr.specific_mode() == true);
    REQUIRE(atr.specific_change_capable() == false);
    REQUIRE(atr.implicit_divider() == false);
    REQUIRE(atr.specific_mode_T() == 1);
  }
  SECTION("invalid, RFU") {
    REQUIRE_THROWS(atr::atr("3B80 10 40"_h2b));
    REQUIRE_THROWS(atr::atr("3B80 10 20"_h2b));
  }
}

TEST_CASE("TC1") {
  SECTION("absent, defaults") {
    // default N = 0
    atr::atr atr("3B00"_h2b);
    REQUIRE_THAT(atr.gt(558, 2, 7'000'000).count(),
                 WithinRel((12.0 * 558 / 2 / 7'000'000)));
    REQUIRE_THAT(atr.cgt(372, 1, 5'000'000).count(),
                 WithinRel((12.0 * 372 / 1 / 5'000'000)));
  }
  SECTION("no T=15") {
    // calculations must just use ETU
    atr::atr atr("3B40 20"_h2b);
    REQUIRE_THAT(atr.gt(558, 2, 7'000'000).count(),
                 WithinRel(((12.0 + 0x20) * 558 / 2 / 7'000'000)));
    REQUIRE_THAT(atr.cgt(372, 1, 5'000'000).count(),
                 WithinRel(((12.0 + 0x20) * 372 / 1 / 5'000'000)));
  }
  SECTION("with T=15") {
    // calculation split between ETU & indicated ETU
    atr::atr atr("3BC0 210F EE"_h2b);
    REQUIRE_THAT(atr.gt(558, 2, 5'000'000).count(),
                 WithinRel((12.0 * 558 / 2 / 5'000'000 +
                            (372.0 / 1) * 0x21 / 5'000'000)));
    REQUIRE_THAT(atr.cgt(558, 4, 5'000'000).count(),
                 WithinRel((12.0 * 558 / 4 / 5'000'000 +
                            (372.0 / 1) * 0x21 / 5'000'000)));
  }
  SECTION("with T=15 & TA1") {
    // calculation split between ETU & indicated ETU (must use TA1)
    atr::atr atr("3BD0 D9220F 24"_h2b);
    REQUIRE_THAT(atr.gt(558, 2, 5'000'000).count(),
                 WithinRel((12.0 * 558 / 2 / 5'000'000 +
                            (2048.0 / 20) * 0x22 / 5'000'000)));
    REQUIRE_THAT(atr.cgt(372, 1, 5'000'000).count(),
                 WithinRel((12.0 * 372 / 1 / 5'000'000 +
                            (2048.0 / 20) * 0x22 / 5'000'000)));
  }
  SECTION("N = 255") {
    // special cases for T=0 & T=1
    atr::atr atr("3B40 FF"_h2b);
    REQUIRE_THAT(atr.gt(558, 2, 7'000'000).count(),
                 WithinRel((12.0 * 558 / 2 / 7'000'000)));
    REQUIRE_THAT(atr.cgt(372, 1, 5'000'000).count(),
                 WithinRel((11.0 * 372 / 1 / 5'000'000)));
  }
  SECTION("N = 255 with T=15 & TA1") {
    // special cases must ignore T=15 & TA1
    atr::atr atr("3BD0 D9FF0F F9"_h2b);
    REQUIRE_THAT(atr.gt(558, 2, 7'000'000).count(),
                 WithinRel((12.0 * 558 / 2 / 7'000'000)));
    REQUIRE_THAT(atr.cgt(372, 1, 5'000'000).count(),
                 WithinRel((11.0 * 372 / 1 / 5'000'000)));
  }
}

TEST_CASE("first TA for T=15") {
  SECTION("absent, defaults") {
    atr::atr atr("3B00"_h2b);
    REQUIRE(atr.clockstop() == atr::clockstop_indicator::not_supported);
    REQUIRE(atr.classes() == atr::operating_condition::A);
  }
  SECTION("with clockstop") {
    atr::atr atr("3B80 80 1F 41 5E"_h2b);
    REQUIRE(atr.clockstop() == atr::clockstop_indicator::low);
    REQUIRE(atr.classes() == atr::operating_condition::A);
  }
  SECTION("with class C") {
    atr::atr atr("3B80 80 1F 04 1B"_h2b);
    REQUIRE(atr.clockstop() == atr::clockstop_indicator::not_supported);
    REQUIRE(atr.classes() == atr::operating_condition::C);
  }
  SECTION("with all classes") {
    atr::atr atr("3B80 80 1F 07 18"_h2b);
    REQUIRE(atr.clockstop() == atr::clockstop_indicator::not_supported);
    REQUIRE(atr.classes() ==
            (atr::operating_condition::A | atr::operating_condition::B |
             atr::operating_condition::C));
  }
  SECTION("invalid class, RFU") {
    REQUIRE_THROWS(atr::atr("3B80 80 1F 00 1F"_h2b));
    REQUIRE_THROWS(atr::atr("3B80 80 1F 0F 10"_h2b));
  }
}

TEST_CASE("TC2") {
  SECTION("absent, defaults") {
    atr::atr atr("3B00"_h2b);
    REQUIRE_THAT(atr.wt(5'000'000.0).count(),
                 WithinRel(10.0 * 960.0 * 372.0 / 5'000'000.0));
  }
  SECTION("preset") {
    // must use provided value for WT calculation
    atr::atr atr("3B80 40 55"_h2b);
    REQUIRE_THAT(atr.wt(5'000'000.0).count(),
                 WithinRel(1.0 * 0x55 * 960.0 * 372.0 / 5'000'000.0));
  }
  SECTION("present & TA1") {
    // must use provided Fi for calculation
    atr::atr atr("3B90 D940 55"_h2b);
    REQUIRE_THAT(atr.wt(5'000'000.0).count(),
                 WithinRel(1.0 * 0x55 * 960.0 * 2048.0 / 5'000'000.0));
  }
  SECTION("invalid, RFU") { REQUIRE_THROWS(atr::atr("3B80 40 00"_h2b)); }
}

TEST_CASE("first TA T=1") {
  SECTION("present") {
    atr::atr atr("3B80 80 11 44 55"_h2b);
    REQUIRE(atr.ifsc() == 0x44);
  }
  SECTION("invalid, RFU") {
    REQUIRE_THROWS(atr::atr("3B80 80 11 00 11"_h2b));
    REQUIRE_THROWS(atr::atr("3B80 80 11 FF EE"_h2b));
  }
}

TEST_CASE("first TB T=1") {
  SECTION("present") {
    atr::atr atr("3B80 80 21 54 75"_h2b);
    REQUIRE_THAT(
        atr.cwt(372, 2, 7'000'000).count(),
        WithinRel((11 + std::pow(2, 0x04)) * (372.0 / 2.0 / 7'000'000)));
    REQUIRE_THAT(atr.bwt(372, 2, 7'000'000).count(),
                 WithinRel(11.0 * 372 / 2 / 7'000'000 +
                           std::pow(2, 0x05) * 960 * 372 / 7'000'000));
  }
  SECTION("w TA1") {
    // must ignore TA1
    atr::atr atr("3B90 D980 21 54 BC"_h2b);
    REQUIRE_THAT(
        atr.cwt(372, 2, 7'000'000).count(),
        WithinRel((11 + std::pow(2, 0x04)) * (372.0 / 2.0 / 7'000'000)));
    REQUIRE_THAT(atr.bwt(372, 2, 7'000'000).count(),
                 WithinRel(11.0 * 372 / 2 / 7'000'000 +
                           std::pow(2, 0x05) * 960 * 372 / 7'000'000));
  }
  SECTION("using Fd") {
    // bwt must use Fd in calculation
    atr::atr atr("3B80 80 21 54 75"_h2b);
    REQUIRE_THAT(atr.bwt(558, 2, 7'000'000).count(),
                 WithinRel(11.0 * 558 / 2 / 7'000'000 +
                           std::pow(2, 0x05) * 960 * 372 / 7'000'000));
  }
  SECTION("invalid, RFU") {
    REQUIRE_THROWS(atr::atr("3B80 80 21 A4 85"_h2b));
    REQUIRE_THROWS(atr::atr("3B80 80 21 F4 d5"_h2b));
  }
}

TEST_CASE("first TC T=1") {
  SECTION("present") {
    atr::atr atr("3B80 80 41 01 40"_h2b);
    REQUIRE(atr.code() == atr::redundancy_code::CRC);
  }
  SECTION("invalid, RFU") {
    REQUIRE_THROWS(atr::atr("3B80 80 41 02 43"_h2b));
    REQUIRE_THROWS(atr::atr("3B80 80 41 81 C0"_h2b));
  }
}

TEST_CASE("historical bytes") {
  SECTION("absent") {
    atr::atr atr("3B00"_h2b);
    REQUIRE(atr.historical_bytes() == ""_h2b);
  }
  SECTION("short") {
    atr::atr atr("3B01 DE"_h2b);
    REQUIRE(atr.historical_bytes() == "DE"_h2b);
  }
  SECTION("long") {
    atr::atr atr("3B0F 112233445566778899AABBCCDDEEFF"_h2b);
    REQUIRE(atr.historical_bytes() == "112233445566778899AABBCCDDEEFF"_h2b);
  }
  SECTION("invalid, too short") {
    REQUIRE_THROWS(atr::atr("3B0F 112233445566778899AABBCCDDEE"_h2b));
  }
}

TEST_CASE("TCK") {
  SECTION("T=0, TCK must be absent") {
    REQUIRE_THROWS(atr::atr("3B00 00"_h2b));
  }
  SECTION("others, TCK must be correct") {
    REQUIRE_THROWS(atr::atr("3B80 01 00"_h2b));
    REQUIRE_THROWS(atr::atr("3B80 0F 00"_h2b));
  }
}

#include <gsl/span>
#include <iomanip>
#include <ios>
#include <iostream>

TEST_CASE("foo") {
  const auto atr = "3B80 80 1F 41"_h2b;
  auto buffer = gsl::span(atr);
  atr::iterate(
      buffer,
      [](atr::if_char c, std::size_t i, std::byte b) {
        std::cout << "global: " << to_char(c) << i << ": " << std::hex
                  << std::setfill('0') << std::setw(2) << static_cast<int>(b)
                  << "\n";
      },
      [](std::byte T, atr::if_char c, std::size_t n, std::byte b) {
        std::cout << "specific: " << n + 1 << " T" << to_char(c)
                  << " for T=" << static_cast<int>(T) << ": " << std::hex
                  << std::setfill('0') << std::setw(2) << static_cast<int>(b)
                  << "\n";
      });
  // REQUIRE(true == false);
}