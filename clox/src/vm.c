#include <stdio.h>
#include "vm.h"
#include "value.h"
#include "debug.h"

static InterpretResult run(void);

static VM vm;

void initVM(void) {
  vm.stackTop = vm.stack;
}

void freeVM(void) {}

InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code.data;
  return run();
}

static InterpretResult run(void) {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.data[READ_BYTE()])
#define BINARY_OP(op) \
  do { \
    double b = pop(); \
    double a = pop(); \
    push(a op b); \
  } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    fputs("          ", stdout);
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      fputs("[ ", stdout);
      printValue(*slot);
      fputs(" ]", stdout);
    }
    puts("");

    disassembleInstruction(vm.chunk,
        (int) (vm.ip - vm.chunk->code.data));
#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_RETURN: {
        printValue(pop());
        puts("");
        return INTERPRET_OK;
      }
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_NEGATE:   push(-pop()); break;
      case OP_ADD:      BINARY_OP(+); break;
      case OP_SUBTRACT: BINARY_OP(-); break;
      case OP_MULTIPLY: BINARY_OP(*); break;
      case OP_DIVIDE:   BINARY_OP(/); break;
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}


void push(Value value) {
  *vm.stackTop = value;
  ++vm.stackTop;
}

Value pop(void) {
  --vm.stackTop;
  return *vm.stackTop;
}
