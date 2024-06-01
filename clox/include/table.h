#pragma once

#include "common.h"
#include "value.h"
#include "vector.h"

#define TABLE_MAX_LOAD 0.75

typedef struct {
  ObjString* key;
  Value value;
} Entry;


VECTOR_DECL(Table, Entry)

bool tableSet(Table* table, ObjString* key, Value value);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars,
    int length, uint32_t hash);
