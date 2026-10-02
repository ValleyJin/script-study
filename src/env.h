#ifndef SL_ENV_H
#define SL_ENV_H

#include "common.h"
#include "object.h"
#include "table.h"

/* docs/02 대로 (이름, Value) 짝의 선형 배열이다. 블록 환경에 변수가 서너 개뿐일 때는
   선형 탐색이 해시보다 빠르다. 체인 맨 끝의 전역만 해시 표다.
   실험 2에서 이 배열을 슬롯 배열로 바꾼다. */
typedef struct Env {
  struct Env* enclosing;    /* NULL 이면 다음은 전역 표다 */
  ObjString** names;
  Value* values;
  int count;
  int capacity;
} Env;

Env* slEnvNew(Env* enclosing);

/* 지금 유효범위에 새 묶임을 만든다. env 가 NULL 이면 전역 표에 넣는다. */
void slEnvDefine(Env* env, ObjString* name, Value value);

/* 체인을 거슬러 올라가 처음 만나는 묶임을 읽는다. 못 찾으면 false 다. */
bool slEnvGet(Env* env, ObjString* name, Value* out);

/* 체인에서 찾은 묶임을 고친다. 못 찾으면 false 다. 새로 만들지 않는다. */
bool slEnvSet(Env* env, ObjString* name, Value value);

Table* slGlobals(void);

#endif
