#include <math.h>
#include <string.h>

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
        default:
          fputs("<obj>", out);
          break;
      }
      break;
  }
}
