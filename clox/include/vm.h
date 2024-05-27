#pragma once

#include "chunk.h"
#include "value.h"

typedef struct {
  Chunk* chunk;
  uint8_t* ip;
  Value* stack;
  Value* stackTop;
  size_t stack_size;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void initVM(void);
void freeVM(void);

InterpretResult interpret(Chunk* chunk);

void push(Value value);
Value pop(void);
