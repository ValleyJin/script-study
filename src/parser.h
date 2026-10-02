#ifndef SL_PARSER_H
#define SL_PARSER_H

#include "ast.h"

/* 소스를 구문 트리로 만든다. 첫 오류에서 바로 끝낸다(종료 코드 65). */
void slParse(const char* source, NodeList* out);

#endif
