#ifndef SL_TABLE_H
#define SL_TABLE_H

#include "common.h"
#include "object.h"

/* 열린 주소 지정 해시 표. 전역 변수와 문자열 인터닝에 쓴다. */
typedef struct {
  ObjString* key;    /* NULL 이면 빈 칸 */
  Value value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} Table;

void slTableInit(Table* t);
bool slTableGet(Table* t, ObjString* key, Value* out);
bool slTableSet(Table* t, ObjString* key, Value value);

/* 인터닝 전용. 같은 내용의 키가 이미 있으면 그것을 돌려준다. */
ObjString* slTableFindString(Table* t, const char* chars, int length, uint32_t hash);

#endif
