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

/* docs/02 의 "두 한도의 값". 잠정값이다. 2단계의 expected/ 를 만들기 전에 재어 확정한다.
   VM 은 최상위 스크립트도 프레임 하나를 차지하므로 허용하는 sl 호출 깊이가
   FRAMES_MAX - 1 이다. 트리 순회도 같은 자리에서 걸리도록 맞춘다. */
#define SL_FRAMES_MAX 1024
#define SL_MAX_CALL   (SL_FRAMES_MAX - 1)

#endif
