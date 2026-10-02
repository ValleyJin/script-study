#include <stdlib.h>
#include <string.h>

#include "memory.h"

static size_t allocCount = 0;
static size_t allocBytes = 0;

void* reallocate(void* ptr, size_t oldSize, size_t newSize) {
  if (newSize == 0) return NULL;          /* 해제하지 않는다 */
  allocCount++;
  if (newSize > oldSize) allocBytes += newSize - oldSize;

  void* out = realloc(ptr, newSize);
  if (out == NULL) {
    fputs("메모리가 모자라다\n", stderr);
    exit(70);
  }
  return out;
}

size_t slAllocCount(void) { return allocCount; }
size_t slAllocBytes(void) { return allocBytes; }
