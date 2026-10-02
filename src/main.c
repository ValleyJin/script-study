#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "error.h"
#include "memory.h"
#include "parser.h"
#include "walk.h"

/* 크기를 미리 묻지 않고 조금씩 늘려 가며 읽는다. fseek 과 ftell 에 기대면 이름 있는
   파이프나 /dev/stdin 에서 ftell 이 -1 을 돌려주고, 그러면 크기가 0 이 되어
   reallocate 가 NULL 을 주고 NUL 을 쓰는 자리에서 죽는다. */
static char* readFile(const char* path) {
  FILE* f = fopen(path, "rb");
  if (f == NULL) {
    fprintf(stderr, "파일을 열 수 없다: %s\n", path);
    exit(SL_EX_USAGE);
  }
  size_t cap = 4096;
  size_t len = 0;
  char* buf = ALLOCATE(char, cap);
  for (;;) {
    if (len + 1 >= cap) {
      size_t old = cap;
      cap *= 2;
      buf = GROW_ARRAY(char, buf, old, cap);
    }
    size_t got = fread(buf + len, 1, cap - len - 1, f);
    len += got;
    if (got == 0) break;
  }
  if (ferror(f)) {
    fprintf(stderr, "파일을 읽을 수 없다: %s\n", path);
    exit(SL_EX_USAGE);
  }
  fclose(f);
  buf[len] = '\0';

  /* sl 소스에 NUL 은 올 수 없다. 그냥 두면 렉서가 먼저 만난 NUL 에서 멈추고
     프로그램의 뒤 절반을 말없이 버린다. 조용히 틀린 답을 내는 꼴이라 여기서 거른다. */
  for (size_t i = 0; i < len; i++) {
    if (buf[i] == '\0') {
      int line = 1;
      for (size_t j = 0; j < i; j++) if (buf[j] == '\n') line++;
      slFatal(line, "알 수 없는 문자: NUL");
    }
  }
  return buf;
}

static void usage(void) {
  fputs("쓰임: sl <갈래> <파일.sl>\n"
        "  --print   구문 트리를 다시 파싱되는 sl 소스로 찍는다\n"
        "  --check   검사 패스만 돌린다\n"
        "  --walk    트리를 걷는 실행기로 돌린다\n"
        "  --vm      바이트코드 VM 으로 돌린다 (3단계에서 붙인다)\n"
        "  --stats   할당 횟수와 누적 바이트를 함께 찍는다\n", stderr);
  exit(SL_EX_USAGE);
}

int main(int argc, char** argv) {
  const char* mode = NULL;
  const char* path = NULL;
  bool stats = false;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--stats") == 0) stats = true;
    else if (strncmp(argv[i], "--", 2) == 0) {
      if (mode != NULL) usage();
      mode = argv[i];
    } else {
      if (path != NULL) usage();
      path = argv[i];
    }
  }
  if (mode == NULL || path == NULL) usage();

  if (strcmp(mode, "--vm") == 0) {
    fputs("--vm 은 1부 3단계에서 붙인다\n", stderr);
    return SL_EX_USAGE;
  }

  char* source = readFile(path);
  NodeList program;
  slParse(source, &program);
  slCheck(&program);          /* --print 도 검사 패스를 먼저 돌린다 */

  if (strcmp(mode, "--print") == 0) {
    slPrintProgram(stdout, &program);
  } else if (strcmp(mode, "--walk") == 0) {
    /* 런타임 오류는 여기로 뛴다 */
    if (setjmp(slRuntimeBand) != 0) {
      fflush(stdout);
      return SL_EX_RUN;
    }
    slWalk(&program);
    fflush(stdout);
  } else if (strcmp(mode, "--check") != 0) {
    usage();
  }

  if (stats) {
    fprintf(stderr, "할당 횟수 %zu, 누적 바이트 %zu\n", slAllocCount(), slAllocBytes());
  }
  return SL_EX_OK;
}
