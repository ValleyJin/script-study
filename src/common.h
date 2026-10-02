#ifndef SL_COMMON_H
#define SL_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* docs/02 의 공유 검사 패스가 거는 상한. 잠정값이다. */
#define SL_MAX_LOCALS 255   /* 인자와 지역 변수를 합쳐 */
#define SL_MAX_ARGS   255   /* 한 호출의 인자 */
#define SL_MAX_DEPTH  128   /* 한 식의 중첩 깊이 */

#endif
