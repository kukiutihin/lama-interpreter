#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "app-runtime.h"

class ByteReader {
  const std::uint8_t* data_;
  std::size_t size_;
  std::size_t pos_;

  template <typename T>
  [[gnu::always_inline]] T read() {
    if (pos_ + sizeof(T) > size_) {
      failure(
          "attempt to read outside code boundaries, current position: %zu, "
          "code size: %zu, bytes to read: %zu\n",
          pos_, size_, sizeof(T));
    }

    T val;
    std::memcpy(&val, data_ + pos_, sizeof(T));
    pos_ += sizeof(T);
    return val;
  }

 public:
  ByteReader(const std::uint8_t* data, std::size_t size)
      : data_(data), size_(size), pos_(0) {}

  [[gnu::always_inline]] std::int32_t i32() { return read<std::int32_t>(); }

  [[gnu::always_inline]] std::uint8_t u8() { return read<std::uint8_t>(); }

  [[gnu::always_inline]] void skip(std::size_t n) {
    pos_ = (pos_ + n > size_) ? size_ : pos_ + n;
  }

  [[gnu::always_inline]] void jumpTo(std::size_t target) { pos_ = target; }

  [[gnu::always_inline]] std::size_t pos() const { return pos_; }

  [[gnu::always_inline]] bool eof() const { return pos_ >= size_; }

  [[gnu::always_inline]] const std::uint8_t* data() const {
    return data_ + pos_;
  }
};
