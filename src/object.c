#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"

static Table strings;          /* 인터닝 표 */
static bool stringsReady = false;
static Obj* objects = NULL;

uint32_t slHashString(const char* key, int length) {
  uint32_t hash = 2166136261u;          /* FNV-1a */
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619u;
  }
  return hash;
}

static Obj* allocObj(size_t size, ObjType type) {
  Obj* o = (Obj*)reallocate(NULL, 0, size);
  o->type = type;
  o->next = objects;
  objects = o;
  return o;
}

ObjString* slCopyString(const char* chars, int length) {
  if (!stringsReady) {
    slTableInit(&strings);
    stringsReady = true;
  }
  uint32_t hash = slHashString(chars, length);
  ObjString* found = slTableFindString(&strings, chars, length, hash);
  if (found != NULL) return found;

  char* heap = ALLOCATE(char, length + 1);
  memcpy(heap, chars, (size_t)length);
  heap[length] = '\0';

  ObjString* s = (ObjString*)allocObj(sizeof(ObjString), OBJ_STRING);
  s->length = length;
  s->hash = hash;
  s->chars = heap;
  slTableSet(&strings, s, NIL_VAL);
  return s;
}

ObjFunction* slNewFunction(ObjString* name, int arity) {
  ObjFunction* f = (ObjFunction*)allocObj(sizeof(ObjFunction), OBJ_FUNCTION);
  f->name = name;
  f->arity = arity;
  f->body = NULL;
  f->chunk = NULL;
  f->upvalueCount = 0;
  return f;
}
