#ifndef SL_LEXER_H
#define SL_LEXER_H

#include "common.h"

typedef enum {
  /* 한 글자 토큰 열넷 */
  T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE,
  T_COMMA, T_SEMI,
  T_PLUS, T_MINUS, T_STAR, T_SLASH,
  T_LESS, T_GREATER, T_EQ, T_BANG,
  /* 두 글자 토큰 넷. 긴 쪽을 먼저 맞춘다 */
  T_EQ_EQ, T_BANG_EQ, T_LESS_EQ, T_GREATER_EQ,
  /* 리터럴 */
  T_NUMBER, T_STRING, T_IDENT,
  /* 예약어 열두 개 */
  T_VAR, T_FUN, T_RETURN, T_IF, T_ELSE, T_WHILE, T_PRINT,
  T_AND, T_OR, T_TRUE, T_FALSE, T_NIL,
  T_EOF,
} TokenType;

typedef struct {
  TokenType type;
  const char* start;
  int length;
  int line;
} Token;

void slLexInit(const char* source);
Token slLexNext(void);

/* 오류 문구에 쓴다 */
const char* slTokenName(TokenType t);

#endif
