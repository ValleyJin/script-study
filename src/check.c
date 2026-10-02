#include "check.h"
#include "error.h"
#include "memory.h"

/* 한 유효범위. 그 안의 선언 이름을 미리 다 모아 둔다.
   그래야 "선언보다 앞선 자리에서 썼는가"를 가릴 수 있다. */
typedef struct {
  ObjString** names;
  int count;        /* 모아 둔 이름 수 */
  int ready;        /* 지금 자리까지 선언이 끝난 수 */
  int capacity;
  bool isFunction;  /* 함수의 호출 유효범위인가 */
} Scope;

#define MAX_SCOPES 256

static Scope scopes[MAX_SCOPES];
static int scopeCount = 0;      /* 0 이면 전역이다 */
static int funDepth = 0;        /* 0 이면 최상위다 */
static ObjString* initializing = NULL;  /* 지금 초깃값을 보고 있는 변수 */

static void push(bool isFunction) {
  if (scopeCount >= MAX_SCOPES) slFatal(0, "유효범위가 너무 깊다");
  Scope* s = &scopes[scopeCount++];
  s->names = NULL;
  s->count = 0;
  s->ready = 0;
  s->capacity = 0;
  s->isFunction = isFunction;
}

static void pop(void) { scopeCount--; }

static void addName(Scope* s, ObjString* name, int line) {
  for (int i = 0; i < s->count; i++) {
    if (s->names[i] == name) slFatal(line, "같은 유효범위에 이름이 겹친다: %s", name->chars);
  }
  if (s->count + 1 > s->capacity) {
    int old = s->capacity;
    s->capacity = GROW_CAPACITY(old);
    s->names = GROW_ARRAY(ObjString*, s->names, old, s->capacity);
  }
  s->names[s->count++] = name;
}

/* 함수 하나가 쓰는 슬롯은 그 호출 유효범위부터 안쪽 블록까지 합쳐 센다. */
static void countLocals(int line) {
  int total = 0;
  for (int i = scopeCount - 1; i >= 0; i--) {
    total += scopes[i].count;
    if (scopes[i].isFunction) break;
  }
  if (total > SL_MAX_LOCALS) {
    slFatal(line, "한 함수의 인자와 지역 변수가 %d개를 넘었다", SL_MAX_LOCALS);
  }
}

static void resolveName(ObjString* name, int line) {
  for (int i = scopeCount - 1; i >= 0; i--) {
    Scope* s = &scopes[i];
    for (int j = 0; j < s->count; j++) {
      if (s->names[j] != name) continue;
      if (j < s->ready) return;              /* 이미 선언이 끝났다 */
      if (name == initializing) {
        slFatal(line, "자기 초깃값에서 자기 이름을 썼다: %s", name->chars);
      }
      slFatal(line, "선언보다 앞선 자리에서 지역 이름을 썼다: %s", name->chars);
    }
  }
  /* 어느 유효범위에도 없으면 전역이다. 전역은 실행할 때 걸러진다. */
}

static void checkExpr(Node* n, int depth);
static void checkStmt(Node* n);
static void checkBlockList(NodeList* list);

static void checkFun(Node* n) {
  /* 인자는 본문 블록과 같은 유효범위에 든다. 본문 블록을 따로 만들지 않는다. */
  push(true);
  Scope* s = &scopes[scopeCount - 1];
  for (int i = 0; i < n->as.fun.paramCount; i++) {
    addName(s, n->as.fun.params[i], n->line);
    s->ready++;
  }
  countLocals(n->line);
  funDepth++;
  checkBlockList(&n->as.fun.body->as.block);
  funDepth--;
  pop();
}

static void checkExpr(Node* n, int depth) {
  if (depth > SL_MAX_DEPTH) {
    slFatal(n->line, "식의 중첩 깊이가 %d을 넘었다", SL_MAX_DEPTH);
  }
  switch (n->kind) {
    case N_NUM: case N_STR: case N_TRUE: case N_FALSE: case N_NIL:
      break;
    case N_IDENT:
      resolveName(n->as.ident, n->line);
      break;
    case N_ASSIGN:
      resolveName(n->as.assign.name, n->line);
      checkExpr(n->as.assign.value, depth + 1);
      break;
    case N_LOGICAL:
    case N_BINARY:
      checkExpr(n->as.bin.l, depth + 1);
      checkExpr(n->as.bin.r, depth + 1);
      break;
    case N_UNARY:
      checkExpr(n->as.un.operand, depth + 1);
      break;
    case N_CALL:
      if (n->as.call.args.count > SL_MAX_ARGS) {
        slFatal(n->line, "한 호출의 인자가 %d개를 넘었다", SL_MAX_ARGS);
      }
      checkExpr(n->as.call.callee, depth + 1);
      for (int i = 0; i < n->as.call.args.count; i++) {
        checkExpr(n->as.call.args.items[i], depth + 1);
      }
      break;
    case N_FUNEXPR:
      checkFun(n);
      break;
    default:
      break;
  }
}

static void checkStmt(Node* n) {
  switch (n->kind) {
    case N_VARDECL:
      /* 변수 선언은 초깃값을 평가하기 전에 이름을 잡는다. 그래서 `var a = a;` 는 오류다. */
      if (n->as.var.init != NULL) {
        ObjString* saved = initializing;
        initializing = n->as.var.name;
        checkExpr(n->as.var.init, 0);
        initializing = saved;
      }
      if (scopeCount > 0) scopes[scopeCount - 1].ready++;
      break;

    case N_FUNDECL:
      /* 함수 선언은 본문을 처리하기 전에 이름을 묶는다. 그래서 재귀가 된다. */
      if (scopeCount > 0) scopes[scopeCount - 1].ready++;
      checkFun(n);
      break;

    case N_EXPRSTMT:
    case N_PRINT:
      checkExpr(n->as.expr, 0);
      break;

    case N_RETURN:
      if (funDepth == 0) slFatal(n->line, "최상위에서 return 을 쓸 수 없다");
      if (n->as.expr != NULL) checkExpr(n->as.expr, 0);
      break;

    case N_IF:
      checkExpr(n->as.iff.cond, 0);
      checkStmt(n->as.iff.then);
      if (n->as.iff.other != NULL) checkStmt(n->as.iff.other);
      break;

    case N_WHILE:
      checkExpr(n->as.loop.cond, 0);
      checkStmt(n->as.loop.body);
      break;

    case N_BLOCK:
      push(false);
      checkBlockList(&n->as.block);
      pop();
      break;

    default:
      break;
  }
}

/* 유효범위에 든 선언 이름을 먼저 모은다. 그러지 않으면 뒤에 선언한 지역 이름을
   앞에서 쓴 것과 전역을 쓴 것을 가릴 수 없다. */
static void collectNames(NodeList* list) {
  if (scopeCount == 0) return;      /* 최상위 선언은 전역이다 */
  Scope* s = &scopes[scopeCount - 1];
  for (int i = 0; i < list->count; i++) {
    Node* n = list->items[i];
    if (n->kind == N_VARDECL) addName(s, n->as.var.name, n->line);
    else if (n->kind == N_FUNDECL) addName(s, n->as.fun.name, n->line);
  }
  countLocals(list->count > 0 ? list->items[0]->line : 0);
}

static void checkBlockList(NodeList* list) {
  collectNames(list);
  for (int i = 0; i < list->count; i++) {
    checkStmt(list->items[i]);
  }
}

void slCheck(NodeList* program) {
  scopeCount = 0;
  funDepth = 0;
  initializing = NULL;
  checkBlockList(program);      /* 전역에는 유효범위를 두지 않는다 */
}
