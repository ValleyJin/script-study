#include <string.h>

#include "memory.h"
#include "table.h"

#define MAX_LOAD 0.75

void slTableInit(Table* t) {
  t->count = 0;
  t->capacity = 0;
  t->entries = NULL;
}

static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
  uint32_t index = key->hash & (uint32_t)(capacity - 1);
  for (;;) {
    Entry* e = &entries[index];
    if (e->key == NULL || e->key == key) return e;
    index = (index + 1) & (uint32_t)(capacity - 1);
  }
}

static void growTable(Table* t, int capacity) {
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }
  for (int i = 0; i < t->capacity; i++) {
    Entry* src = &t->entries[i];
    if (src->key == NULL) continue;
    Entry* dst = findEntry(entries, capacity, src->key);
    dst->key = src->key;
    dst->value = src->value;
  }
  t->entries = entries;
  t->capacity = capacity;
}

bool slTableGet(Table* t, ObjString* key, Value* out) {
  if (t->count == 0) return false;
  Entry* e = findEntry(t->entries, t->capacity, key);
  if (e->key == NULL) return false;
  *out = e->value;
  return true;
}

bool slTableSet(Table* t, ObjString* key, Value value) {
  if ((double)(t->count + 1) > (double)t->capacity * MAX_LOAD) {
    growTable(t, GROW_CAPACITY(t->capacity));
  }
  Entry* e = findEntry(t->entries, t->capacity, key);
  bool isNew = (e->key == NULL);
  if (isNew) t->count++;
  e->key = key;
  e->value = value;
  return isNew;
}

ObjString* slTableFindString(Table* t, const char* chars, int length, uint32_t hash) {
  if (t->count == 0) return NULL;
  uint32_t index = hash & (uint32_t)(t->capacity - 1);
  for (;;) {
    Entry* e = &t->entries[index];
    if (e->key == NULL) return NULL;
    if (e->key->length == length && e->key->hash == hash &&
        memcmp(e->key->chars, chars, (size_t)length) == 0) {
      return e->key;
    }
    index = (index + 1) & (uint32_t)(t->capacity - 1);
  }
}
