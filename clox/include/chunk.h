#pragma once

#include "vector.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_RETURN,
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

