#include "resolve.hpp"
#include "log.hpp"

ArrayView<Environment> Resolver::resolve() {
  auto global = Environment(-1);  // @hack, -1
  environments.add(global);
  current_environment = environments.size-1;

  collect_declarations();
  resolve_references();

  return ArrayView<Environment>(environments.data, environments.size);
}

void Resolver::collect_declarations() {
  for (auto stmt : program) {
    collect_declaration(stmt);
  }
}

// collect procedure declarations and fill in the environments array
void Resolver::collect_declaration(Stmt* stmt) {
  stmt->scope = environments.get_ref(current_environment);

  switch (stmt->kind) {
    case StmtKind::DECL_VAR: {
      auto decl_var = static_cast<Decl_Var_Stmt*>(stmt);

      Variable var;
      var.type = decl_var->decl.type;
      // for type inferrence this should pass through as non-determined to typecheck
      environments.get_ref(current_environment)->bind_variable(decl_var->decl.name.lexeme, var);
      break;
    }
    case StmtKind::DECL_PROC: {
      auto decl_proc = static_cast<Decl_Proc_Stmt*>(stmt);

      Procedure proc;

      int enclosing = current_environment;
      Environment proc_scope = Environment(current_environment);
      environments.add(proc_scope);
      current_environment = environments.size - 1;

      proc.procedure_scope = environments.get_ref(current_environment);
      proc.body = decl_proc->body;
      proc.proc_id = 0;  // assigned by the environment

      for (auto proc_stmt : decl_proc->body) {
          collect_declaration(proc_stmt);
      }

      DArray<Variable> parameters(decl_proc->parameters.count);
      for (int i = 0; i < decl_proc->parameters.count; i++) {
        Decl_Var param = decl_proc->parameters.get(i);
        int var_id = environments.get_ref(current_environment)->bind_variable(param.name.lexeme, Variable{0 /*assigned in the call*/, param.type});
        parameters.add(Variable{var_id, param.type});
      }

      proc.parameters = ArrayView<Variable>(parameters.data, parameters.size);

      proc.is_nested = enclosing > 1;

      current_environment = enclosing;

      decl_proc->proc_id = environments.get_ref(current_environment)->bind_procedure(decl_proc->name.lexeme, proc);
      break;
    }
    case StmtKind::IF: {
      auto if_s = static_cast<If_Stmt*>(stmt);
      collect_declaration(if_s->then_stmt);  // then statement must exist
      if (if_s->else_stmt) {
        collect_declaration(if_s->else_stmt);
      }
      break;
    }
    case StmtKind::FOR: {
      auto for_s = static_cast<For_Stmt*>(stmt);
      collect_declaration(for_s->body);
      break;
    }
    case StmtKind::BLOCK: {
      auto block = static_cast<Block_Stmt*>(stmt);

      int enclosing = current_environment;
      Environment proc_scope = Environment(current_environment);
      environments.add(proc_scope);
      current_environment = environments.size - 1;  // last index

      for (auto s : block->body) {
        collect_declaration(s);
      }

      current_environment = enclosing;
      break;
    }
    default: break;   // return, import, expression, assign
                      // any statement that doesn't contain other statements
  }
}

void Resolver::resolve_references() {
  for (auto stmt : program) {
    resolve_reference(stmt);
  }
}

void Resolver::resolve_reference(Stmt* stmt) {
  // visit every expression and fill in the variable and procedure references with correct ids
  auto scope = stmt->scope;

  switch (stmt->kind) {
    case StmtKind::DECL_VAR: {
      auto decl_var = static_cast<Decl_Var_Stmt*>(stmt);

      if (decl_var->initializer) {
        resolve_expression(decl_var->initializer, scope);
      }
      break;
    }
    case StmtKind::DECL_PROC: {
      auto decl_proc = static_cast<Decl_Proc_Stmt*>(stmt);

      for (auto s : decl_proc->body) {
        resolve_reference(s);
      }
      break;
    }
    case StmtKind::IF: {
      auto if_s = static_cast<If_Stmt*>(stmt);

      resolve_expression(if_s->cond, scope);
      resolve_reference(if_s->then_stmt);
      if (if_s->else_stmt) {
        resolve_reference(if_s->else_stmt);
      }
      break;
    }
    case StmtKind::FOR: {
      auto for_s = static_cast<For_Stmt*>(stmt);

      resolve_expression(for_s->condition, scope);
      resolve_reference(for_s->body);
      break;
    }
    case StmtKind::BLOCK: {
      auto block = static_cast<Block_Stmt*>(stmt);

      for (auto s : block->body) {
        resolve_reference(s);
      }

      break;
    }
    case StmtKind::EXPRESSION: {
      auto expr = static_cast<Expr_Stmt*>(stmt);
      resolve_expression(expr->expr, scope);
      break;
    }
    case StmtKind::ASSIGN: {
      auto assign = static_cast<Assign_Stmt*>(stmt);
      resolve_expression(assign->rhs, scope);
      break;
    }
    case StmtKind::IMPORT: {
      // @todo module names
      break;
    }
    case StmtKind::RETURN: {
      auto return_s = static_cast<Return_Stmt*>(stmt);
      for (auto ret : return_s->returns) {
        resolve_expression(ret, scope);
      }
      break;
    }
    default:
      panic_and_abort("INTERNAL: Unexpected statement kind");
  }
}

bool Resolver::resolve_expression(Expr* expr, const Environment*  scope) {
  switch (expr->type) {
    case ExprType::BINARY: {
      auto binary = static_cast<Binary_Expr*>(expr);
      if (binary->left)  resolve_expression(binary->left, scope);
      if (binary->right) resolve_expression(binary->right, scope);
      break;
    }
    case ExprType::UNARY: {
      auto unary = static_cast<Unary_Expr*>(expr);
      resolve_expression(unary->operand, scope);
      break;
    }
    case ExprType::GROUPING: {
      auto grouping = static_cast<Grouping_Expr*>(expr);
      resolve_expression(grouping->expr, scope);
      break;
    }
    case ExprType::VARIABLE: {
      auto var = static_cast<Variable_Expr*>(expr);

      const Variable* declaration = NULL;;
      auto search = scope;
      while (search != NULL) {
        declaration = search->get_variable(var->identifier.lexeme);
        if (declaration) break;

        if (search->parent_index == -1)  // global
          break;
        search = environments.get_ref(search->parent_index);
      }

      if (!declaration) {
        char buff[1024];
        null_terminate(var->identifier.lexeme, buff);
        errorf(var->identifier.line, "Use of undeclared variable %s", buff);
        return false;
      }

      var->var_id = declaration->var_id;
      break;
    }
    case ExprType::LITERAL:
      break; // nothing to do
    case ExprType::CALL: {
      auto call = static_cast<Call_Expr*>(expr);

      auto call_expr = call;
      bool found = false;
      while (call_expr->expression && !found) {
        auto expr = call_expr->expression;
        if (expr->type == ExprType::VARIABLE) {
          found = true;

          auto proc_name = static_cast <Variable_Expr*> (expr);
          const Procedure* proc = NULL;
          auto search = scope;
          while (search != NULL) {
            proc = search->get_procedure(proc_name->identifier.lexeme);
            if (proc) {
              break;
            }

            if (search->parent_index == -1) { // global
              break;
            }
            search = environments.get_ref(search->parent_index);
          }

          if (!proc) {
            String_Builder* scratch = scratch_string_builder();
            scratch->clear_and_append(proc_name->identifier.lexeme);
            errorf(proc_name->identifier.line, "Use of undeclared procedure %s", scratch->c_string());
            return false;
          }

          call_expr->proc_id = proc->proc_id;
        } else if (expr->type == ExprType::CALL) {
          call_expr = static_cast <Call_Expr*> (expr);
        } else {
          panic_and_abort("INTERNAL Expression chain in call expression should only contain call expressions or procedure names");
        }
      }

      break;
    }
    case ExprType::MEMBER: {
      // @todo member lookup
      auto member = static_cast<Member_Expr*>(expr);
      resolve_expression(member->expression, scope);
      break;
    }
    default:
      panic_and_abort("INTERNAL: Unexpected expression type");
  }

  return true;
}

void Resolver::dump_environments() {
  auto global_scope = environments.data[0];
  printf("global scope:\n");
  global_scope.dump();

  for (int index = 1; index < environments.size; index++) {
    auto env = environments.data[index];
    auto parent_index = env.parent_index;
    printf("%s environment, child of %s\n", ordinal_string(index), ordinal_string(parent_index));  // debug
    env.dump();
  }
}
