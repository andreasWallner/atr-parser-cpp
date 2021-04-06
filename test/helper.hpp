#ifndef header_test_helper_
#define header_test_helper_

#include <cstddef>
#include <vector>

static std::vector<std::byte> operator""_h2b(const char *chars,
                                             std::size_t size) {
  auto str = std::string_view(chars, size);

  constexpr char space[] = {' ', '\t', '\r', '\n'};
  std::vector<std::byte> result;
  bool first = true;
  std::byte b;

  size_t idx = 0;
  for (char c : str) {
    using namespace std::literals::string_literals;
    std::byte nibble;
    if (std::find(std::cbegin(space), std::cend(space), c) != std::cend(space))
      continue;

    if (c >= '0' && c <= '9')
      nibble = static_cast<std::byte>(c - '0');
    else if (c >= 'a' && c <= 'f')
      nibble = static_cast<std::byte>(c - 'a' + 10);
    else if (c >= 'A' && c <= 'F')
      nibble = static_cast<std::byte>(c - 'A' + 10);
    else
      throw std::runtime_error("invalid character in hex string: "s +
                               std::string(str.substr(idx, idx + 1)) +
                               " at index "s + std::to_string(idx));

    if (first)
      b = nibble << 4;
    else
      result.push_back(b | nibble);

    first = !first;
    idx++;
  }

  if (!first)
    throw std::runtime_error("half hex byte found in hex string");

  return result;
}

#endif