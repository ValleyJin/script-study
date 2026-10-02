#include <string.h>

#include "error.h"
#include "lexer.h"

static const char* start;
static const char* cur;
static int line;

void slLexInit(const char* source) {
  start = source;
  cur = source;
  line = 1;
}

static bool atEnd(void)   { return *cur == '\0'; }
static char advance(void) { return *cur++; }
static char peek(void)    { return *cur; }
static char peekNext(void){ return atEnd() ? '\0' : cur[1]; }

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
static bool isDigit(char c) { return c >= '0' && c <= '9'; }

static Token make(TokenType type) {
  Token t;
  t.type = type;
  t.start = start;
  t.length = (int)(cur - start);
  t.line = line;
  return t;
}

static bool match(char expected) {
  if (atEnd() || *cur != expected) return false;
  cur++;
  return true;
}

static void skipBlank(void) {
  for (;;) {
    char c = peek();
    if (c == ' ' || c == '\t' || c == '\r') { advance(); }
    else if (c == '\n') { line++; advance(); }
    else if (c == '/' && peekNext() == '/') {
      while (!atEnd() && peek() != '\n') advance();
    } else {
      return;
    }
  }
}

/* 예약어는 먼저 IDENT 로 읽은 뒤 표에서 찾아 바꾼다. */
static TokenType identType(void) {
  static const struct { const char* word; TokenType type; } table[] = {
    {"var", T_VAR}, {"fun", T_FUN}, {"return", T_RETURN},
    {"if", T_IF}, {"else", T_ELSE}, {"while", T_WHILE}, {"print", T_PRINT},
    {"and", T_AND}, {"or", T_OR},
    {"true", T_TRUE}, {"false", T_FALSE}, {"nil", T_NIL},
  };
  int len = (int)(cur - start);
  for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
    if ((int)strlen(table[i].word) == len &&
        memcmp(start, table[i].word, (size_t)len) == 0) {
      return table[i].type;
    }
  }
  return T_IDENT;
}

static Token number(void) {
  while (isDigit(peek())) advance();
  if (peek() == '.') {
    /* `1.` 은 어휘 오류다. `.` 는 sl 의 토큰이 아니다. */
    if (!isDigit(peekNext())) slFatal(line, "소수점 뒤에 숫자가 없다");
    advance();
    while (isDigit(peek())) advance();
  }
  return make(T_NUMBER);
}

static Token string(void) {
  while (!atEnd() && peek() != '"') {
    if (peek() == '\n') slFatal(line, "문자열이 닫히지 않았다");
    advance();
  }
  if (atEnd()) slFatal(line, "문자열이 닫히지 않았다");
  advance();                      /* 닫는 따옴표 */
  return make(T_STRING);
}

Token slLexNext(void) {
  skipBlank();
  start = cur;
  if (atEnd()) return make(T_EOF);

  char c = advance();
  if (isAlpha(c)) {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return make(identType());
  }
  if (isDigit(c)) return number();

  switch (c) {
    case '(': return make(T_LPAREN);
    case ')': return make(T_RPAREN);
    case '{': return make(T_LBRACE);
    case '}': return make(T_RBRACE);
    case ',': return make(T_COMMA);
    case ';': return make(T_SEMI);
    case '+': return make(T_PLUS);
    case '-': return make(T_MINUS);
    case '*': return make(T_STAR);
    case '/': return make(T_SLASH);
    case '"': return string();
    /* 긴 쪽을 먼저 맞춘다 */
    case '=': return make(match('=') ? T_EQ_EQ      : T_EQ);
    case '!': return make(match('=') ? T_BANG_EQ    : T_BANG);
    case '<': return make(match('=') ? T_LESS_EQ    : T_LESS);
    case '>': return make(match('=') ? T_GREATER_EQ : T_GREATER);
    default: break;
  }
  slFatal(line, "알 수 없는 문자: '%c'", c);
  return make(T_EOF);   /* 닿지 않는다 */
}

const char* slTokenName(TokenType t) {
  switch (t) {
    case T_LPAREN: return "'('";
    case T_RPAREN: return "')'";
    case T_LBRACE: return "'{'";
    case T_RBRACE: return "'}'";
    case T_COMMA:  return "','";
    case T_SEMI:   return "';'";
    case T_PLUS:   return "'+'";
    case T_MINUS:  return "'-'";
    case T_STAR:   return "'*'";
    case T_SLASH:  return "'/'";
    case T_LESS:   return "'<'";
    case T_GREATER:return "'>'";
    case T_EQ:     return "'='";
    case T_BANG:   return "'!'";
    case T_EQ_EQ:  return "'=='";
    case T_BANG_EQ:return "'!='";
    case T_LESS_EQ:return "'<='";
    case T_GREATER_EQ: return "'>='";
    case T_NUMBER: return "수";
    case T_STRING: return "문자열";
    case T_IDENT:  return "이름";
    case T_VAR:    return "'var'";
    case T_FUN:    return "'fun'";
    case T_RETURN: return "'return'";
    case T_IF:     return "'if'";
    case T_ELSE:   return "'else'";
    case T_WHILE:  return "'while'";
    case T_PRINT:  return "'print'";
    case T_AND:    return "'and'";
    case T_OR:     return "'or'";
    case T_TRUE:   return "'true'";
    case T_FALSE:  return "'false'";
    case T_NIL:    return "'nil'";
    case T_EOF:    return "파일 끝";
  }
  return "?";
}
