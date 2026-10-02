#include <math.h>

#include "ast.h"
#include "memory.h"

void slNodeListInit(NodeList* l) {
  l->items = NULL;
  l->count = 0;
  l->capacity = 0;
}

void slNodeListPush(NodeList* l, Node* n) {
  if (l->count + 1 > l->capacity) {
    int old = l->capacity;
    l->capacity = GROW_CAPACITY(old);
    l->items = GROW_ARRAY(Node*, l->items, old, l->capacity);
  }
  l->items[l->count++] = n;
}

Node* slNewNode(NodeKind kind, int line) {
  Node* n = ALLOCATE(Node, 1);
  n->kind = kind;
  n->line = line;
  return n;
}

static void indent(FILE* out, int depth) {
  for (int i = 0; i < depth; i++) fputs("  ", out);
}

static const char* opText(TokenType op) {
  switch (op) {
    case T_PLUS: return "+";   case T_MINUS: return "-";
    case T_STAR: return "*";   case T_SLASH: return "/";
    case T_LESS: return "<";   case T_LESS_EQ: return "<=";
    case T_GREATER: return ">";case T_GREATER_EQ: return ">=";
    case T_EQ_EQ: return "=="; case T_BANG_EQ: return "!=";
    case T_AND: return "and";  case T_OR: return "or";
    case T_BANG: return "!";
    default: return "?";
  }
}

/* 런타임 print 의 형식(slPrintValue)과 다른 일이다. 여기서는 다시 파싱되는
   리터럴을 내야 한다. sl 의 NUMBER 에 지수 표기가 없으므로 %g 가 'e' 를 내면
   그 소스는 다시 파싱되지 않는다. 그런 리터럴은 tests/ 에 두지 않는다. */
static void printNumberLiteral(FILE* out, double d) {
  /* 정수값은 %.0f 로 찍는다. long long 범위를 넘어도 지수 표기가 나오지 않아
     다시 파싱된다. 소수부가 있는 아주 작은 수는 %.17g 가 지수를 내므로 다시
     파싱되지 않는다. 그런 리터럴은 tests/ 에 두지 않는다. */
  if (isfinite(d) && d == floor(d)) fprintf(out, "%.0f", d);
  else fprintf(out, "%.17g", d);
}

static void printExpr(FILE* out, Node* n, int depth);
static void printStmt(FILE* out, Node* n, int depth);
static void printBlock(FILE* out, NodeList* list, int depth);

static void printFunTail(FILE* out, FunBody* f, int depth) {
  fputc('(', out);
  for (int i = 0; i < f->paramCount; i++) {
    if (i > 0) fputs(", ", out);
    fputs(f->params[i]->chars, out);
  }
  fputs(") ", out);
  printBlock(out, &f->body->as.block, depth);
}

static void printExpr(FILE* out, Node* n, int depth) {
  switch (n->kind) {
    case N_NUM:
      printNumberLiteral(out, n->as.number);
      break;
    case N_STR:
      fputc('"', out);
      fputs(n->as.string->chars, out);
      fputc('"', out);
      break;
    case N_TRUE:  fputs("true", out); break;
    case N_FALSE: fputs("false", out); break;
    case N_NIL:   fputs("nil", out); break;
    case N_IDENT: fputs(n->as.ident->chars, out); break;

    case N_ASSIGN:
      fputc('(', out);
      fputs(n->as.assign.name->chars, out);
      fputs(" = ", out);
      printExpr(out, n->as.assign.value, depth);
      fputc(')', out);
      break;

    case N_LOGICAL:
    case N_BINARY:
      fputc('(', out);
      printExpr(out, n->as.bin.l, depth);
      fprintf(out, " %s ", opText(n->as.bin.op));
      printExpr(out, n->as.bin.r, depth);
      fputc(')', out);
      break;

    case N_UNARY:
      fputc('(', out);
      fputs(opText(n->as.un.op), out);
      printExpr(out, n->as.un.operand, depth);
      fputc(')', out);
      break;

    case N_CALL:
      fputc('(', out);
      printExpr(out, n->as.call.callee, depth);
      fputc('(', out);
      for (int i = 0; i < n->as.call.args.count; i++) {
        if (i > 0) fputs(", ", out);
        printExpr(out, n->as.call.args.items[i], depth);
      }
      fputs("))", out);
      break;

    case N_FUNEXPR:
      fputs("(fun ", out);
      printFunTail(out, &n->as.fun, depth);
      fputc(')', out);
      break;

    default:
      fputs("<?>", out);
      break;
  }
}

static void printBlock(FILE* out, NodeList* list, int depth) {
  fputs("{\n", out);
  for (int i = 0; i < list->count; i++) {
    printStmt(out, list->items[i], depth + 1);
  }
  indent(out, depth);
  fputs("}", out);
}

static void printStmt(FILE* out, Node* n, int depth) {
  indent(out, depth);
  switch (n->kind) {
    case N_VARDECL:
      fputs("var ", out);
      fputs(n->as.var.name->chars, out);
      if (n->as.var.init != NULL) {
        fputs(" = ", out);
        printExpr(out, n->as.var.init, depth);
      }
      fputs(";\n", out);
      break;

    case N_FUNDECL:
      fputs("fun ", out);
      fputs(n->as.fun.name->chars, out);
      printFunTail(out, &n->as.fun, depth);
      fputc('\n', out);
      break;

    case N_EXPRSTMT:
      printExpr(out, n->as.expr, depth);
      fputs(";\n", out);
      break;

    case N_PRINT:
      fputs("print ", out);
      printExpr(out, n->as.expr, depth);
      fputs(";\n", out);
      break;

    case N_RETURN:
      fputs("return", out);
      if (n->as.expr != NULL) {
        fputc(' ', out);
        printExpr(out, n->as.expr, depth);
      }
      fputs(";\n", out);
      break;

    case N_IF:
      fputs("if (", out);
      printExpr(out, n->as.iff.cond, depth);
      fputs(")\n", out);
      printStmt(out, n->as.iff.then, depth + 1);
      if (n->as.iff.other != NULL) {
        indent(out, depth);
        fputs("else\n", out);
        printStmt(out, n->as.iff.other, depth + 1);
      }
      break;

    case N_WHILE:
      fputs("while (", out);
      printExpr(out, n->as.loop.cond, depth);
      fputs(")\n", out);
      printStmt(out, n->as.loop.body, depth + 1);
      break;

    case N_BLOCK:
      printBlock(out, &n->as.block, depth);
      fputc('\n', out);
      break;

    default:
      fputs("<?>\n", out);
      break;
  }
}

void slPrintProgram(FILE* out, NodeList* program) {
  for (int i = 0; i < program->count; i++) {
    printStmt(out, program->items[i], 0);
  }
}
