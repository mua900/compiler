#pragma once

#include "template.hpp"
#include "common.hpp"
#include "expr.hpp"

#include "type.hpp"
#include "scope.hpp"

enum class StmtKind {
    DECL_VAR, DECL_PROC,
    IF, FOR, ASSIGN, BLOCK,
    EXPRESSION,
    IMPORT, RETURN
};

struct Stmt {
    StmtKind kind;
    // @volatile maybe an id instead of a direct pointer
    const Environment* scope = NULL;
};

struct Block_Stmt : Stmt {
    ArrayView<Stmt*> body;

    Block_Stmt(Stmt* const * data, size_t count) : body(data, count) { kind = StmtKind::BLOCK; }
};

// this exists because we want to use these both in the parameter list and variable declaration
struct Decl_Var {
    Token name;
    Type_ID type = Type::NONE;
};

struct Decl_Var_Stmt : Stmt {
    Decl_Var decl;
    Expr* initializer = NULL;
    int var_id;

    Decl_Var_Stmt() { kind = StmtKind::DECL_VAR; }
};

struct Decl_Proc_Stmt : Stmt {
    Token name;
    ArrayView<Decl_Var> parameters;
    ArrayView<Decl_Var> returns;
    ArrayView<Stmt*> body;
    int proc_id;

    Decl_Proc_Stmt() : parameters(NULL, 0), returns(NULL, 0), body(NULL, 0) { kind = StmtKind::DECL_PROC; }
};

struct If_Stmt : Stmt {
    Expr* cond = NULL;       // @todo rename to condition
    Stmt* then_stmt = NULL;  // @todo rename
    Stmt* else_stmt = NULL;

    If_Stmt() { kind = StmtKind::IF; }
};

struct For_Stmt : Stmt {
    Expr* condition = NULL;
    Stmt* body = NULL;

    For_Stmt() { kind = StmtKind::FOR; }
};

struct Assign_Stmt : Stmt {
    Token target;
    Expr* rhs = NULL;

    int var_id;

    Assign_Stmt() { kind = StmtKind::ASSIGN; }
};

struct Expr_Stmt : Stmt {
    // @todo Expr_Type expr_type;
    Expr* expr;

    Expr_Stmt(Expr* expr) : expr(expr) { kind = StmtKind::EXPRESSION; }
};

struct Import_Stmt : Stmt {
    Token module_name;

    Import_Stmt(Token mod_name) : module_name(mod_name) { kind = StmtKind::IMPORT; }
};

struct Return_Stmt : Stmt {
    DArray<Expr*> returns;

    Return_Stmt(DArray<Expr*> return_exprs) : returns(return_exprs) { kind = StmtKind::RETURN; }
};

void print_stmt(Stmt* s);
