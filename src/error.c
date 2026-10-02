#include <stdarg.h>
#include <stdlib.h>

#include "error.h"

void slFatal(int line, const char* fmt, ...) {
  fprintf(stderr, "[%d] 오류: ", line);
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(SL_EX_DATA);
}

jmp_buf slRuntimeBand;

void slRuntimeError(int line, const char* fmt, ...) {
  fprintf(stderr, "[%d] 오류: ", line);
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  longjmp(slRuntimeBand, 1);
}
