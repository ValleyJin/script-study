#ifndef SL_AST_H
#define SL_AST_H

#include "common.h"
#include "lexer.h"
#include "object.h"

typedef enum {
  /* 식 */
  N_NUM, N_STR, N_TRUE, N_FALSE, N_NIL,
  N_IDENT, N_ASSIGN, N_LOGICAL, N_BINARY, N_UNARY, N_CALL, N_FUNEXPR,
  /* 문 */
  N_VARDECL, N_FUNDECL, N_EXPRSTMT, N_PRINT,
  N_IF, N_WHILE, N_RETURN, N_BLOCK,
} NodeKind;

typedef struct Node Node;

typedef struct {
  Node** items;
  int count;
  int capacity;
} NodeList;

/* 함수 선언과 익명 함수가 함께 쓴다 */
typedef struct {
  ObjString* name;        /* 익명이면 NULL */
  ObjString** params;
  int paramCount;
  Node* body;             /* N_BLOCK */
  ObjFunction* fn;        /* 트리 순회가 노드마다 하나 만들어 두고 다시 쓴다 */
} FunBody;

struct Node {
  NodeKind kind;
  int line;               /* 오류 메시지의 줄 번호를 두 갈래가 맞추려면 필요하다 */
  union {
    double number;                                  /* N_NUM */
    ObjString* string;                              /* N_STR */
    ObjString* ident;                               /* N_IDENT */
    struct { ObjString* name; Node* value; } assign;/* N_ASSIGN */
    struct { TokenType op; Node* l; Node* r; } bin; /* N_LOGICAL, N_BINARY */
    struct { TokenType op; Node* operand; } un;     /* N_UNARY */
    struct { Node* callee; NodeList args; } call;   /* N_CALL */
    FunBody fun;                                    /* N_FUNEXPR, N_FUNDECL */
    struct { ObjString* name; Node* init; } var;    /* N_VARDECL, init 는 NULL 가능 */
    Node* expr;                                     /* N_EXPRSTMT, N_PRINT, N_RETURN */
    struct { Node* cond; Node* then; Node* other; } iff;  /* other 는 NULL 가능 */
    struct { Node* cond; Node* body; } loop;        /* N_WHILE */
    NodeList block;                                 /* N_BLOCK */
  } as;
};

void slNodeListInit(NodeList* l);
void slNodeListPush(NodeList* l, Node* n);

Node* slNewNode(NodeKind kind, int line);

/* 구문 트리를 다시 파싱되는 sl 소스로 찍는다. 식에는 괄호를 모두 붙인다.
   1단계의 왕복 검사와 기대 트리 검사가 이 출력을 쓴다. */
void slPrintProgram(FILE* out, NodeList* program);

#endif
