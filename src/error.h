#ifndef SL_ERROR_H
#define SL_ERROR_H

#include <setjmp.h>

#include "common.h"

/* study/02 의 "오류 문구와 종료 코드" 표 */
#define SL_EX_OK    0
#define SL_EX_USAGE 64
#define SL_EX_DATA  65   /* 어휘, 구문, 검사 오류 */
#define SL_EX_RUN   70   /* 런타임 오류 */

/* 모든 오류를 표준 오류로 내고 "[줄] 오류: 문구" 꼴을 쓴다.
   어휘, 구문, 검사 오류는 첫 오류에서 바로 끝낸다. */
void slFatal(int line, const char* fmt, ...);

/* 런타임 오류는 최상단으로 뛴다. study/02 에서 오류 플래그 대신 이 길을 골랐다.
   eval 을 부르는 자리마다 검사 분기를 붙이면 1부가 재려는 속도 자체가 달라진다. */
extern jmp_buf slRuntimeBand;
void slRuntimeError(int line, const char* fmt, ...);

#endif
