#pragma once

#include "common.h"

#define VECTOR_DECL(StructName, Type) \
  typedef struct { \
    int   size; \
    int   capacity; \
    Type* data; \
  } StructName; \
  void init_ ## StructName (StructName*); \
  void free_ ## StructName (StructName*); \
  void resize_ ## StructName (StructName*, int newsize); \
  void reserve_ ## StructName (StructName*, int newcap); \
  void push_back_ ## StructName (StructName*, Type ele); \
  void push_back_unsafe_ ## StructName (StructName*, Type ele); \


#define VECTOR_IMPL(StructName, Type) \
  void init_ ## StructName (StructName* v) { \
    v->size = 0; \
    v->capacity = 0; \
    v->data = NULL; \
  } \
  void free_ ## StructName (StructName* v) { \
    if (v->data) free(v->data); \
    init_ ## StructName (v); \
  } \
  void resize_ ## StructName (StructName* v, int newsize) { \
    v->data = realloc(v, newsize * sizeof(Type)); \
    if (newsize > v->size) \
      memset(v->data + v->size, 0, \
          (newsize - v->size) * sizeof(Type)); \
    v->size = newsize; \
    v->capacity = newsize; \
  } \
  void reserve_ ## StructName (StructName* v, int newcap) { \
    if (newcap > v->capacity) \
      v->data = realloc(v->data, newcap * sizeof(Type)); \
    v->capacity = newcap; \
  } \
  void push_back_ ## StructName (StructName* v, Type ele) { \
    if (v->size == v->capacity) \
      reserve_ ## StructName (v, MAX(8, v->capacity * 2)); \
    push_back_unsafe_ ## StructName (v, ele); \
  } \
  void push_back_unsafe_ ## StructName (StructName* v, Type ele) { \
    v->data[v->size] = ele; \
    v->size++; \
  } \

