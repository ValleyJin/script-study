#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
typedef enum { VAL_NIL, VAL_BOOL, VAL_NUM, VAL_OBJ } ValueType;
typedef struct Obj Obj;
typedef struct { ValueType type; union { bool boolean; double number; Obj* obj; } as; } Value;
int main(void) {
  printf("sizeof(ValueType) = %zu\n", sizeof(ValueType));
  printf("sizeof(Value)     = %zu\n", sizeof(Value));
  printf("_Alignof(Value)   = %zu\n", _Alignof(Value));
  printf("offsetof(type)    = %zu\n", offsetof(Value, type));
  printf("offsetof(as)      = %zu\n", offsetof(Value, as));
  printf("빈틈              = %zu바이트\n", offsetof(Value,as) - sizeof(ValueType));
  return 0;
}
