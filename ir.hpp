#pragma once

#include "template.hpp"

// ir, 3AC

enum class IR_Op : int {
    Invalid = 0,
    Call /* operand1 -> proc id, operand2 -> arity */, Param /* operand1 -> param */,
    Scope_Start, Scope_End,
    /* binary operand1, operand2 */
    Add, Sub, Mult, Div, Mod,
    /* unary operand1 */
    Negate, Not,
};

// @todo this will also need floating points etc. in the ir so a basic type system for here

// instead of doing another tree for this we can have a
// single thing to represent all ir constructs
// each type of ir node gets to use two integer operands
// to store its relevant data
struct IR_Instr {
    IR_Op type;
    int id;             // value id
    int operand1;
    int operand2;
};

struct Environment;
struct Stmt;

ArrayView<IR_Instr> translate(ArrayView<Stmt*> program, ArrayView<Environment> decls);