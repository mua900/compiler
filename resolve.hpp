#pragma once

#include "common.hpp"
#include "expr.hpp"
#include "template.hpp"
#include "stmt.hpp"

#include "environment.hpp"
#include "type.hpp"

struct Resolver {
    DArray<Environment> environments;
    ArrayView<Stmt*> program;

    int current_environment = 0;
    //Environment* current_environment = NULL;

    Resolver(ArrayView<Stmt*> program) : program(program) {}

    ArrayView<Environment> resolve();

    void collect_declarations();
    void collect_declaration(Stmt* stmt);

    bool resolve_expression(Expr* expr, const Environment* begin_scope);

    void resolve_reference(Stmt* stmt);
    void resolve_references();

    void dump_environments();
};
