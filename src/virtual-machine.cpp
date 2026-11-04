#include "virtual-machine.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "app-runtime.h"
#include "runtime_common.h"
#include "tokens.h"

void VirtualMachine::initStack() {
  __gc_stack_top = reinterpret_cast<std::size_t>(&stack[0]);
  __gc_stack_bottom = __gc_stack_top;

  // reserves space for globals in stack
  for (std::size_t i = 0; i < globalsCount; i++) push(BOX(0));

  // nobody saves ip for main function (call is not called)
  // we will save it here manually so that main can return control to EOF
  push(BOX(0));                            // argc
  push(BOX(0));                            // argv
  push(BOX(globalsCount));                 // sp
  push(BOX(reinterpret_cast<ptrt>(eof)));  // ip
}

/* STACK layout
 *
 * -------------
 *     arg_1       <- saved sp
 * -------------
 *      ...
 * -------------
 *     arg_n
 * -------------
 *    closure      saved in CALLC
 * -------------
 *    saved sp     saved in CALL/CALLC
 * -------------
 *    saved ip     saved in CALL/CALLC
 * -------------
 *    saved fp     saved in BEGIN/BEGIN_CLOSURE
 * -------------
 *   argsCount     saved in BEGIN/BEGIN_CLOSURE
 * -------------
 * capturesCount   saved in BEGIN/BEGIN_CLOSURE
 * -------------
 *  locals count   saved in BEGIN/BEGIN_CLOSURE
 * -------------
 *    local_1      <- new fp
 * -------------
 *      ...
 * -------------
 *    local_n
 * -------------
 *  return value
 * -------------
 *                 <- sp
 */

#define BINOP(fn)                            \
  {                                          \
    auto b = reinterpret_cast<void*>(pop()); \
    auto a = reinterpret_cast<void*>(pop()); \
    push(fn(a, b));                          \
  }

void VirtualMachine::interpret() {
  while (true) {
    currentOpIp = ip++;
    switch (*currentOpIp) {
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
        push(BOX(readI32()));
        break;

      case OP_STRING: {
        auto rawString = getFromConstPool(readI32());
        auto ptr = Bstring(reinterpret_cast<aint*>(&rawString));
        push(reinterpret_cast<ptrt>(ptr));
        break;
      }

      case OP_SEXP: {
        auto tag = getFromConstPool(readI32());
        push(LtagHash(reinterpret_cast<const char*>(tag)));

        auto n = readI32();
        auto ptr =
            Bsexp(reinterpret_cast<aint*>(byOffset(stackOffset() - (n + 1))),
                  BOX(n + 1));

        popN(n + 1);
        push(reinterpret_cast<ptrt>(ptr));
        break;
      }

      case OP_STA: {
        auto value = pop();
        auto index = expectedNum(pop());
        auto aggregate = expectedAggregate(pop());

        auto updated = Bsta(reinterpret_cast<void*>(aggregate), index,
                            reinterpret_cast<void*>(value));
        push(reinterpret_cast<ptrt>(updated));
        break;
      }

      case OP_DROP:
        pop();
        break;

      case OP_DUP:
        push(top());
        break;

      case OP_SWAP:
        stackSwap();
        break;

      case OP_ELEM: {
        auto index = expectedNum(pop());
        auto aggregate = expectedAggregate(pop());
        auto value = Belem(reinterpret_cast<void*>(aggregate), index);
        push(reinterpret_cast<ptrt>(value));
        break;
      }

      case OP_LD_GLO: {
        auto index = readI32();
        push(*getGlobal(index));
        break;
      }

      case OP_LD_LOC: {
        auto index = readI32();
        push(*getLocal(index));
        break;
      }

      case OP_LD_ARG: {
        auto index = readI32();
        push(*getArg(index));
        break;
      }

      case OP_LD_CLO: {
        auto index = readI32();
        push(reinterpret_cast<ptrt>(getCapture(index)));
        break;
      }

      case OP_ST_GLO: {
        auto index = readI32();
        *getGlobal(index) = top();
        break;
      }

      case OP_ST_LOC: {
        auto index = readI32();
        *getLocal(index) = top();
        break;
      }

      case OP_ST_ARG: {
        auto index = readI32();
        *getArg(index) = top();
        break;
      }

      case OP_ST_CLO: {
        auto index = readI32();
        auto closure = *getClosure();

        auto capturesCount = *byOffset(fp - 2);
        if (index < 0 || index >= capturesCount)
          failure(
              "incorrect closure capture index: %d, number of captures: %d "
              "(%s:%s)",
              index, capturesCount, getLastTokenString(), getLastTokenOffset());

        push(reinterpret_cast<ptrt>(Bsta(reinterpret_cast<void*>(closure),
                                         BOX(index + 1),
                                         reinterpret_cast<void*>(pop()))));
        break;
      }

      case OP_JMP:
        jumpToIp(readI32());
        break;

      case OP_CJMP_Z: {
        auto offset = readI32();
        if (UNBOX(pop()) == 0) jumpToIp(offset);
        break;
      }

      case OP_CJMP_NZ: {
        auto offset = readI32();
        if (UNBOX(pop()) != 0) jumpToIp(offset);
        break;
      }

      case OP_BEGIN: {
        auto argsCount = readI32();
        auto localsCount = readI32();
        prologue(localsCount, 0, argsCount);
        break;
      }

      case OP_BEGIN_CLOSURE: {
        auto argsCount = readI32();
        auto localsCount = readI32();
        auto capturesCount = readI32();
        prologue(localsCount, capturesCount, argsCount);
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
            expectedClosure(byOffset(stackOffset() - argsCount - 1));
        auto closure = *closureAddr;
        auto offset = UNBOX(expectedNum(reinterpret_cast<ptrt>(
            Belem(reinterpret_cast<void*>(closure), BOX(0)))));  // closure[0]

        // [captures count][closure][arg_0]...[arg_n-1]...
        // ->
        // [arg_0]...[arg_n-1]...
        for (std::size_t i = 0; i < argsCount; i++)
          closureAddr[i] = closureAddr[i + 1];

        // ->
        // [arg_0]...[arg_n-1][closure][captures count]...
        pop();
        push(closure);

        callerSequence(argsCount + 1, offset);
        break;
      }

      case OP_END: {
        auto returnValue = pop();
        stackJumpTo(fp);
        pop();  // locals count
        pop();  // captures count
        pop();  // args count
        fp = UNBOX(pop());
        ip = reinterpret_cast<std::uint8_t*>(UNBOX(pop()));
        stackJumpTo(UNBOX(pop()));  // jumps to saved sp
        push(returnValue);
        break;
      }

      case OP_CLOSURE: {
        auto ipOffset = readI32();
        auto capturesCount = readI32();
        push(BOX(ipOffset));

        for (std::size_t i = 0; i < capturesCount; i++) {
          auto kind = readU8();
          auto index = readI32();

          switch (kind) {
            case 0:  // global
              push(*getGlobal(index));
              break;

            case 1:  // local
              push(*getLocal(index));
              break;

            case 2:  // arg
              push(*getArg(index));
              break;

            case 3:  // capture
              push(getCapture(index));
              break;
          }
        }

        auto closure = reinterpret_cast<ptrt>(
            Bclosure(reinterpret_cast<aint*>(
                         byOffset(stackOffset() - capturesCount - 1)),
                     BOX(capturesCount)));

        popN(capturesCount + 1);
        push(closure);
        break;
      }

      case OP_TAG: {
        auto name = LtagHash(
            reinterpret_cast<const char*>(getFromConstPool(readI32())));
        auto fieldsCount = readI32();
        push(Btag(reinterpret_cast<void*>(pop()), name, BOX(fieldsCount)));
        break;
      }

      case OP_ARRAY: {
        auto elementsCount = readI32();
        push(Barray_patt(reinterpret_cast<void*>(pop()), BOX(elementsCount)));
        break;
      }

      case OP_FAIL: {
        auto line = readI32();
        auto col = readI32();
        pop();
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
        push(Bstring_patt(reinterpret_cast<void*>(pop()),
                          reinterpret_cast<void*>(pop())));
        break;

      case OP_PATT_STRING:
        push(BOX(isString(pop())));
        break;

      case OP_PATT_ARRAY:
        push(BOX(isArray(pop())));
        break;

      case OP_PATT_SEXP:
        push(BOX(isSexp(pop())));
        break;

      case OP_PATT_BOXED:
        push(Bboxed_patt(reinterpret_cast<void*>(pop())));
        break;

      case OP_PATT_UNBOXED:
        push(Bunboxed_patt(reinterpret_cast<void*>(pop())));
        break;

      case OP_PATT_CLOSURE:
        push(BOX(isClosure(pop())));
        break;

      case OP_BARRAY: {
        auto n = readI32();
        auto elems = byOffset(stackOffset() - n);
        auto array = Barray(reinterpret_cast<aint*>(elems), BOX(n));
        popN(n);
        push(reinterpret_cast<ptrt>(array));
        break;
      }

      case OP_EOF:
        return;
    }
  }
}
