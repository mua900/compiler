#pragma once

#include "type.hpp"
#include "template.hpp"
struct Value;
struct Stmt;
struct Environment;

// we enumarete the variables and procedure inside the same scope

struct Variable {
    int var_id = 0;  // this is assigned by the environment
    Type_ID type = Type::NONE;
};

struct Procedure {
    int proc_id = 0;  // this is assigned by the environment
    const Environment* procedure_scope;
    ArrayView<Variable> parameters;
    ArrayView<Stmt*> body;
    Type_ID return_type;

    // proc_flags
    bool is_nested : 1;  // lexically scoped inside a scope

    Procedure() : parameters(NULL, 0),body(NULL, 0) {}
    Procedure(ArrayView<Stmt*> body, ArrayView<Variable> parameters, const Environment* proc_scope) : procedure_scope(proc_scope), parameters(parameters), body(body) {}
};

// @todo structures
struct Structure {
    int struct_id;
    ArrayView<Variable> members;

    Structure() : members(NULL, 0) {}
};
