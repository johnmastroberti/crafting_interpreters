#include "table.h"
#include "object.h"

VECTOR_IMPL(Table, Entry)

static void adjustCapacity(Table*, int);
static Entry* findEntry(Entry* entries, int capacity,
    ObjString* key);

bool tableSet(Table* table, ObjString* key, Value value) {
  if (table->size + 1 > table->capacity * TABLE_MAX_LOAD) {
    adjustCapacity(table, table->capacity * 2);
  }
  Entry* entry = findEntry(table->data, table->capacity, key);
  bool isNewKey = entry->key == NULL;
  if (isNewKey && IS_NIL(entry->value)) table->size++;

  entry->key = key;
  entry->value = value;
  return isNewKey;
}

static Entry* findEntry(Entry* entries, int capacity,
    ObjString* key) {
  uint32_t index = key->hash % capacity;
  Entry* tombstone = NULL;

  for (;;) {
    Entry* entry = entries + index;
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // Empty entry
        return tombstone != NULL ? tombstone : entry;
      } else {
        // Hit a tombstone
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      // Key found
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

static void adjustCapacity(Table* table, int capacity) {
  Table new_table;
  init_Table(&new_table);

  // Avoid adjusting to zero capacity
  capacity = MAX(capacity, 8);

  reserve_Table(&new_table, capacity);
  // Zero out all new buckets
  for (int i = 0; i < capacity; ++i) {
    new_table.data[i].key = NULL;
    new_table.data[i].value = NIL_VAL;
  }
  

  // Re-hash the table
  table->size = 0;
  for (int i = 0; i < table->capacity; ++i) {
    Entry* entry = table->data + i;
    if (entry->key == NULL) continue;

    Entry* dest = findEntry(new_table.data, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->size++;
  }

  free_Table(table);

  table->data = new_table.data;
  table->capacity = capacity;
  table->size = new_table.size;
}

void tableAddAll(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; ++i) {
    Entry* entry = from->data + i;
    if (entry->key != NULL)
      tableSet(to, entry->key, entry->value);
  }
}

bool tableGet(Table* table, ObjString* key, Value* value) {
  if (table->size == 0) return false;

  Entry* entry = findEntry(table->data, table->capacity, key);
  if (entry->key == NULL) return false;

  *value = entry->value;
  return true;
}

bool tableDelete(Table* table, ObjString* key) {
  if (table->size == 0) return false;

  // Find the entry to be deleted
  Entry* entry = findEntry(table->data, table->capacity, key);
  if (entry->key == NULL) return false;

  // Insert a tombstone value
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}

ObjString* tableFindString(Table* table, const char* chars,
    int length, uint32_t hash) {
  if (table->size == 0) return NULL;

  uint32_t index = hash % table->capacity;
  for (;;) {
    Entry* entry = table->data + index;
    if (entry->key == NULL) {
      // Stop if an empty non-tombstone entry is found
      if (IS_NIL(entry->value)) return NULL;
    } else if (entry->key->length == length &&
        entry->key->hash == hash &&
        memcmp(entry->key->chars, chars, length) == 0) {
      // Found it
      return entry->key;
    }
    index = (index + 1) % table->capacity;
  }

}
