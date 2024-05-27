#include <stdlib.h>
#include "chunk.h"
#include "memory.h"
#include "vector.h"

VECTOR_IMPL(ByteArray, uint8_t)
VECTOR_IMPL(IntArray, int)

void init_Chunk(Chunk* chunk) {
  init_ByteArray(&chunk->code);
  init_IntArray(&chunk->lines);
  init_ValueArray(&chunk->constants);
}

void free_Chunk(Chunk* chunk) {
  free_ByteArray(&chunk->code);
  free_IntArray(&chunk->lines);
  free_ValueArray(&chunk->constants);
}

int addConstant(Chunk* chunk, Value value) {
  push_back_ValueArray(&chunk->constants, value);
  return chunk->constants.size - 1;
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
  push_back_ByteArray(&chunk->code, byte);
  push_back_IntArray(&chunk->lines, line);
}
