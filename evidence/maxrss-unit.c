#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
int main(int argc, char** argv) {
  size_t mb = (argc > 1) ? (size_t)atoi(argv[1]) : 200;
  size_t n = mb * 1024 * 1024;
  volatile unsigned char sink = 0;
  unsigned char* p = malloc(n);
  if (!p) return 1;
  for (size_t i = 0; i < n; i += 4096) p[i] = (unsigned char)(i & 0xff);
  for (size_t i = 0; i < n; i += 4096) sink ^= p[i];   /* 지워지지 않게 쓴다 */
  struct rusage ru; getrusage(RUSAGE_SELF, &ru);
  printf("%zu MB 만진 뒤 ru_maxrss = %ld  (sink=%d)\n", mb, (long)ru.ru_maxrss, sink);
  printf("  바이트로 읽으면 %.1f MB\n", ru.ru_maxrss/1048576.0);
  printf("  KB로  읽으면 %.1f MB\n", ru.ru_maxrss/1024.0);
  return 0;
}
