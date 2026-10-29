#include <cstdint>

enum ConstantType : std::uint8_t {
  CONST_STRING
};

using ConstantPool = std::uint8_t*;
