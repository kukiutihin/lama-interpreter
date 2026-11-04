#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

#include "app-runtime.h"
#include "bytecode-reader.h"
#include "tokens.h"

static constexpr size_t ELEMSIZE = sizeof(ptrt);
static constexpr size_t STACKSIZE = 64 * 1024 * ELEMSIZE;

class VirtualMachine {
  ptrt stack[STACKSIZE];

  const std::uint8_t* constantPool;
  const std::int32_t poolSize;
  const std::size_t globalsCount;

  const uint8_t* baseIp;
  const uint8_t* ip;
  const uint8_t* currentOpIp;
  const std::size_t codeSize;
  std::size_t fp;

  // needed to use instead of real ip
  std::uint8_t dummyForEOF[1] = {OP_EOF};
  std::uint8_t* eof = &dummyForEOF[0];

 public:
  VirtualMachine(Bytecode bytecode, const GC& gc)
      : constantPool(bytecode.constantPool),
        poolSize(bytecode.header.constPoolSize),
        globalsCount(bytecode.header.globalsCount),
        baseIp(bytecode.code),
        ip(bytecode.code),
        currentOpIp(bytecode.code),
        codeSize(bytecode.codeSize),
        fp(0) {
    initStack();
  }

  void interpret();
  void initStack();

 private:
  [[gnu::always_inline]] std::size_t stackSize() const {
    return (__gc_stack_bottom - __gc_stack_top) / ELEMSIZE;
  }

  [[gnu::always_inline]] void expectedSize(std::size_t n) const {
    if (stackSize() < n)
      failure("attempt to get %d elements from stack, stack size is %d (%s:%d)",
              n, stackSize(), getLastTokenString(), getLastTokenOffset());
  }

  [[gnu::always_inline]] void expectedCorrectIpOffset(
      std::size_t offset) const {
    if (offset < 0 || offset > (codeSize - 1))
      failure("attempt to jump to invalid offset: %d (%s:%d)", offset,
              getLastTokenString(), getLastTokenOffset());
  }

  [[gnu::always_inline]] ptrt pop() {
    expectedSize(1);
    __gc_stack_bottom -= ELEMSIZE;
    return *reinterpret_cast<ptrt*>(__gc_stack_bottom);
  }

  [[gnu::always_inline]] ptrt top() const {
    expectedSize(1);
    return *reinterpret_cast<ptrt*>(__gc_stack_bottom - ELEMSIZE);
  }

  [[gnu::always_inline]] void stackSwap() {
    expectedSize(2);
    std::swap(*reinterpret_cast<ptrt*>(__gc_stack_bottom - ELEMSIZE),
              *reinterpret_cast<ptrt*>(__gc_stack_bottom - (ELEMSIZE * 2)));
  }

  [[gnu::always_inline]] void popN(std::size_t n) {
    expectedSize(n);
    __gc_stack_bottom -= (ELEMSIZE * n);
  }

  [[gnu::always_inline]] void stackJumpTo(std::size_t offsetEl) {
    __gc_stack_bottom = __gc_stack_top + (offsetEl * ELEMSIZE);
  }

  [[gnu::always_inline]] void push(ptrt value) {
    *reinterpret_cast<ptrt*>(__gc_stack_bottom) = value;
    __gc_stack_bottom += ELEMSIZE;
  }

  [[gnu::always_inline]] std::size_t stackOffset() const {
    return ((__gc_stack_bottom - __gc_stack_top) / ELEMSIZE);
  }

  // Returns pointer to an element by given offset (in elements)
  [[gnu::always_inline]] ptrt* byOffset(std::size_t offset) {
    return reinterpret_cast<ptrt*>((__gc_stack_top + (ELEMSIZE * offset)));
  }

  [[gnu::always_inline]] std::size_t ipOffset() { return ip - baseIp; }

  template <typename T>
  [[gnu::always_inline]] T read() {
    if (ipOffset() + sizeof(T) > codeSize)
      failure(
          "attempt to read outside code boundaries, current position: %d, code "
          "size: %d, bytes to read: %d",
          ipOffset(), codeSize, sizeof(T));

    T var;
    std::memcpy(&var, ip, sizeof(T));
    ip += sizeof(T);
    return var;
  };

  [[gnu::always_inline]] std::int32_t readI32() {
    return read<std::int32_t>();
  };

  [[gnu::always_inline]] std::int32_t readU8() { return read<std::uint8_t>(); };

  [[gnu::always_inline]] ptrt* getGlobal(std::int32_t index) {
    if (index < 0 || index >= globalsCount)
      failure(
          "incorrect global variable index: %d, number of global variables: %d "
          "(%s:%s)",
          index, globalsCount, getLastTokenString(), getLastTokenOffset());

    return byOffset(index);
  };

  [[gnu::always_inline]] ptrt* getLocal(std::int32_t index) {
    auto localsCount = *byOffset(fp - 1);
    if (index < 0 || index >= localsCount)
      failure(
          "incorrect local variable index: %d, number of local variables: %d "
          "(%s:%s)",
          index, localsCount, getLastTokenString(), getLastTokenOffset());

    return byOffset(fp + index);
  };

  [[gnu::always_inline]] ptrt* getArg(std::int32_t index) {
    auto argsCount = *byOffset(fp - 3);
    if (index < 0 || index >= argsCount)
      failure(
          "incorrect argument index: %d, number of arguments: %d "
          "(%s:%s)",
          index, argsCount, getLastTokenString(), getLastTokenOffset());

    auto savedSP = UNBOX(*byOffset(fp - 6));
    return byOffset(savedSP + index);
  };

  [[gnu::always_inline]] ptrt* getClosure() { return byOffset(fp - 7); };

  [[gnu::always_inline]] ptrt getCapture(std::int32_t index) {
    auto capturesCount = *byOffset(fp - 2);
    if (index < 0 || index >= capturesCount)
      failure(
          "incorrect closure capture index: %d, number of captures: %d "
          "(%s:%s)",
          index, capturesCount, getLastTokenString(), getLastTokenOffset());

    auto closure = *getClosure();
    return reinterpret_cast<ptrt>(
        Belem(reinterpret_cast<void*>(closure), BOX(index + 1)));
  };

  [[gnu::always_inline]] const char* getLastTokenString() const {
    return opToString((Token)*currentOpIp);
  };

  [[gnu::always_inline]] std::size_t getLastTokenOffset() const {
    return currentOpIp - baseIp;
  };

  [[gnu::always_inline]] static ptrt isClosure(ptrt value) {
    return UNBOX(Bclosure_tag_patt(reinterpret_cast<void*>(value))) > 0 ? 1 : 0;
  };

  [[gnu::always_inline]] static ptrt isSexp(ptrt value) {
    return UNBOX(Bsexp_tag_patt(reinterpret_cast<void*>(value))) > 0 ? 1 : 0;
  };

  [[gnu::always_inline]] static ptrt isArray(ptrt value) {
    return UNBOX(Barray_tag_patt(reinterpret_cast<void*>(value))) > 0 ? 1 : 0;
  };

  [[gnu::always_inline]] static ptrt isString(ptrt value) {
    return UNBOX(Bstring_tag_patt(reinterpret_cast<void*>(value))) > 0 ? 1 : 0;
  };

  [[gnu::always_inline]] ptrt* expectedClosure(ptrt* value) {
    if (!isClosure(*value))
      failure("expected closure: %s:%d", getLastTokenString(),
              getLastTokenOffset());
    return value;
  };

  [[gnu::always_inline]] ptrt expectedNum(ptrt value) {
    if (!UNBOXED(value))
      failure("expected a num, but received a ptr: %s:%d", getLastTokenString(),
              getLastTokenOffset());
    return value;
  };

  [[gnu::always_inline]] ptrt expectedAggregate(ptrt value) {
    if (!isClosure(value) && !isSexp(value) && !isArray(value) &&
        !isString(value))
      failure("expected an aggregate (indexable) (%s:%d)", getLastTokenString(),
              getLastTokenOffset());
    return value;
  };

  [[gnu::always_inline]] void jumpToIp(std::int32_t offset) {
    expectedCorrectIpOffset(offset);
    ip = baseIp + offset;
  };

  [[gnu::always_inline]] void prologue(std::int32_t localsCount,
                                       std::int32_t capturesCount,
                                       std::int32_t argsCount) {
    push(BOX(fp));
    push(BOX(argsCount));
    push(BOX(capturesCount));
    push(BOX(localsCount));
    fp = stackOffset();

    // reserving space in stack for locals
    for (std::size_t i = 0; i < localsCount; i++) push(BOX(0));
  };

  [[gnu::always_inline]] void callerSequence(std::int32_t argsCount,
                                             std::int32_t offset) {
    expectedCorrectIpOffset(offset);

    auto dest = *(baseIp + offset);
    if (dest != OP_BEGIN && dest != OP_BEGIN_CLOSURE)
      failure(
          "called not a function, callee offset: %d, value by offset: %d "
          "(%s:%d)",
          offset, *(baseIp + offset), getLastTokenString(),
          getLastTokenOffset());

    push(BOX(stackOffset() - argsCount));   // saving sp
    push(BOX(reinterpret_cast<ptrt>(ip)));  // saving ip
    ip = baseIp + offset;
  };

  [[gnu::always_inline]] const std::uint8_t* getFromConstPool(
      std::int32_t offset) {
    if (offset < 0 || offset >= poolSize)
      failure(
          "invalid constant pool offset: %d, constant pool size: %d (%s:%d)",
          offset, poolSize, getLastTokenString(), getLastTokenOffset());
    return &constantPool[offset];
  }

  [[gnu::always_inline]] void callBuiltin(std::int32_t offset) {
    auto name = std::string_view(
        reinterpret_cast<const char*>(getFromConstPool(-offset - 1)));

    if (name == "write")
      push(Lwrite(pop()));

    else if (name == "read")
      push(Lread());

    else if (name == "length") {
      auto val = pop();
      push(Llength(reinterpret_cast<void*>(val)));

    } else if (name == "string") {
      auto val = pop();
      push(reinterpret_cast<ptrt>(Lstring(reinterpret_cast<aint*>(&val))));
    } else
      failure("unknown function: %s (%s:%d)\n", name.data(),
              getLastTokenString(), getLastTokenOffset());
  };
};
