#include "object.h"
#include "vm.h"


#define ALLOCATE_OBJ(type, objectType) \
  (type*) allocateObject(sizeof(type), objectType)


static ObjString* allocateString(char*, int);
static Obj* allocateObject(size_t size, ObjType type);
static void freeObject(Obj*);

ObjString* copyString(const char* chars, int length) {
  char* heapChars = malloc(length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length);
}

ObjString* takeString(char* chars, int length) {
  return allocateString(chars, length);
}

static ObjString* allocateString(char* chars, int length) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
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
