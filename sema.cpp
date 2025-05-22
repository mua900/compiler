#include "sema.hpp"

#include "common.hpp"
#include "log.hpp"
#include "stmt.hpp"
#include "environment.hpp"

// @todo
void semantic_analysis(ArrayView<Stmt*> program, ArrayView<Environment> declarations) {

    for (int i = 0; i < program.count; i++) {
        auto stmt = program.get(i);
        auto scope = stmt->scope;

        switch (stmt->kind) {
            case StmtKind::DECL_VAR: {
                auto var_decl = static_cast<Decl_Var_Stmt*>(stmt);
                break;
            }
            case StmtKind::DECL_PROC: {
                auto proc_decl = static_cast<Decl_Proc_Stmt*>(stmt);
                break;
            }
            case StmtKind::IF: {
                auto ifs = static_cast<If_Stmt*>(stmt);

                auto then_case = ifs->then_stmt;
                auto else_case = ifs->else_stmt;
                break;
            }
            case StmtKind::FOR: {
                auto fors = static_cast<For_Stmt*>(stmt);
                break;
            }
            case StmtKind::ASSIGN: {
                auto assign = static_cast<Assign_Stmt*>(stmt);
                break;
            }
            case StmtKind::BLOCK: {
                auto block = static_cast<Block_Stmt*>(stmt);
                break;
            }
            case StmtKind::EXPRESSION: {
                auto expr_s = static_cast<Expr_Stmt*>(stmt);
                break;
            }
            case StmtKind::IMPORT: {
                auto import_s = static_cast<Import_Stmt*>(stmt);
                break;
            }
            case StmtKind::RETURN: {
                auto ret_s = static_cast<Return_Stmt*>(stmt);
                break;
            }
        }
    }
}