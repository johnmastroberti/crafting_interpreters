#include <stdio.h>
#include <assert.h>
#include "value.h"

VECTOR_IMPL(ValueArray, Value)

void printValue(Value value) {
  switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      break;
    case VAL_NIL:
      printf("nil"); 
      break;
    case VAL_NUMBER: 
      printf("%g", AS_NUMBER(value)); 
      break;
  }
}

bool valuesEqual(Value a, Value b) {
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL:    return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:     return true;
    case VAL_NUMBER:  return AS_NUMBER(a) == AS_NUMBER(b);
    default:          assert(false); return false; // unreachable
  }
}
