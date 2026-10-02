#ifndef SL_ERROR_H
#define SL_ERROR_H

#include "common.h"

/* docs/02 의 "오류 문구와 종료 코드" 표 */
#define SL_EX_OK    0
#define SL_EX_USAGE 64
#define SL_EX_DATA  65   /* 어휘, 구문, 검사 오류 */
#define SL_EX_RUN   70   /* 런타임 오류 */

/* 모든 오류를 표준 오류로 내고 "[줄] 오류: 문구" 꼴을 쓴다.
   어휘, 구문, 검사 오류는 첫 오류에서 바로 끝낸다. */
void slFatal(int line, const char* fmt, ...);

#endif
