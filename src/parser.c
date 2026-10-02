#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "memory.h"
#include "parser.h"

/* 토큰 두 개를 미리 본다. 되돌리기는 쓰지 않는다. */
static Token cur;
static Token nxt;

static void bump(void) {
  cur = nxt;
  nxt = slLexNext();
}

static bool check(TokenType t)     { return cur.type == t; }
static bool checkNext(TokenType t) { return nxt.type == t; }

static bool match(TokenType t) {
  if (!check(t)) return false;
  bump();
  return true;
}

static void expect(TokenType t) {
  if (check(t)) { bump(); return; }
  slFatal(cur.line, "토큰이 빠졌다: %s", slTokenName(t));
}

static ObjString* identName(void) {
  if (!check(T_IDENT)) slFatal(cur.line, "이름이 와야 한다");
  ObjString* s = slCopyString(cur.start, cur.length);
  bump();
  return s;
}

static Node* expression(void);
static Node* statement(void);
static Node* declaration(void);
static Node* block(void);

/* primary → NUMBER | STRING | "true" | "false" | "nil" | IDENT
           | "(" expr ")" | "fun" "(" params? ")" block */
static void funTail(Node* n) {
  expect(T_LPAREN);
  n->as.fun.params = NULL;
  n->as.fun.paramCount = 0;
  if (!check(T_RPAREN)) {
    int cap = 0;
    do {
      if (n->as.fun.paramCount + 1 > cap) {
        int old = cap;
        cap = GROW_CAPACITY(old);
        n->as.fun.params = GROW_ARRAY(ObjString*, n->as.fun.params, old, cap);
      }
      n->as.fun.params[n->as.fun.paramCount++] = identName();
    } while (match(T_COMMA));
  }
  expect(T_RPAREN);
  if (!check(T_LBRACE)) slFatal(cur.line, "토큰이 빠졌다: %s", slTokenName(T_LBRACE));
  n->as.fun.body = block();
}

static Node* primary(void) {
  int line = cur.line;
  if (check(T_NUMBER)) {
    Node* n = slNewNode(N_NUM, line);
    /* 잘라서 읽으면 값이 말없이 달라진다. 길면 그때마다 자리를 잡는다. */
    char small[64];
    char* buf = small;
    if (cur.length >= (int)sizeof(small)) buf = ALLOCATE(char, cur.length + 1);
    memcpy(buf, cur.start, (size_t)cur.length);
    buf[cur.length] = '\0';
    n->as.number = strtod(buf, NULL);
    bump();
    return n;
  }
  if (check(T_STRING)) {
    Node* n = slNewNode(N_STR, line);
    /* 앞뒤 따옴표를 뺀다 */
    n->as.string = slCopyString(cur.start + 1, cur.length - 2);
    bump();
    return n;
  }
  if (match(T_TRUE))  return slNewNode(N_TRUE, line);
  if (match(T_FALSE)) return slNewNode(N_FALSE, line);
  if (match(T_NIL))   return slNewNode(N_NIL, line);
  if (check(T_IDENT)) {
    Node* n = slNewNode(N_IDENT, line);
    n->as.ident = identName();
    return n;
  }
  if (match(T_LPAREN)) {
    Node* inner = expression();
    expect(T_RPAREN);
    return inner;          /* 묶음에는 노드를 두지 않는다 */
  }
  if (match(T_FUN)) {
    Node* n = slNewNode(N_FUNEXPR, line);
    n->as.fun.name = NULL;
    funTail(n);
    return n;
  }
  slFatal(line, "식이 와야 한다");
  return NULL;
}

/* call → primary ( "(" args? ")" )* */
static Node* call(void) {
  Node* n = primary();
  while (check(T_LPAREN)) {
    int line = cur.line;
    bump();
    Node* c = slNewNode(N_CALL, line);
    c->as.call.callee = n;
    slNodeListInit(&c->as.call.args);
    if (!check(T_RPAREN)) {
      do {
        slNodeListPush(&c->as.call.args, expression());
      } while (match(T_COMMA));
    }
    expect(T_RPAREN);
    n = c;
  }
  return n;
}

/* unary → ( "!" | "-" ) unary | call */
static Node* unary(void) {
  if (check(T_BANG) || check(T_MINUS)) {
    int line = cur.line;
    TokenType op = cur.type;
    bump();
    Node* n = slNewNode(N_UNARY, line);
    n->as.un.op = op;
    n->as.un.operand = unary();
    return n;
  }
  return call();
}

/* 왼쪽 결합 이항 연산자를 한 꼴로 묶는다 */
static Node* binaryLevel(Node* (*next)(void), const TokenType* ops, int n_ops,
                         NodeKind kind) {
  Node* left = next();
  for (;;) {
    bool hit = false;
    for (int i = 0; i < n_ops; i++) {
      if (check(ops[i])) { hit = true; break; }
    }
    if (!hit) return left;
    int line = cur.line;
    TokenType op = cur.type;
    bump();
    Node* n = slNewNode(kind, line);
    n->as.bin.op = op;
    n->as.bin.l = left;
    n->as.bin.r = next();
    left = n;
  }
}

static Node* factor(void) {
  static const TokenType ops[] = {T_STAR, T_SLASH};
  return binaryLevel(unary, ops, 2, N_BINARY);
}
static Node* term(void) {
  static const TokenType ops[] = {T_PLUS, T_MINUS};
  return binaryLevel(factor, ops, 2, N_BINARY);
}
static Node* compare(void) {
  static const TokenType ops[] = {T_LESS, T_LESS_EQ, T_GREATER, T_GREATER_EQ};
  return binaryLevel(term, ops, 4, N_BINARY);
}
static Node* equality(void) {
  static const TokenType ops[] = {T_EQ_EQ, T_BANG_EQ};
  return binaryLevel(compare, ops, 2, N_BINARY);
}
static Node* andLevel(void) {
  static const TokenType ops[] = {T_AND};
  return binaryLevel(equality, ops, 1, N_LOGICAL);
}
static Node* orLevel(void) {
  static const TokenType ops[] = {T_OR};
  return binaryLevel(andLevel, ops, 1, N_LOGICAL);
}

/* assign → IDENT "=" assign | or
   IDENT 다음 토큰이 "=" 이면 대입이다. 미리 보기 한 번으로 완전히 갈린다. */
static Node* assignment(void) {
  if (check(T_IDENT) && checkNext(T_EQ)) {
    int line = cur.line;
    ObjString* name = identName();
    expect(T_EQ);
    Node* n = slNewNode(N_ASSIGN, line);
    n->as.assign.name = name;
    n->as.assign.value = assignment();
    return n;
  }
  return orLevel();
}

static Node* expression(void) { return assignment(); }

static Node* block(void) {
  int line = cur.line;
  expect(T_LBRACE);
  Node* n = slNewNode(N_BLOCK, line);
  slNodeListInit(&n->as.block);
  while (!check(T_RBRACE) && !check(T_EOF)) {
    slNodeListPush(&n->as.block, declaration());
  }
  expect(T_RBRACE);
  return n;
}

static Node* statement(void) {
  int line = cur.line;
  if (match(T_PRINT)) {
    Node* n = slNewNode(N_PRINT, line);
    n->as.expr = expression();
    expect(T_SEMI);
    return n;
  }
  if (match(T_IF)) {
    Node* n = slNewNode(N_IF, line);
    expect(T_LPAREN);
    n->as.iff.cond = expression();
    expect(T_RPAREN);
    n->as.iff.then = statement();
    /* else 를 만나면 지금 파싱하던 if 에 바로 붙인다. 매달린 else 규칙이다. */
    n->as.iff.other = match(T_ELSE) ? statement() : NULL;
    return n;
  }
  if (match(T_WHILE)) {
    Node* n = slNewNode(N_WHILE, line);
    expect(T_LPAREN);
    n->as.loop.cond = expression();
    expect(T_RPAREN);
    n->as.loop.body = statement();
    return n;
  }
  if (match(T_RETURN)) {
    Node* n = slNewNode(N_RETURN, line);
    n->as.expr = check(T_SEMI) ? NULL : expression();
    expect(T_SEMI);
    return n;
  }
  if (check(T_LBRACE)) return block();

  Node* n = slNewNode(N_EXPRSTMT, line);
  n->as.expr = expression();
  expect(T_SEMI);
  return n;
}

static Node* declaration(void) {
  int line = cur.line;
  if (match(T_VAR)) {
    Node* n = slNewNode(N_VARDECL, line);
    n->as.var.name = identName();
    n->as.var.init = match(T_EQ) ? expression() : NULL;
    expect(T_SEMI);
    return n;
  }
  /* stmt 자리의 fun. 둘째 토큰이 IDENT 면 선언, '(' 면 익명 함수로 시작하는 식이다. */
  if (check(T_FUN) && checkNext(T_IDENT)) {
    bump();
    Node* n = slNewNode(N_FUNDECL, line);
    n->as.fun.name = identName();
    funTail(n);
    return n;
  }
  return statement();
}

void slParse(const char* source, NodeList* out) {
  slLexInit(source);
  nxt = slLexNext();
  bump();
  slNodeListInit(out);
  while (!check(T_EOF)) {
    slNodeListPush(out, declaration());
  }
}
