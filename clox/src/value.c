#include <stdio.h>
#include <assert.h>
#include "value.h"
#include "object.h"

VECTOR_IMPL(ValueArray, Value)

void printObject(Value value);
static void printFunction(ObjFunction*);

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
    case VAL_OBJ:     return AS_OBJ(a) == AS_OBJ(b);
    default:          assert(false); return false; // unreachable
  }
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
    case OBJ_FUNCTION:
      printFunction(AS_FUNCTION(value));
      break;
  }
}

static void printFunction(ObjFunction* function) {
  if (function->name)
    printf("<fn %s>", function->name->chars);
  else
    printf("<script>");
}
