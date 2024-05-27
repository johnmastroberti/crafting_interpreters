#include <stdio.h>

#include "debug.h"
#include "value.h"

static const char* op_names[OP_LAST];

static void init_op_names(void) {
#define ADD_OP_NAME(name) op_names[name] = #name
  ADD_OP_NAME(OP_CONSTANT);
  ADD_OP_NAME(OP_ADD);
  ADD_OP_NAME(OP_SUBTRACT);
  ADD_OP_NAME(OP_MULTIPLY);
  ADD_OP_NAME(OP_DIVIDE);
  ADD_OP_NAME(OP_NEGATE);
  ADD_OP_NAME(OP_RETURN);
}

void disassembleChunk(Chunk* chunk, const char* name) {
  init_op_names();
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->code.size; )
    offset = disassembleInstruction(chunk, offset);
}

static int simpleInstruction(const char* name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int constantInstruction(const char* name, Chunk* chunk,
    int offset) {
  uint8_t constant = chunk->code.data[offset + 1];
  printf("%-16s %4d '", name, constant);
  printValue(chunk->constants.data[constant]);
  printf("'\n");
  return offset + 2;
}

int disassembleInstruction(Chunk* chunk, int offset) {
  printf("%04d ", offset);

  if (offset > 0 &&
      chunk->lines.data[offset] == chunk->lines.data[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines.data[offset]);
  }

  uint8_t instruction = chunk->code.data[offset];
  switch (instruction) {
    // Instructions with operands
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, offset);
    // Single byte instructions
    case OP_RETURN:
    case OP_NEGATE:
    case OP_ADD:
    case OP_SUBTRACT:
    case OP_MULTIPLY:
    case OP_DIVIDE:
      return simpleInstruction(op_names[instruction], offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}

