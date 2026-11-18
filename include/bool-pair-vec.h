#pragma once

#include <vector>

class BoolPairVec {
  std::vector<std::uint8_t> data;

 public:
  explicit BoolPairVec(std::size_t size) : data(size, 0) {}

  [[gnu::always_inline]] bool isFirst(std::size_t index) const {
    return data[index] & 1;
  }

  [[gnu::always_inline]] bool isSecond(std::size_t index) const {
    return (data[index] >> 1) & 1;
  }

  [[gnu::always_inline]] void setFirst(std::size_t index) { data[index] |= 1; }
  [[gnu::always_inline]] void setSecond(std::size_t index) { data[index] |= 2; }

  [[gnu::always_inline]] void set(std::size_t index, bool isSeparator,
                                  bool visited) {
    data[index] = (std::uint8_t)0 | ((std::uint8_t)isSeparator & 1) << 1 |
                  (std::uint8_t)visited & 1;
  }

  [[gnu::always_inline]] std::pair<bool, bool> get(std::size_t index) const {
    return {isFirst(index), isSecond(index)};
  }
};
