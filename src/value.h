#ifndef SL_VALUE_H
#define SL_VALUE_H

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

/* docs/02 의 값 표현. 이 기계에서 16바이트다. 태그가 4바이트고 뒤에 4바이트가 빈다.
   재는 코드는 evidence/value-size.c 에 있다. */
typedef enum { VAL_NIL, VAL_BOOL, VAL_NUM, VAL_OBJ } ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj* obj;
  } as;
} Value;

#define NIL_VAL         ((Value){VAL_NIL,  {.number = 0}})
#define BOOL_VAL(b)     ((Value){VAL_BOOL, {.boolean = (b)}})
#define NUM_VAL(n)      ((Value){VAL_NUM,  {.number = (n)}})
#define OBJ_VAL(o)      ((Value){VAL_OBJ,  {.obj = (Obj*)(o)}})

#define IS_NIL(v)       ((v).type == VAL_NIL)
#define IS_BOOL(v)      ((v).type == VAL_BOOL)
#define IS_NUM(v)       ((v).type == VAL_NUM)
#define IS_OBJ(v)       ((v).type == VAL_OBJ)

#define AS_BOOL(v)      ((v).as.boolean)
#define AS_NUM(v)       ((v).as.number)
#define AS_OBJ(v)       ((v).as.obj)

/* false 와 nil 만 거짓이다. 0 과 빈 문자열은 참이다. */
bool slTruthy(Value v);

/* 타입이 다르면 늘 거짓이다. 수는 IEEE 754 대로 견주므로 nan 은 자기 자신과도 다르다.
   문자열은 인터닝하므로 내용이 같으면 같은 객체다. */
bool slEqual(Value a, Value b);

/* docs/02 의 "print 가 찍는 글자" 표. 줄바꿈은 붙이지 않는다. */
void slPrintValue(FILE* out, Value v);

#endif
