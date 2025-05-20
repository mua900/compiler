#pragma once

#include "type.hpp"
#include "environment.hpp"

class Typechecker {
    ArrayView<Environment> declarations;
    const Environment* curr_env = NULL;

public:
    Typechecker(ArrayView<Environment> decls) : declarations(decls) {
        curr_env = &decls.data[0];
    }

    bool typecheck(ArrayView<Stmt*> program, ArrayView<Environment> declarations);
    Type_ID typecheck_expr(Expr* expr);
    bool typecheck_statement(Stmt* stmt);
};