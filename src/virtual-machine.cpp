#include "virtual-machine.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

#include "app-runtime.h"
#include "runtime_common.h"
#include "stack.h"
#include "tokens.h"

Stack VirtualMachine::allocateStack() {
  Stack stack;

  // reserves space for globals in stack
  for (std::size_t i = 0; i < globalsCount; i++) stack.push(BOX(0));

  // nobody saves ip for main function (call is not called)
  // we will save it here manually so that main can return control to EOF
  stack.push(BOX(0));                            // argc
  stack.push(BOX(0));                            // argv
  stack.push(BOX(globalsCount));                 // sp
  stack.push(BOX(reinterpret_cast<ptrt>(eof)));  // ip

  return stack;
}

/* STACK layout
 *
 * -----------
 *    arg_1     <- saved sp
 * -----------
 *     ...
 * -----------
 *    arg_n
 * -----------
 *   closure     optional
 * -----------
 *  saved sp     saved in call
 * -----------
 *  saved ip     saved in call
 * -----------
 *  saved fp     saved in begin
 * -----------
 *   local_1     <- new fp
 * -----------
 *     ...
 * -----------
 *   local_n
 * -----------
 * return value
 * -----------
 *               <- sp
 */

void VirtualMachine::interpret(uint8_t* ipBase, std::size_t codeStartOffset) {
  auto stack = allocateStack();
  std::uint8_t* pool = constantPool;
  std::size_t fp = 0;
  auto ip = ipBase;

  auto read = [&ip]<typename T> [[gnu::always_inline]] (std::size_t size) -> T {
    T var;
    std::memcpy(&var, ip, size);
    ip += size;
    return var;
  };

  auto readI32 = [&] [[gnu::always_inline]] () -> std::int32_t {
    return read.operator()<std::int32_t>(4);
  };

  auto readU8 = [&] [[gnu::always_inline]] () -> std::uint8_t {
    return read.operator()<std::uint8_t>(1);
  };

  auto isClosure = [&] [[gnu::always_inline]] (ptrt value) -> ptrt {
    return BOX(
        UNBOX(Bclosure_tag_patt(reinterpret_cast<void*>(value))) > 0 ? 1 : 0);
  };

  auto isSexp = [&] [[gnu::always_inline]] (ptrt value) -> ptrt {
    return BOX(UNBOX(Bsexp_tag_patt(reinterpret_cast<void*>(value))) > 0 ? 1
                                                                         : 0);
  };

  auto isArray = [&] [[gnu::always_inline]] (ptrt value) -> ptrt {
    return BOX(UNBOX(Barray_tag_patt(reinterpret_cast<void*>(value))) > 0 ? 1
                                                                          : 0);
  };

  auto isString = [&] [[gnu::always_inline]] (ptrt value) -> ptrt {
    return BOX(UNBOX(Bstring_tag_patt(reinterpret_cast<void*>(value))) > 0 ? 1
                                                                           : 0);
  };

  auto opOffset = [&] [[gnu::always_inline]] () -> std::size_t {
    return codeStartOffset + (ip - ipBase);
  };

  auto expectedClosure = [&] [[gnu::always_inline]] (ptrt * value) -> ptrt* {
    if (!isClosure(*value))
      failure("expected closure: %s:%d", opToString((Token) * (ip - 1)),
              opOffset());
    return value;
  };

  auto expectedNum = [&] [[gnu::always_inline]] (ptrt value) -> ptrt {
    if (!UNBOXED(value))
      failure("expected a num, but received a ptr: %s:%d",
              opToString((Token) * (ip - 1)), opOffset());
    return value;
  };

  auto expectedAggregate = [&] [[gnu::always_inline]] (ptrt value) -> ptrt {
    if (!isClosure(value) && !isSexp(value) && !isArray(value) &&
        !isString(value))
      failure("expected an aggregate (indexable): %s:%d",
              opToString((Token) * (ip - 1)), opOffset());
    return value;
  };

  auto getGlobal = [&] [[gnu::always_inline]] (std::int32_t index) {
    return stack.byOffset(index);
  };

  auto getLocal = [&] [[gnu::always_inline]] (std::int32_t index) {
    return stack.byOffset(fp + index);
  };

  auto getArg = [&] [[gnu::always_inline]] (std::int32_t index) {
    auto savedSP = UNBOX(*stack.byOffset(fp - 3));
    return stack.byOffset(savedSP + index);
  };

  auto getClosure = [&] [[gnu::always_inline]] () {
    return stack.byOffset(fp - 4);
  };

  auto getCapture = [&] [[gnu::always_inline]] (std::int32_t index) {
    auto closure = *getClosure();
    return reinterpret_cast<ptrt>(
        Belem(reinterpret_cast<void*>(closure), BOX(index + 1)));
  };

  auto jumpTo = [&] [[gnu::always_inline]] (std::int32_t offset) {
    ip = ipBase + offset;
  };

  auto prologue = [&] [[gnu::always_inline]] (std::int32_t localsCount) {
    stack.push(BOX(fp));  // saving fp
    fp = stack.offset();

    // reserving space in stack for locals
    for (std::size_t i = 0; i < localsCount; i++) stack.push(BOX(0));
  };

  auto callerSequence = [&] [[gnu::always_inline]] (std::int32_t argsCount,
                                                    std::int32_t offset) {
    stack.push(BOX(stack.offset() - argsCount));  // saving sp
    stack.push(BOX(reinterpret_cast<ptrt>(ip)));  // saving ip
    ip = ipBase + offset;
  };

  auto callBuiltin = [&] [[gnu::always_inline]] (std::int32_t offset) {
    auto name =
        std::string_view(reinterpret_cast<const char*>(&pool[(-offset - 1)]));

    if (name == "write")
      stack.push(Lwrite(stack.pop()));

    else if (name == "read")
      stack.push(Lread());

    else if (name == "length") {
      auto val = stack.pop();
      stack.push(Llength(reinterpret_cast<void*>(val)));

    } else if (name == "string") {
      auto val = stack.pop();
      stack.push(
          reinterpret_cast<ptrt>(Lstring(reinterpret_cast<aint*>(&val))));
    } else
      failure("unknown function: %s\n", name.data());
  };

#define BINOP(fn)                                  \
  {                                                \
    auto b = reinterpret_cast<void*>(stack.pop()); \
    auto a = reinterpret_cast<void*>(stack.pop()); \
    stack.push(fn(a, b));                          \
  }

  while (true) {
    switch (*ip++) {
      case OP_BINOP_ADD:
        BINOP(Ls__Infix_43);
        break;

      case OP_BINOP_SUB:
        BINOP(Ls__Infix_45);
        break;

      case OP_BINOP_MUL:
        BINOP(Ls__Infix_42);
        break;

      case OP_BINOP_DIV:
        BINOP(Ls__Infix_47);
        break;

      case OP_BINOP_MOD:
        BINOP(Ls__Infix_37);
        break;

      case OP_BINOP_LT:
        BINOP(Ls__Infix_60);
        break;

      case OP_BINOP_LE:
        BINOP(Ls__Infix_6061);
        break;

      case OP_BINOP_GT:
        BINOP(Ls__Infix_62);
        break;

      case OP_BINOP_GE:
        BINOP(Ls__Infix_6261);
        break;

      case OP_BINOP_EQ:
        BINOP(Ls__Infix_6161);
        break;

      case OP_BINOP_NEQ:
        BINOP(Ls__Infix_3361);
        break;

      case OP_BINOP_AND:
        BINOP(Ls__Infix_3838);
        break;

      case OP_BINOP_OR:
        BINOP(Ls__Infix_3333);
        break;

      case OP_CONST:
        stack.push(BOX(readI32()));
        break;

      case OP_STRING: {
        auto rawString = &pool[readI32()];
        auto ptr = Bstring(reinterpret_cast<aint*>(&rawString));
        stack.push(reinterpret_cast<ptrt>(ptr));
        break;
      }

      case OP_SEXP: {
        auto tag = reinterpret_cast<const char*>(&pool[readI32()]);
        stack.push(LtagHash(tag));

        auto n = readI32();
        auto ptr = Bsexp(
            reinterpret_cast<aint*>(stack.byOffset(stack.offset() - (n + 1))),
            BOX(n + 1));

        stack.popN(n + 1);
        stack.push(reinterpret_cast<ptrt>(ptr));
        break;
      }

      case OP_STA: {
        auto value = stack.pop();
        auto index = expectedNum(stack.pop());
        auto aggregate = expectedAggregate(stack.pop());

        auto updated = Bsta(reinterpret_cast<void*>(aggregate), index,
                            reinterpret_cast<void*>(value));
        stack.push(reinterpret_cast<ptrt>(updated));
        break;
      }

      case OP_DROP:
        stack.pop();
        break;

      case OP_DUP:
        stack.push(stack.top());
        break;

      case OP_SWAP:
        stack.swap();
        break;

      case OP_ELEM: {
        auto index = expectedNum(stack.pop());
        auto aggregate = expectedAggregate(stack.pop());
        auto value = Belem(reinterpret_cast<void*>(aggregate), index);
        stack.push(reinterpret_cast<ptrt>(value));
        break;
      }

      case OP_LD_GLO: {
        auto index = readI32();
        stack.push(*getGlobal(index));
        break;
      }

      case OP_LD_LOC: {
        auto index = readI32();
        stack.push(*getLocal(index));
        break;
      }

      case OP_LD_ARG: {
        auto index = readI32();
        stack.push(*getArg(index));
        break;
      }

      case OP_LD_CLO: {
        auto index = readI32();
        stack.push(reinterpret_cast<ptrt>(getCapture(index)));
        break;
      }

      case OP_ST_GLO: {
        auto index = readI32();
        *getGlobal(index) = stack.top();
        break;
      }

      case OP_ST_LOC: {
        auto index = readI32();
        *getLocal(index) = stack.top();
        break;
      }

      case OP_ST_ARG: {
        auto index = readI32();
        *getArg(index) = stack.top();
        break;
      }

      case OP_ST_CLO: {
        auto offset = readI32();
        auto closure = *getClosure();

        stack.push(reinterpret_cast<ptrt>(
            Bsta(reinterpret_cast<void*>(closure), BOX(offset + 1),
                 reinterpret_cast<void*>(stack.pop()))));
        break;
      }

      case OP_JMP:
        jumpTo(readI32());
        break;

      case OP_CJMP_Z: {
        auto offset = readI32();
        if (UNBOX(stack.pop()) == 0) jumpTo(offset);
        break;
      }

      case OP_CJMP_NZ: {
        auto offset = readI32();
        if (UNBOX(stack.pop()) != 0) jumpTo(offset);
        break;
      }

      case OP_BEGIN: {
        readI32();  // args
        auto localsCount = readI32();
        prologue(localsCount);
        break;
      }

      case OP_BEGIN_CLOSURE: {
        readI32();  // args
        auto localsCount = readI32();
        readI32();  // captures
        prologue(localsCount);
        break;
      }

      case OP_CALL: {
        auto offset = readI32();
        auto argsCount = readI32();

        // positive offset points to internal function in bytecode
        // negative points to external (built-in) function
        if (offset >= 0)
          callerSequence(argsCount, offset);
        else
          callBuiltin(offset);

        break;
      }

      case OP_CALLC: {
        auto argsCount = readI32();
        auto closureAddr =
            expectedClosure(stack.byOffset(stack.offset() - argsCount - 1));
        auto closure = *closureAddr;
        auto offset = UNBOX(expectedNum(reinterpret_cast<ptrt>(
            Belem(reinterpret_cast<void*>(closure), BOX(0)))));  // closure[0]

        //    (sp)
        // [closure][arg_0]...[arg_n-1]... ->
        //
        //   (sp)
        // [arg_0]...[arg_n-1]...
        for (std::size_t i = 0; i < argsCount; i++)
          closureAddr[i] = closureAddr[i + 1];

        //   (sp)
        // [arg_0]...[arg_n-1][closure]...
        stack.pop();
        stack.push(closure);

        callerSequence(argsCount + 1, offset);
        break;
      }

      case OP_END: {
        auto returnValue = stack.pop();
        stack.jumpTo(fp);
        fp = UNBOX(stack.pop());
        ip = reinterpret_cast<std::uint8_t*>(UNBOX(stack.pop()));
        stack.jumpTo(UNBOX(stack.pop()));  // jumps to saved sp
        stack.push(returnValue);
        break;
      }

      case OP_CLOSURE: {
        auto ipOffset = readI32();
        auto capturesCount = readI32();
        stack.push(BOX(ipOffset));

        for (std::size_t i = 0; i < capturesCount; i++) {
          auto kind = readU8();
          auto index = readI32();

          switch (kind) {
            case 0:  // global
              stack.push(*getGlobal(index));
              break;

            case 1:  // local
              stack.push(*getLocal(index));
              break;

            case 2:  // arg
              stack.push(*getArg(index));
              break;

            case 3:  // capture
              stack.push(getCapture(index));
              break;
          }
        }

        auto closure = reinterpret_cast<ptrt>(
            Bclosure(reinterpret_cast<aint*>(
                         stack.byOffset(stack.offset() - capturesCount - 1)),
                     BOX(capturesCount)));

        stack.popN(capturesCount + 1);
        stack.push(closure);
        break;
      }

      case OP_TAG: {
        auto name = LtagHash(reinterpret_cast<const char*>(&pool[readI32()]));
        auto fieldsCount = readI32();
        stack.push(
            Btag(reinterpret_cast<void*>(stack.pop()), name, BOX(fieldsCount)));
        break;
      }

      case OP_ARRAY: {
        auto elementsCount = readI32();
        stack.push(Barray_patt(reinterpret_cast<void*>(stack.pop()),
                               BOX(elementsCount)));
        break;
      }

      case OP_FAIL: {
        auto line = readI32();
        auto col = readI32();
        stack.pop();
        failure("Match error at %d:%d\n", line, col);
        break;
      }

      case OP_FAIL_KEEP: {
        auto line = readI32();
        auto col = readI32();
        failure("Match error at %d:%d\n", line, col);
        break;
      }

      case OP_LINE:
        readI32();
        break;

      case OP_PATT_STR_CMP:
        stack.push(Bstring_patt(reinterpret_cast<void*>(stack.pop()),
                                reinterpret_cast<void*>(stack.pop())));
        break;

      case OP_PATT_STRING:
        stack.push(isString(stack.pop()));
        break;

      case OP_PATT_ARRAY:
        stack.push(isArray(stack.pop()));
        break;

      case OP_PATT_SEXP:
        stack.push(isSexp(stack.pop()));
        break;

      case OP_PATT_BOXED:
        stack.push(Bboxed_patt(reinterpret_cast<void*>(stack.pop())));
        break;

      case OP_PATT_UNBOXED:
        stack.push(Bunboxed_patt(reinterpret_cast<void*>(stack.pop())));
        break;

      case OP_PATT_CLOSURE:
        stack.push(isClosure(stack.pop()));
        break;

      case OP_BARRAY: {
        auto n = readI32();
        auto elems = stack.byOffset(stack.offset() - n);
        auto array = Barray(reinterpret_cast<aint*>(elems), BOX(n));
        stack.popN(n);
        stack.push(reinterpret_cast<ptrt>(array));
        break;
      }

      case OP_EOF:
        return;
    }
  }
}
