#include <math.h>
#include <string.h>

#include "error.h"
#include "lexer.h"
#include "memory.h"
#include "object.h"
#include "value.h"

bool slTruthy(Value v) {
  if (IS_NIL(v)) return false;
  if (IS_BOOL(v)) return AS_BOOL(v);
  return true;
}

bool slEqual(Value a, Value b) {
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_NIL:  return true;
    case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NUM:  return AS_NUM(a) == AS_NUM(b);   /* nan != nan */
    case VAL_OBJ:  return AS_OBJ(a) == AS_OBJ(b);   /* 문자열은 인터닝했다 */
  }
  return false;
}

static void printNumber(FILE* out, double d) {
  if (isnan(d)) { fputs("nan", out); return; }
  if (isinf(d)) { fputs(d > 0 ? "inf" : "-inf", out); return; }
  if (d == floor(d) && fabs(d) < 1e15) {
    fprintf(out, "%.0f", d);          /* 소수부가 0이면 정수처럼 */
  } else {
    fprintf(out, "%g", d);
  }
}

void slPrintValue(FILE* out, Value v) {
  switch (v.type) {
    case VAL_NIL:  fputs("nil", out); break;
    case VAL_BOOL: fputs(AS_BOOL(v) ? "true" : "false", out); break;
    case VAL_NUM:  printNumber(out, AS_NUM(v)); break;
    case VAL_OBJ:
      switch (OBJ_TYPE(v)) {
        case OBJ_STRING:
          fputs(AS_CSTRING(v), out);           /* 따옴표 없이 내용만 */
          break;
        case OBJ_FUNCTION: {
          ObjFunction* f = AS_FUNCTION(v);
          if (f->name == NULL) fputs("<fn>", out);
          else fprintf(out, "<fn %s>", f->name->chars);
          break;
        }
        case OBJ_CLOSURE: {
          ObjFunction* f = AS_CLOSURE(v)->fn;
          if (f->name == NULL) fputs("<fn>", out);
          else fprintf(out, "<fn %s>", f->name->chars);
          break;
        }
        default:
          fputs("<obj>", out);
          break;
      }
      break;
  }
}

/* 오류 문구에 쓸 연산자 글자. 프린터의 것과 따로 두지 않으려 했으나 ast.c 는
   구문 트리를 찍는 일만 하므로 여기에 작은 표를 하나 더 둔다. */
static const char* opChars(TokenType op) {
  switch (op) {
    case T_PLUS: return "+";    case T_MINUS: return "-";
    case T_STAR: return "*";    case T_SLASH: return "/";
    case T_LESS: return "<";    case T_LESS_EQ: return "<=";
    case T_GREATER: return ">"; case T_GREATER_EQ: return ">=";
    default: return "?";
  }
}

Value slAdd(Value a, Value b, int line) {
  if (IS_NUM(a) && IS_NUM(b)) return NUM_VAL(AS_NUM(a) + AS_NUM(b));
  if (IS_STRING(a) && IS_STRING(b)) {
    ObjString* x = AS_STRING(a);
    ObjString* y = AS_STRING(b);
    int len = x->length + y->length;
    char* buf = ALLOCATE(char, len + 1);
    memcpy(buf, x->chars, (size_t)x->length);
    memcpy(buf + x->length, y->chars, (size_t)y->length);
    buf[len] = '\0';
    Value out = OBJ_VAL(slCopyString(buf, len));   /* 인터닝한다 */
    return out;
  }
  slRuntimeError(line, "수 둘이나 문자열 둘이 아닌 값에 '+' 를 썼다");
  return NIL_VAL;
}

Value slArith(TokenType op, Value a, Value b, int line) {
  if (!IS_NUM(a) || !IS_NUM(b)) {
    slRuntimeError(line, "수가 아닌 값에 연산자를 썼다: %s", opChars(op));
  }
  double x = AS_NUM(a), y = AS_NUM(b);
  switch (op) {
    case T_MINUS: return NUM_VAL(x - y);
    case T_STAR:  return NUM_VAL(x * y);
    case T_SLASH: return NUM_VAL(x / y);   /* 1/0 은 inf 다. 오류로 보지 않는다 */
    default: break;
  }
  slRuntimeError(line, "수가 아닌 값에 연산자를 썼다: %s", opChars(op));
  return NIL_VAL;
}

Value slCompare(TokenType op, Value a, Value b, int line) {
  if (!IS_NUM(a) || !IS_NUM(b)) {
    slRuntimeError(line, "수가 아닌 값에 연산자를 썼다: %s", opChars(op));
  }
  double x = AS_NUM(a), y = AS_NUM(b);
  switch (op) {
    case T_LESS:       return BOOL_VAL(x < y);
    case T_LESS_EQ:    return BOOL_VAL(x <= y);
    case T_GREATER:    return BOOL_VAL(x > y);
    case T_GREATER_EQ: return BOOL_VAL(x >= y);
    default: break;
  }
  slRuntimeError(line, "수가 아닌 값에 연산자를 썼다: %s", opChars(op));
  return NIL_VAL;
}

Value slNegate(Value a, int line) {
  if (!IS_NUM(a)) slRuntimeError(line, "수가 아닌 값에 연산자를 썼다: -");
  return NUM_VAL(-AS_NUM(a));
}

void slCheckCallable(Value callee, int line) {
  if (!IS_OBJ(callee) || OBJ_TYPE(callee) != OBJ_CLOSURE) {
    slRuntimeError(line, "함수가 아닌 값을 불렀다");
  }
}

void slCheckArity(int want, int got, int line) {
  if (want != got) {
    slRuntimeError(line, "인자 개수가 맞지 않는다: %d개를 받는 함수에 %d개를 넘겼다",
                   want, got);
  }
}
