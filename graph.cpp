// utility that uses graphiz to create visuallizations for several things we would like to do that to.

#include <cstdio>

#include "template.hpp"
#include "token.hpp"
#include "expr.hpp"
#include "stmt.hpp"

char* expression_string(Expr* expr, const char* expr_name) {
  static char buffer[1024];
  int writen = 0;
#ifdef DEBUG
  writen = snprintf(buffer, 1024, "%s %s", expr_name, expr->source);
#else
  writen = snprintf(buffer, 1024, "%s", expr_name);
#endif
  if (!writen) {
    fprintf(stderr, "INTERNAL ERROR : Couldn't write anything to expression string in graph creation utility\n");
  }

  return buffer;
}

bool expression_tree_to_dot(Expr* root, const char* filename) {
  if (!root) {
    fprintf(stderr, "INTERNAL : Provided expression is null to the graph generator\n");
    return false;
  }

  FILE* file = fopen(filename, "w");
  if (!file) {
    fprintf(stderr, "Couldn't open file: %s\n", filename);
    return false;
  }

  fprintf(file, "digraph ExprTree {\n");

  int node_id = 0;

  struct Node_Info {
    Expr* expr;
    int parent_id;
  };

  DArray<Node_Info> stack(1024);

  // Node_Info stack[1024];
  stack[0] = {root, 0};
  int sp = 1;

  int safety = 0;

  Expr current_expr;
  while (sp) {
    if (safety > 1000) {
      fclose(file);
      panic_and_abort("INTERNAL: Probably infinite looping in the graph generator for expression");
    }

    Node_Info ni = stack[sp-1];

    if (!ni.expr) {
      sp--;
      continue;
    }

    current_expr = *ni.expr;

    // connect parent -> current
    fprintf(file, "  node%d -> node%d\n", ni.parent_id, node_id);

    char scratch[1024];
    switch (current_expr.type) {
      case ExprType::BINARY: {
        auto binary = static_cast<Binary_Expr*>(&current_expr);

        fprintf(file, "  node%d [label=\"%s %s\"]\n", node_id, expression_string(binary, "Binary"), operator_string(binary->opperator));

        stack[sp] =     {binary->left, node_id};  // overwrite
        stack[sp + 1] = {binary->right, node_id};
        sp++;
        break;
      }
      case ExprType::UNARY: {
        auto unary = static_cast<Unary_Expr*>(&current_expr);

        fprintf(file, "  node%d [label=\"%s %s\"]\n", node_id, expression_string(unary, "Unary"), operator_string(unary->opperator));

        stack[sp] = {unary->operand, node_id};  // overwrite
        break;
      }
      case ExprType::GROUPING: {
        auto grouping = static_cast<Grouping_Expr*>(&current_expr);

        fprintf(file, "  node%d [label=\"%s\"]\n", node_id, expression_string(grouping, "Grouping"));
        stack[sp] = {grouping->expr, node_id};  // overwrite
        break;
      }
      case ExprType::VARIABLE: {
        auto variable = static_cast<Variable_Expr*>(&current_expr);

        null_terminate(variable->identifier.lexeme, scratch);
        fprintf(file, "  node%d [label=\"%s %s\"]\n", node_id, expression_string(variable, "Variable"), scratch);
        sp--;
        break;
      }
      case ExprType::LITERAL: {
        auto literal = static_cast<Literal*>(&current_expr);

        null_terminate(literal->value.string(), scratch);
        fprintf(file, "  node%d [label=\"%s %s\"]\n", node_id, expression_string(literal, "Literal"), scratch);
        sp--;
        break;
      }
      case ExprType::CALL: {
        auto call = static_cast<Call_Expr*>(&current_expr);

        if (call->expression->type == ExprType::VARIABLE) {
          auto proc = static_cast<Variable_Expr*>(call->expression);
          null_terminate(proc->identifier.lexeme, scratch);
          fprintf(file, "  node%d [label=\"%s %s ", node_id, expression_string(call, "Call"), scratch);
        } else {
          fprintf(file, "  node%d [label=\"%s ", node_id, expression_string(call, "Call"));
        }

        for (int i = 0; i < call->arguments.size; i++) {
          String_Builder sb(128);
          expression_string(call->arguments.get(i), &sb);
          fprintf(file, " %s ", sb.c_string());
          sb.free();
        }

        fprintf(file, "\"]\n");
        stack[sp] = {call->expression, node_id};  // overwrite
        break;
      }
      case ExprType::MEMBER: {
        auto member = static_cast<Member_Expr*>(&current_expr);

        null_terminate(member->member.lexeme, scratch);
        fprintf(file, "  node%d [label=\"%s %s]\n", node_id, expression_string(member, "Member"), scratch);
        stack[sp] = {member->expression, node_id};  // overwrite
        break;
      }
      default: {
        fprintf(stderr, "INTERNAL : Invalid expression type in graph creation\n");
        sp--;
      }
    }

    node_id++;
    safety++;

    stack.ensure_capacity(sp + 1);
  }

  fprintf(file, "}\n");

  stack.free();

  fclose(file);
  return true;
}

bool ast_to_dot(ArrayView<Stmt*> program, char* filename) {
  FILE* file = fopen(filename, "w");
  if (!file) {
    fprintf(stderr, "Couldn't open file: %s\n", filename);
    return false;
  }

  int node_id = 1;

  for (auto stmt : program) {
    switch (stmt->kind) {
      case StmtKind::DECL_VAR: {
        fprintf(file, "  node%d [label=\"Variable Declaration\"]\n", node_id);
        break;
      }
      case StmtKind::DECL_PROC: {
        fprintf(file, "  node%d [label=\"Procedure Declaration\"]\n", node_id);
        break;
      }
      case StmtKind::IF: {
        fprintf(file, "  node%d [label=\"If Statement\"]\n", node_id);
        break;
      }
      case StmtKind::FOR: {
        fprintf(file, "  node%d [label=\"For Statement\"]\n", node_id);
        break;
      }
      case StmtKind::ASSIGN: {
        fprintf(file, "  node%d [label=\"Assign Statement\"]\n", node_id);
        break;
      }
      case StmtKind::BLOCK: {
        fprintf(file, "  node%d [label=\"Block Statement\"]\n", node_id);
        break;
      }
      case StmtKind::EXPRESSION: {
        fprintf(file, "  node%d [label=\"Expression Statement\"]\n", node_id);
        break;
      }
    }

    node_id++;
  }

  fprintf(file, "}\n");

  fclose(file);
  return true;
}
