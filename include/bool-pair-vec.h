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

  [[gnu::always_inline]] std::pair<bool, bool> get(std::size_t index) const {
    return {isFirst(index), isSecond(index)};
  }
};
