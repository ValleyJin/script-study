#ifndef SL_WALK_H
#define SL_WALK_H

#include "ast.h"

/* 갈래 A. 구문 트리를 재귀로 돈다. sl 의 호출 깊이가 호스트 C 스택의 깊이로
   그대로 옮겨 간다. 그래서 깊이를 세어 SL_MAX_CALL 에서 런타임 오류를 낸다. */
void slWalk(NodeList* program);

#endif
