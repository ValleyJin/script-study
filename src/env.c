#include "env.h"
#include "memory.h"

static Table globals;
static bool globalsReady = false;

Table* slGlobals(void) {
  if (!globalsReady) {
    slTableInit(&globals);
    globalsReady = true;
  }
  return &globals;
}

Env* slEnvNew(Env* enclosing) {
  Env* e = ALLOCATE(Env, 1);
  e->enclosing = enclosing;
  e->names = NULL;
  e->values = NULL;
  e->count = 0;
  e->capacity = 0;
  return e;
}

void slEnvDefine(Env* env, ObjString* name, Value value) {
  if (env == NULL) {
    slTableSet(slGlobals(), name, value);
    return;
  }
  if (env->count + 1 > env->capacity) {
    int old = env->capacity;
    env->capacity = GROW_CAPACITY(old);
    env->names = GROW_ARRAY(ObjString*, env->names, old, env->capacity);
    env->values = GROW_ARRAY(Value, env->values, old, env->capacity);
  }
  env->names[env->count] = name;
  env->values[env->count] = value;
  env->count++;
}

/* 이름은 인터닝했으므로 포인터만 견준다. */
static int indexOf(Env* e, ObjString* name) {
  for (int i = e->count - 1; i >= 0; i--) {
    if (e->names[i] == name) return i;
  }
  return -1;
}

bool slEnvGet(Env* env, ObjString* name, Value* out) {
  for (Env* e = env; e != NULL; e = e->enclosing) {
    int i = indexOf(e, name);
    if (i >= 0) { *out = e->values[i]; return true; }
  }
  return slTableGet(slGlobals(), name, out);
}

bool slEnvSet(Env* env, ObjString* name, Value value) {
  for (Env* e = env; e != NULL; e = e->enclosing) {
    int i = indexOf(e, name);
    if (i >= 0) { e->values[i] = value; return true; }
  }
  Value dummy;
  if (slTableGet(slGlobals(), name, &dummy)) {
    slTableSet(slGlobals(), name, value);
    return true;
  }
  return false;
}
