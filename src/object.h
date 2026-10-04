#ifndef SL_OBJECT_H
#define SL_OBJECT_H

#include "common.h"
#include "value.h"

/* 종류 태그와 구조체 선언을 모두 여기 둔다. 한 실행 파일에 두 갈래를 넣으므로
   ObjType 이 한 벌이어야 한다. ObjUpvalue 는 Value 에 담기지 않는다. */
typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
  OBJ_CLOSURE,
  OBJ_UPVALUE,
} ObjType;

struct Obj {
  ObjType type;
  Obj* next;          /* 만든 순서대로 이어 둔다 */
};

struct ObjString {
  Obj obj;
  int length;
  uint32_t hash;
  char* chars;        /* NUL 로 끝낸다 */
};

typedef struct Chunk Chunk;   /* VM 만 쓴다. 3단계에서 채운다 */
typedef struct Node Node;     /* 트리 순회가 쓴다 */

/* 두 갈래가 함께 쓴다. 쓰지 않는 쪽 포인터를 비워 둔다. */
typedef struct {
  Obj obj;
  ObjString* name;    /* 익명이면 NULL */
  int arity;
  Node* fnNode;       /* 트리 순회가 쓴다. N_FUNDECL 이나 N_FUNEXPR 노드다 */
  Chunk* chunk;       /* VM 이 쓴다 */
  int upvalueCount;
} ObjFunction;

/* 두 갈래가 각자 쓴다. 둘 다 ObjFunction* 을 든다. 공유 출력과 같음 판정이 그것을 읽는다.
   그 밖에 트리 순회는 환경을, VM 은 upvalue 배열을 가리킨다. */
typedef struct Env Env;

typedef struct {
  Obj obj;
  ObjFunction* fn;
  Env* env;           /* 트리 순회가 쓴다 */
} ObjClosure;

#define OBJ_TYPE(v)     (AS_OBJ(v)->type)
#define IS_STRING(v)    (IS_OBJ(v) && OBJ_TYPE(v) == OBJ_STRING)
#define IS_FUNCTION(v)  (IS_OBJ(v) && OBJ_TYPE(v) == OBJ_FUNCTION)
#define IS_CLOSURE(v)   (IS_OBJ(v) && OBJ_TYPE(v) == OBJ_CLOSURE)
#define AS_STRING(v)    ((ObjString*)AS_OBJ(v))
#define AS_CSTRING(v)   (AS_STRING(v)->chars)
#define AS_FUNCTION(v)  ((ObjFunction*)AS_OBJ(v))
#define AS_CLOSURE(v)   ((ObjClosure*)AS_OBJ(v))

/* 인터닝한다. 내용이 같으면 같은 객체를 돌려준다. */
ObjString* slCopyString(const char* chars, int length);

ObjFunction* slNewFunction(ObjString* name, int arity);

/* study/02 대로 fun 선언이나 식을 평가할 때마다 새로 만든다. 잡은 변수가 없어도
   다시 쓰지 않는다. 안 그러면 make() == make() 의 답이 두 갈래에서 갈린다. */
ObjClosure* slNewClosure(ObjFunction* fn, Env* env);

uint32_t slHashString(const char* key, int length);

#endif
