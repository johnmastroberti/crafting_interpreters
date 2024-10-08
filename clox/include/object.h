#pragma once

#include "common.h"
#include "value.h"
#include "chunk.h"

typedef enum {
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_STRING,
} ObjType;

struct Obj {
  ObjType type;
  struct Obj* next;
};

struct ObjString {
  Obj obj;
  int length;
  char* chars;
  uint32_t hash;
};

typedef struct {
  Obj obj;
  int arity;
  int upvalueCount;
  Chunk chunk;
  ObjString* name;
} ObjFunction;

typedef struct {
  Obj obj;
  ObjFunction* function;
} ObjClosure;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

ObjFunction* newFunction(void);
ObjNative* newNative(NativeFn function);
ObjClosure* newClosure(ObjFunction* function);

ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);

void freeObjects(void);

static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define OBJ_TYPE(value)       (AS_OBJ(value)->type)

#define IS_STRING(value)      isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value)    isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)      isObjType(value, OBJ_NATIVE)
#define IS_CLOSURE(value)     isObjType(value, OBJ_CLOSURE)

#define AS_STRING(value)      ((ObjString*) AS_OBJ(value))
#define AS_CSTRING(value)     ((AS_STRING(value))->chars)
#define AS_FUNCTION(value)    ((ObjFunction*) AS_OBJ(value))
#define AS_NATIVE(value) \
  (((ObjNative*) AS_OBJ(value))->function)
#define AS_CLOSURE(value)     ((ObjClosure*) AS_OBJ(value))
