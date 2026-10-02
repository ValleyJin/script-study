#include "env.h"
#include "memory.h"
#include "error.h"
#include "object.h"
#include "walk.h"

static int callDepth = 0;

/* return 을 flag 로 올린다. longjmp 는 런타임 오류에만 쓴다. */
static bool returning = false;
static Value returnValue;

static Value evalExpr(Node* n, Env* env);
static void execStmt(Node* n, Env* env);
static void execList(NodeList* list, Env* env);

/* 노드마다 ObjFunction 을 하나 만들어 두고 다시 쓴다. ObjClosure 는 평가할 때마다
   새로 만든다. docs/02 의 값 표현 절에 적은 대로다. */
static ObjClosure* makeClosure(Node* n, Env* env) {
  if (n->as.fun.fn == NULL) {
    ObjFunction* f = slNewFunction(n->as.fun.name, n->as.fun.paramCount);
    f->fnNode = n;
    n->as.fun.fn = f;
  }
  return slNewClosure(n->as.fun.fn, env);
}

static Value callClosure(ObjClosure* c, Value* args, int argCount, int line) {
  slCheckArity(c->fn->arity, argCount, line);
  if (callDepth >= SL_MAX_CALL) {
    slRuntimeError(line, "재귀가 너무 깊다: %d단계를 넘었다", SL_MAX_CALL);
  }

  Node* fnNode = c->fn->fnNode;
  /* 인자는 본문 블록과 같은 유효범위에 든다. 본문 블록을 따로 만들지 않는다. */
  Env* local = slEnvNew(c->env);
  for (int i = 0; i < argCount; i++) {
    slEnvDefine(local, fnNode->as.fun.params[i], args[i]);
  }

  callDepth++;
  execList(&fnNode->as.fun.body->as.block, local);
  callDepth--;

  if (returning) {
    returning = false;
    return returnValue;
  }
  return NIL_VAL;      /* return 없이 끝난 함수는 nil 을 돌려준다 */
}

static Value evalExpr(Node* n, Env* env) {
  switch (n->kind) {
    case N_NUM:   return NUM_VAL(n->as.number);
    case N_STR:   return OBJ_VAL(n->as.string);
    case N_TRUE:  return BOOL_VAL(true);
    case N_FALSE: return BOOL_VAL(false);
    case N_NIL:   return NIL_VAL;

    case N_IDENT: {
      Value v;
      if (!slEnvGet(env, n->as.ident, &v)) {
        slRuntimeError(n->line, "정의되지 않은 변수를 읽었다: %s", n->as.ident->chars);
      }
      return v;
    }

    case N_ASSIGN: {
      Value v = evalExpr(n->as.assign.value, env);
      if (!slEnvSet(env, n->as.assign.name, v)) {
        slRuntimeError(n->line, "정의되지 않은 변수에 대입했다: %s",
                       n->as.assign.name->chars);
      }
      return v;
    }

    case N_LOGICAL: {
      /* 단축 평가한다. 식의 값은 마지막으로 평가한 피연산자 자체다. */
      Value l = evalExpr(n->as.bin.l, env);
      if (n->as.bin.op == T_AND) {
        if (!slTruthy(l)) return l;
      } else {
        if (slTruthy(l)) return l;
      }
      return evalExpr(n->as.bin.r, env);
    }

    case N_BINARY: {
      /* 왼쪽을 먼저 평가한다. 지역 변수에 차례로 받아 순서를 고정한다.
         binop(eval(l), eval(r)) 로 적으면 순서가 C 컴파일러에 달린다. */
      Value a = evalExpr(n->as.bin.l, env);
      Value b = evalExpr(n->as.bin.r, env);
      switch (n->as.bin.op) {
        case T_PLUS:    return slAdd(a, b, n->line);
        case T_MINUS: case T_STAR: case T_SLASH:
          return slArith(n->as.bin.op, a, b, n->line);
        case T_LESS: case T_LESS_EQ: case T_GREATER: case T_GREATER_EQ:
          return slCompare(n->as.bin.op, a, b, n->line);
        case T_EQ_EQ:   return BOOL_VAL(slEqual(a, b));
        case T_BANG_EQ: return BOOL_VAL(!slEqual(a, b));
        default: break;
      }
      slRuntimeError(n->line, "모르는 이항 연산자다");
      return NIL_VAL;
    }

    case N_UNARY: {
      Value a = evalExpr(n->as.un.operand, env);
      if (n->as.un.op == T_BANG) return BOOL_VAL(!slTruthy(a));
      return slNegate(a, n->line);
    }

    case N_CALL: {
      /* 부르는 쪽 식을 먼저 평가하고 그다음 인자들을 왼쪽부터 평가한다. */
      Value callee = evalExpr(n->as.call.callee, env);
      int argCount = n->as.call.args.count;
      /* Value args[SL_MAX_ARGS] 로 두면 프레임마다 4080바이트를 잡는다. 깊이 1022 에서
         C 스택을 4MB 넘게 먹어 여유가 거의 없었다. 작은 자리를 쓰고 넘칠 때만 힙을 쓴다. */
      Value small[8];
      Value* args = small;
      if (argCount > (int)(sizeof(small) / sizeof(small[0]))) {
        args = ALLOCATE(Value, argCount);
      }
      for (int i = 0; i < argCount; i++) {
        args[i] = evalExpr(n->as.call.args.items[i], env);
      }
      slCheckCallable(callee, n->line);
      return callClosure(AS_CLOSURE(callee), args, argCount, n->line);
    }

    case N_FUNEXPR:
      return OBJ_VAL(makeClosure(n, env));

    default:
      slRuntimeError(n->line, "식이 아닌 노드를 평가했다");
      return NIL_VAL;
  }
}

static void execStmt(Node* n, Env* env) {
  switch (n->kind) {
    case N_VARDECL: {
      Value v = (n->as.var.init != NULL) ? evalExpr(n->as.var.init, env) : NIL_VAL;
      slEnvDefine(env, n->as.var.name, v);
      break;
    }

    case N_FUNDECL:
      /* 본문을 처리하기 전에 이름을 묶는다. 그래서 재귀가 된다. */
      slEnvDefine(env, n->as.fun.name, OBJ_VAL(makeClosure(n, env)));
      break;

    case N_EXPRSTMT:
      evalExpr(n->as.expr, env);
      break;

    case N_PRINT: {
      Value v = evalExpr(n->as.expr, env);
      slPrintValue(stdout, v);
      fputc('\n', stdout);
      break;
    }

    case N_RETURN:
      returnValue = (n->as.expr != NULL) ? evalExpr(n->as.expr, env) : NIL_VAL;
      returning = true;
      break;

    case N_IF:
      if (slTruthy(evalExpr(n->as.iff.cond, env))) {
        execStmt(n->as.iff.then, env);
      } else if (n->as.iff.other != NULL) {
        execStmt(n->as.iff.other, env);
      }
      break;

    case N_WHILE:
      while (!returning && slTruthy(evalExpr(n->as.loop.cond, env))) {
        execStmt(n->as.loop.body, env);
      }
      break;

    case N_BLOCK:
      /* 블록마다 환경을 새로 만든다. 그러지 않으면 while 본문에서 만든 클로저가
         한 변수를 함께 보게 되어 VM 과 답이 갈린다. */
      execList(&n->as.block, slEnvNew(env));
      break;

    default:
      break;
  }
}

static void execList(NodeList* list, Env* env) {
  for (int i = 0; i < list->count; i++) {
    if (returning) return;
    execStmt(list->items[i], env);
  }
}

void slWalk(NodeList* program) {
  callDepth = 0;
  returning = false;
  /* 최상위 선언은 전역이다. 전역 환경을 NULL 로 두면 전역 표로 간다. */
  execList(program, NULL);
}
