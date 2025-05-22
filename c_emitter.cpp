#include "c_emitter.hpp"
#include "common.hpp"
#include "stmt.hpp"
#include "environment.hpp"

// @todo complete

static void translate_statement(Stmt* statement, FILE* output, String_Builder* sb);
static void dump_declarations(ArrayView<Environment> decls, FILE* output_file);

void output_c_code(ArrayView<Stmt*> program, ArrayView<Environment> decls, FILE* output_file) {
    auto output = output_file;

    String includes = String("#include <stdlib.h>\n#include <stdio.h>\n#include <string.h>\n");
    fwrite(includes.data, sizeof(char), includes.size, output);

    String_Builder sb = String_Builder(512);

    for (auto top_level : program) {
      translate_statement(top_level, output, &sb);
    }

    fclose(output);
}

// @todo
static void dump_declarations(ArrayView<Environment> decls, FILE* output) {
  // @todo type declarations first when they are a thing
  // they also need to have some sort of dependency info so they will be outputed in the correct order

  // global declarations on the top of the file
  auto global = decls.get_ref(0);

  for (int i = 0; i < global->variables.size; i++) {
    auto symbol = global->variable_names.get(i);
    auto var = global->variables.get(i);
  }


  for (int i = 0; i < decls.count; i++) {
  }
}

// simple recursive implementation
// this is not a proper implementation for quick prototyping.
static void translate_statement(Stmt* statement, FILE* output, String_Builder* sb) {
  switch (statement->kind) {
  case StmtKind::DECL_VAR: {
    auto decl_var = static_cast<Decl_Var_Stmt*>(statement);

    sb->clear_and_append(decl_var->decl.name.lexeme);
    fprintf(output, "%s %s ", type_string(decl_var->decl.type), sb->c_string());  // @fixme non-basic type names + c type names
    if (decl_var->initializer) {
      expression_string(decl_var->initializer, sb);
      fprintf(output, "= %s", sb->c_string());
    }
    fprintf(output, ";\n");
    break;
  }
  case StmtKind::DECL_PROC: {
    auto proc = static_cast<Decl_Proc_Stmt*>(statement);

    sb->clear_and_append(proc->name.lexeme);

    auto returns = proc->returns;
    if (returns.count) {
      // @todo if this is a multi return value procedure, make a new typedef to hold return values for this
      fprintf(output, "%s ", type_string(returns.get(0).type));
    }
    else {
      fprintf(output, "void ");
    }
    fprintf(output, "%s(", sb->c_string());

    for (int i = 0; i < proc->parameters.count; i++) {
      auto param = proc->parameters.get(i);

      sb->clear_and_append(param.name.lexeme);
      fprintf(output, "%s %s", type_string(param.type), sb->c_string());
      if (i != proc->parameters.count-1)
        fprintf(output, ",");
    }

    fprintf(output, ") {\n");

    for (auto bstmt : proc->body) {
      translate_statement(bstmt, output, sb);
    }

    fprintf(output, "}\n");
    break;
  }
  case StmtKind::ASSIGN: {
    auto assign = static_cast<Assign_Stmt*>(statement);

    sb->clear_and_append(assign->target.lexeme);
    fprintf(output, "%s", sb->c_string());
    expression_string(assign->rhs, sb);
    fprintf(output, " = %s\n", sb->c_string());
    break;
  }
  case StmtKind::BLOCK: {
    auto block = static_cast<Block_Stmt*>(statement);

    fprintf(output, "{\n");
    for (auto block_s : block->body) {
      translate_statement(block_s, output, sb);
    }
    printf("}\n");
    break;
  }
  case StmtKind::IF: {
    auto if_s = static_cast<If_Stmt*>(statement);

    expression_string(if_s->cond, sb);
    fprintf(output, "if (%s) {\n", sb->c_string());
    translate_statement(if_s->then_stmt, output, sb);

    if (if_s->else_stmt) {
      fprintf(output, "else {\n");
      translate_statement(if_s->else_stmt, output, sb);
    }

    break;
  }
  case StmtKind::FOR: {
    auto for_s = static_cast<For_Stmt*>(statement);
    expression_string(for_s->condition, sb);
    fprintf(output, "while (%s) {\n", sb->c_string());
    translate_statement(for_s->body, output, sb);
    fprintf(output, "}\n");
    break;
  }
  case StmtKind::IMPORT: {
    auto import_s = static_cast<Import_Stmt*>(statement);
    // @todo how will the import system translate to C?
    break;
  }
  case StmtKind::EXPRESSION: {
    auto expr_stmt = static_cast<Expr_Stmt*>(statement);
    expression_string(expr_stmt->expr, sb);
    fprintf(output, "%s;\n", sb->c_string());
    break;
  }
  case StmtKind::RETURN: {
    panic_and_abort("C returns not implemented");
    auto ret_stmt = static_cast<Return_Stmt*>(statement);
    // expression_string(ret_stmt->return_expr, sb);
    fprintf(output, "return %s;\n", sb->c_string());
    break;
  }
  default: panic_and_abort("INTERNAL Unhandled case in statement printer");
  }
}
