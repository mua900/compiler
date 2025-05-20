#pragma once

#include "scope.hpp"
#include "template.hpp"
#include "ir.hpp"
#include "bytecode.hpp"

struct Environment;

DArray<Code_Block> output_bytecode(ArrayView<IR_Instr> program, ArrayView<Environment> declarations);

// @todo this will need a register allocator if we will only have 10 registers in the bytecode

void emit_bytecode_mov32(Code_Block* code, Register reg, s32 value);
void emit_bytecode_jmp(Code_Block* code, u32 address);
void emit_bytecode_jmp_cond(Code_Block* code, u32 address, Jump_Condition condition);
void emit_bytecode_read(Code_Block* code, Register target_reg, Register address_reg);
void emit_bytecode_write(Code_Block* code, Register source_reg, Register address_reg);
void emit_bytecode_constant(Code_Block* code, Register reg, u32 const_index);
void emit_bytecode_binary_op(Code_Block* code, Register operand1, Register operand2, Binary_Operation binop);
// void emit_bytecode_unary_op(Code_Block* code, Register reg, Unary_Operation unop);
void emit_bytecode_return(Code_Block* code);

Code_Block emit_bytecode(ArrayView<IR_Instr> block, ArrayView<Environment> declarations);
