#include "object.h"
#include "vm.h"
#include "table.h"


#define ALLOCATE_OBJ(type, objectType) \
  (type*) allocateObject(sizeof(type), objectType)


static ObjString* allocateString(char*, int, uint32_t);
static Obj* allocateObject(size_t size, ObjType type);
static void freeObject(Obj*);
static uint32_t hashString(const char* key, int length);

ObjString* copyString(const char* chars, int length) {
  char* heapChars = malloc(length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  uint32_t hash = hashString(chars, length);

  ObjString* interned = tableFindString(&get_VM()->strings, 
      chars, length, hash);
  if (interned) return interned;

  return allocateString(heapChars, length, hash);
}

ObjString* takeString(char* chars, int length) {
  uint32_t hash = hashString(chars, length);

  ObjString* interned = tableFindString(&get_VM()->strings, 
      chars, length, hash);
  if (interned) {
    free(chars);
    return interned;
  }

  return allocateString(chars, length, hash);
}

static ObjString* allocateString(char* chars, int length, 
    uint32_t hash) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  return string;
}

static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = malloc(size);
  object->type = type;

  VM* vm = get_VM();
  object->next = vm->objects;
  vm->objects = object;
  return object;
}

void freeObjects(void) {
  VM* vm = get_VM();
  Obj* object = vm->objects;
  while (object != NULL) {
    Obj* next = object->next;
    freeObject(object);
    object = next;
  }
}

static void freeObject(Obj* object) {
  switch (object->type) {
    case OBJ_STRING: {
      ObjString* string = (ObjString*) object;
      free(string->chars);
      free(object);
      break;
    }
  }
}

static uint32_t hashString(const char* key, int length) {
  // FNV-1a hashing algorithm
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; ++i) {
    hash ^= (uint8_t) key[i];
    hash *= 16777619;
  }
  return hash;
}
