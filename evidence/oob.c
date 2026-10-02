#include <stdlib.h>
#include <stdio.h>
int main(void) {
  char* p = malloc(16);
  p[16] = 'x';            /* 한 바이트 넘긴다 */
  printf("안 잡혔다: %c\n", p[16]);
  return 0;
}
