#include "stmt.hpp"

void print_stmt(Stmt* s) {
    if (!s) panic_and_abort("Internal: called print_stmt with null statement");

    String_Builder sb(512);

    switch (s->kind) {
        case StmtKind::DECL_VAR: {
            auto stmt = static_cast<Decl_Var_Stmt*>(s);

            String type_s = String(type_string(stmt->decl.type));

            const auto none = String("none");
            String expr_string = String(none);
            sb.clear();
            if (stmt->initializer) {
                expression_human_readable_string(stmt->initializer, &sb);
            }

            char decl_name[512];
            null_terminate(stmt->decl.name.lexeme, decl_name);

            printf("Statement Variable Declaration, declared variable name: %s, type: %s, initilializer: %s\n", decl_name, type_s.data, expr_string.data);
            break;
        }
        case StmtKind::DECL_PROC: {
            auto proc = static_cast<Decl_Proc_Stmt*>(s);

            printf("Statement Procedure Declaration:\n");
            char proc_name_nt[1024];
            null_terminate(proc->name.lexeme, proc_name_nt);
            printf("Declared procedure name: %s, parameter count: %zu, return count: %zu\n", proc_name_nt, proc->parameters.count, proc->returns.count);

            for (int i = 0; i < proc->parameters.count; i++) {
                char buff[1024];
                null_terminate(proc->parameters.get(i).name.lexeme, buff);
                printf("        %dth parameter name: %s\n", i+1, buff);
            }

            for (int i = 0; i < proc->returns.count; i++) {
                char buff[1024];
                null_terminate(proc->returns.get(i).name.lexeme, buff);
                printf("        %dth return name, type: %s %s\n", i+1, buff, type_string(proc->returns.get(i).type));
            }

            printf("Statement body has %zu statements\n", proc->body.count);
            for (auto bstmt : proc->body) {
                print_stmt(bstmt);  // @todo indent?
            }

            printf("End of procedure %s\n", proc_name_nt);
            break;
        }
        case StmtKind::ASSIGN: {
            auto stmt = static_cast<Assign_Stmt*>(s);

            printf("Statement Assignment\n");
            char buff[1024];
            null_terminate(stmt->target.lexeme, buff);
            printf("assignment target : %s\n", buff);
            if (!stmt->rhs) {
                panic_and_abort("Internal: Invalid assign statement, null source expression (shouldn't have been appended to the ast)");
            }
            printf("assignment source : ");
            print_expr(stmt->rhs);

            break;
        }
        case StmtKind::BLOCK: {
            auto stmt = static_cast<Block_Stmt*>(s);

            printf("Block Statement\n");
            for (auto block_s : stmt->body) {
                print_stmt(block_s);
            }
            printf("End Block Statement\n");

            break;
        }
        case StmtKind::IF: {
            printf("If Statement : \n");
            auto stmt = static_cast<If_Stmt*>(s);

            printf("then branch: \n");
            print_stmt(stmt->then_stmt);

            if (stmt->else_stmt) {
                printf("else branch: \n");
                print_stmt(stmt->else_stmt);
            }

            break;
        }
        case StmtKind::FOR: {
            printf("For Statement\n");
            // @todo print for statement
            break;
        }
        case StmtKind::IMPORT: {
            auto import = static_cast<Import_Stmt*>(s);
            char buff[1024];
            null_terminate(import->module_name.lexeme, buff);
            printf("Import Statement, imported module name %s\n", buff);
            break;
        }
        case StmtKind::EXPRESSION: {
            auto expr_stmt = static_cast<Expr_Stmt*>(s);
            printf("Expression Statement\n");
            print_expr(expr_stmt->expr);
            break;
        }
        default: panic_and_abort("INTERNAL Unhandled case in statement printer");
    }

    sb.free();
}

const char* stmt_name(Stmt* s) {
    switch (s->kind) {
        case StmtKind::DECL_VAR: return "STMT_DECL_VAR";
        case StmtKind::DECL_PROC: return "STMT_DECL_PROC";
        case StmtKind::IF: return "STMT_IF";
        case StmtKind::FOR: return "STMT_FOR";
        case StmtKind::ASSIGN: return "STMT_ASSIGN";
        case StmtKind::BLOCK: return "STMT_BLOCK";
        case StmtKind::EXPRESSION: return "STMT_EXPRESSION";
        /*RETURN*/
    }

    return "<UNKNOWN STATEMENT>";
}
