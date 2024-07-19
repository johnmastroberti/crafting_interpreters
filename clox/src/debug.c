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
  ADD_OP_NAME(OP_NIL);
  ADD_OP_NAME(OP_TRUE);
  ADD_OP_NAME(OP_FALSE);
  ADD_OP_NAME(OP_NOT);
  ADD_OP_NAME(OP_EQUAL);
  ADD_OP_NAME(OP_GREATER);
  ADD_OP_NAME(OP_LESS);
  ADD_OP_NAME(OP_PRINT);
  ADD_OP_NAME(OP_POP);
  ADD_OP_NAME(OP_DEFINE_GLOBAL);
  ADD_OP_NAME(OP_GET_GLOBAL);
  ADD_OP_NAME(OP_SET_GLOBAL);
  ADD_OP_NAME(OP_GET_LOCAL);
  ADD_OP_NAME(OP_SET_LOCAL);
  ADD_OP_NAME(OP_JUMP);
  ADD_OP_NAME(OP_JUMP_IF_FALSE);
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

static int byteInstruction(const char* name, Chunk* chunk,
    int offset) {
  uint8_t slot = chunk->code.data[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2;
}

static int jumpInstruction(const char* name, int sign,
    Chunk* chunk, int offset) {
  uint16_t jump = (uint16_t) (chunk->code.data[offset+1] << 8);
  jump |= chunk->code.data[offset+2];
  printf("%-16s %4d -> %d\n", name, offset,
      offset + 3 + sign * jump);
  return offset + 3;
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
    case OP_DEFINE_GLOBAL:
    case OP_GET_GLOBAL:
    case OP_SET_GLOBAL:
      return constantInstruction(op_names[instruction], 
          chunk, offset);
    case OP_GET_LOCAL:
    case OP_SET_LOCAL:
      return byteInstruction(op_names[instruction], 
          chunk, offset);
    case OP_JUMP:
    case OP_JUMP_IF_FALSE:
      return jumpInstruction(op_names[instruction],
          1, chunk, offset);
    // Single byte instructions
    case OP_RETURN:
    case OP_NEGATE:
    case OP_ADD:
    case OP_SUBTRACT:
    case OP_MULTIPLY:
    case OP_DIVIDE:
    case OP_NIL:
    case OP_TRUE:
    case OP_FALSE:
    case OP_NOT:
    case OP_EQUAL:
    case OP_GREATER:
    case OP_LESS:
    case OP_PRINT:
    case OP_POP:
      return simpleInstruction(op_names[instruction], offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}

