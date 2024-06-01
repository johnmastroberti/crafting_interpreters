#include <stdio.h>
#include <assert.h>
#include "value.h"
#include "object.h"

VECTOR_IMPL(ValueArray, Value)

void printObject(Value value);

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
    case VAL_OBJ:
      printObject(value);
      break;
  }
}

bool valuesEqual(Value a, Value b) {
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL:    return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:     return true;
    case VAL_NUMBER:  return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ: {
      if (OBJ_TYPE(a) != OBJ_TYPE(b)) 
        return false;

      ObjString* aString = AS_STRING(a);
      ObjString* bString = AS_STRING(b);
      return aString->length == bString->length &&
        memcmp(aString->chars, bString->chars,
            aString->length) == 0;
    }
    default:          assert(false); return false; // unreachable
  }
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
  }
}
