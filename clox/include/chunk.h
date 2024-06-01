#pragma once

#include "vector.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_POP,
  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_RETURN,
  OP_LAST
} OpCode;

VECTOR_DECL(ByteArray, uint8_t)
VECTOR_DECL(IntArray, int)

typedef struct {
  ByteArray code;
  IntArray lines;
  ValueArray constants;
} Chunk;

void init_Chunk(Chunk*);
void free_Chunk(Chunk*);

int addConstant(Chunk*, Value value);
void writeChunk(Chunk* chunk, uint8_t byte, int line);

