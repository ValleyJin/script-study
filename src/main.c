#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "error.h"
#include "memory.h"
#include "parser.h"

static char* readFile(const char* path) {
  FILE* f = fopen(path, "rb");
  if (f == NULL) {
    fprintf(stderr, "파일을 열 수 없다: %s\n", path);
    exit(SL_EX_USAGE);
  }
  fseek(f, 0L, SEEK_END);
  long size = ftell(f);
  rewind(f);
  char* buf = ALLOCATE(char, size + 1);
  size_t got = fread(buf, 1, (size_t)size, f);
  buf[got] = '\0';
  fclose(f);
  return buf;
}

static void usage(void) {
  fputs("쓰임: sl <갈래> <파일.sl>\n"
        "  --print   구문 트리를 다시 파싱되는 sl 소스로 찍는다\n"
        "  --check   검사 패스만 돌린다\n"
        "  --walk    트리를 걷는 실행기로 돌린다 (2단계에서 붙인다)\n"
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

  if (strcmp(mode, "--walk") == 0) {
    fputs("--walk 은 1부 2단계에서 붙인다\n", stderr);
    return SL_EX_USAGE;
  }
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
  } else if (strcmp(mode, "--check") != 0) {
    usage();
  }

  if (stats) {
    fprintf(stderr, "할당 횟수 %zu, 누적 바이트 %zu\n", slAllocCount(), slAllocBytes());
  }
  return SL_EX_OK;
}
