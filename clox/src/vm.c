#include <stdio.h>
#include <stdarg.h>
#include "vm.h"
#include "value.h"
#include "debug.h"
#include "compiler.h"
#include "chunk.h"
#include "object.h"


static VM vm;


static InterpretResult run(void);
static Value peek(int distance);
static void runtimeError(const char* format, ...);
static void resetStack(void);
static bool isFalsey(Value);
static void concatenate(void);

void initVM(void) {
  resetStack();
  vm.objects = NULL;
  init_Table(&vm.strings);
}

void freeVM(void) {
  free_Table(&vm.strings);
  freeObjects();
}

VM* get_VM(void) { return &vm; }

InterpretResult interpret(const char* source) {
  Chunk chunk;
  init_Chunk(&chunk);

  if (!compile(source, &chunk)) {
    free_Chunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code.data;

  InterpretResult result = run();
  free_Chunk(&chunk);
  return result;
}

static InterpretResult run(void) {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.data[READ_BYTE()])
#define BINARY_OP(valueType, op) \
  do { \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtimeError("Operands must be numbers"); \
      return INTERPRET_RUNTIME_ERROR; \
    } \
    double b = AS_NUMBER(pop()); \
    double a = AS_NUMBER(pop()); \
    push(valueType(a op b)); \
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
      case OP_NIL: push(NIL_VAL); break;
      case OP_TRUE: push(BOOL_VAL(true)); break;
      case OP_FALSE: push(BOOL_VAL(false)); break;
      case OP_NOT:
        push(BOOL_VAL(isFalsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtimeError("Operand must be a number");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesEqual(a, b)));
        break;
      }
      case OP_GREATER:  BINARY_OP(BOOL_VAL,   >); break;
      case OP_LESS:     BINARY_OP(BOOL_VAL,   <); break;
      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          runtimeError(
              "Operands must be two numbers or two strings");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

static void resetStack(void) {
  vm.stackTop = vm.stack;
}

void push(Value value) {
  *vm.stackTop = value;
  ++vm.stackTop;
}

Value pop(void) {
  --vm.stackTop;
  return *vm.stackTop;
}

static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code.data - 1;
  int line = vm.chunk->lines.data[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  resetStack();
}

static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate(void) {
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());

  int length = a->length + b->length;
  char* chars = malloc(length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
  push(OBJ_VAL(result));
}
