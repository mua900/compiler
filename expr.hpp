#pragma once

#include "template.hpp"
#include "common.hpp"
#include "token.hpp"
#include "type.hpp"

enum class ExprType {
    BINARY,
    UNARY,
    GROUPING,
    VARIABLE,
    LITERAL,
    CALL,
    MEMBER,
    // @todo
    /*
    CAST,
    */
};

struct Expr {
    ExprType type;

#ifdef DEBUG
    const char* source;
#endif

    location_t location;
};

// the binary and unary operators use the same type.
// we could split them to different types (binop, unop)

struct Binary_Expr : Expr {
    Expr* left;
    Operator opperator;
    Expr* right;

    Binary_Expr() : left(NULL), opperator(Operator::NONE), right(NULL) { type = ExprType::BINARY; }
    Binary_Expr(Expr* left, Operator op, Expr* right) : left(left), opperator(op), right(right) { type = ExprType::BINARY; }
};

struct Unary_Expr : Expr {
    Operator opperator;
    Expr* operand;

    Unary_Expr() : opperator(Operator::NONE), operand(NULL) {
        type = ExprType::UNARY;
    }
    Unary_Expr(Expr* expr) : opperator(Operator::NONE), operand(expr) { type = ExprType::UNARY; }
    Unary_Expr(Operator op, Expr* expr) : opperator(op), operand(expr) { type = ExprType::UNARY; }
};

struct Grouping_Expr : Expr {
    Expr* expr;

    Grouping_Expr(Expr* expr) : expr(expr) { type = ExprType::GROUPING; }
};

struct Variable_Expr : Expr {
    Token identifier;
    int var_id;  // in the current scope

    Variable_Expr(Token ident) : identifier(ident) { type = ExprType::VARIABLE; }
};

struct Literal : Expr {
    Value value;

    Literal(Value value) : value(value) { type = ExprType::LITERAL; }
};

struct Member_Expr : Expr {
    Expr* expression;
    Token member;

    Member_Expr() { type = ExprType::MEMBER; }
};

struct Call_Expr : Expr {
    Expr* expression;
    // an expression that should evaluate to a procedure
    // which can only be a call to another procedure that returns another procedure at the moment
    Array<Expr*> arguments;  // this needs to be mutable because of collapse expression
    int proc_id;

    Call_Expr() : arguments(NULL, 0) { type = ExprType::CALL; }
};

// @todo
/*
struct Cast_Expr : Expr {
    Type_ID cast_type;
    Expr* expr;

    Cast_Expr(Type_ID cast_type, Expr* expr) : cast_type(cast_type), expr(expr) { type = ExprType::CAST; }
};
*/

void print_expr(Expr* expr);
Expr* collapse_expr(Expr* expr);

// clears the string builder and fills it with expression string
void expression_string(Expr* expression, String_Builder* builder);

void expression_human_readable_string(Expr* expr, String_Builder* builder);
const char* expr_type_str(const Expr* expr);

ArrayView<Expr*> find_subexpressions(Expr* expr, ExprType type);
int expr_deep(Expr* expr);