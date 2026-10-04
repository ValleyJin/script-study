#ifndef SL_VALUE_H
#define SL_VALUE_H

#include "common.h"
#include "lexer.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

/* study/02 의 값 표현. 이 기계에서 16바이트다. 태그가 4바이트고 뒤에 4바이트가 빈다.
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

/* study/02 의 "print 가 찍는 글자" 표. 줄바꿈은 붙이지 않는다. */
void slPrintValue(FILE* out, Value v);

/* 연산자의 타입 검사와 오류 문구를 여기 둔다. 두 갈래가 이것을 함께 쓴다.
   각자 짜면 문구가 어긋나고 4단계의 글자 대조가 깨진다.
   타입이 어긋나면 런타임 오류를 내므로 돌아오지 않는다. */
Value slAdd(Value a, Value b, int line);          /* 수 둘 또는 문자열 둘 */
Value slArith(TokenType op, Value a, Value b, int line);
Value slCompare(TokenType op, Value a, Value b, int line);
Value slNegate(Value a, int line);

/* 호출할 때 걸러야 하는 것. 둘 다 두 갈래가 함께 쓴다. */
void slCheckCallable(Value callee, int line);
void slCheckArity(int want, int got, int line);

#endif
