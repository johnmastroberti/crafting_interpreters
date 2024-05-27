#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char* argv[]) {
  initVM();

  Chunk chunk;
  init_Chunk(&chunk);

  int constant = addConstant(&chunk, 1.2);
  for (int i = 0; i < 20; ++i) {
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);
  }

  writeChunk(&chunk, OP_RETURN, 123);

  disassembleChunk(&chunk, "test chunk");
  interpret(&chunk);
  free_Chunk(&chunk);

  freeVM();
  return 0;
}
