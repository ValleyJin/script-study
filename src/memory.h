#ifndef SL_MEMORY_H
#define SL_MEMORY_H

#include "common.h"

/* 모든 할당이 이 함수를 지난다. study/02 의 메모리 절대로 아무것도 해제하지 않으므로
   누적 바이트가 곧 최대 할당량이다. 상주 메모리는 이 수로 알 수 없다. */
void* reallocate(void* ptr, size_t oldSize, size_t newSize);

size_t slAllocCount(void);
size_t slAllocBytes(void);

#define ALLOCATE(type, count) \
    ((type*)reallocate(NULL, 0, sizeof(type) * (size_t)(count)))

#define GROW_CAPACITY(cap) ((cap) < 8 ? 8 : (cap) * 2)

#define GROW_ARRAY(type, ptr, oldCount, newCount) \
    ((type*)reallocate(ptr, sizeof(type) * (size_t)(oldCount), \
                            sizeof(type) * (size_t)(newCount)))

#endif
