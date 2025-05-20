#include "bytecode_emitter.hpp"

#include "bytecode_data.hpp"

static void zero_terminate(Code_Block* block);
static void code_block_maybe_grow(Code_Block* code, int desired_storage);

DArray<Code_Block> output_bytecode(ArrayView<IR_Instr> program, ArrayView<Environment> declarations) {
  return DArray<Code_Block>();
}

// @todo register allocator
Code_Block emit_bytecode(ArrayView<IR_Instr> ir_code, ArrayView<Environment> declarations) {
  Code_Block output;

  Processor processor;
  Variable register_occupants[10] = {};

  for (auto instr : ir_code) {
    switch (instr.type) {
      case IR_Op::Call:
      case IR_Op::Param:
      case IR_Op::Scope_Start:
      case IR_Op::Scope_End:
      case IR_Op::Add:
      case IR_Op::Sub:
      case IR_Op::Mult:
      case IR_Op::Div:
      case IR_Op::Mod:
      case IR_Op::Negate:
      case IR_Op::Not:
      case IR_Op::Invalid:
        break;
    }
  }

  zero_terminate(&output);

  return output;
}

static void zero_terminate(Code_Block* block) {
  code_block_maybe_grow(block, block->size + 2);
  block->code[block->size - 2] = 0;
  block->code[block->size - 1] = 0;
}

static void code_block_maybe_grow(Code_Block* code, int desired_storage) {
  if (code->storage <= desired_storage) {
    // @fixme proper allocation size
    auto new_storage = code->storage * 1.4;  // @xxx arbitrary
    u8* ncode = (u8*)malloc(new_storage);
    memcpy(ncode, code->code, code->size);

    free(code->code);
    code->code = ncode;
    code->storage = new_storage;
  }
}

void write_little_endian(u8* mem, u32 value) {
    mem[0] = (uint8_t)(value & 0xFF);
    mem[1] = (uint8_t)((value >> 8) & 0xFF);
    mem[2] = (uint8_t)((value >> 16) & 0xFF);
    mem[3] = (uint8_t)((value >> 24) & 0xFF);
}

void write_little_endian_16(u8* mem, u16 value) {
    mem[0] = (uint8_t)(value & 0xFF);
    mem[1] = (uint8_t)((value >> 8) & 0xFF);
}

void emit_scope_exit() {
}

void emit_bytecode_mov32(Code_Block* code, Register reg, s32 value) {
  auto instr_size = instruction_bytes[Op_Mov];
  code_block_maybe_grow(code, code->size + instr_size);

  code->code[code->size] = Op_Mov;

  code->code[code->size + 1] = reg;

  write_little_endian(&code->code[code->size + 2], value);
  code->size += instr_size;
}

void emit_bytecode_jmp(Code_Block* code, u16 address) {
  auto instr_size = instruction_bytes[Op_Jmp];
  code_block_maybe_grow(code, code->size + instr_size);

  code->code[code->size] = Op_Jmp;

  write_little_endian_16(&code->code[code->size + 1], address);
  code->size += instr_size;
}

void emit_bytecode_jmp_cond(Code_Block* code, u32 address, Jump_Condition condition) {
  auto instr_size = instruction_bytes[Op_Jz];  // assuming all the conditional jumps have the same length
  code_block_maybe_grow(code, code->size + instr_size);

  Opcode jump_kind;
  switch (condition) {
    case Jump_Condition::NEGATIVE:
      jump_kind = Op_Jn;
      break;
    case Jump_Condition::N_NEGATIVE:
      jump_kind = Op_Jnn;
      break;
    case Jump_Condition::ZERO:
      jump_kind = Op_Jz;
      break;
    case Jump_Condition::N_ZERO:
      jump_kind = Op_Jnz;
      break;
  }

  code->code[code->size] = jump_kind;

  write_little_endian_16(&code->code[code->size + 1], address);

  code->size += instr_size;
}

void emit_bytecode_read(Code_Block* code, Register target_reg, Register address_reg) {
  auto instr_size = instruction_bytes[Op_Read];
  code_block_maybe_grow(code, code->size + instr_size);

  code->code[code->size] = Op_Read;
  code->code[code->size + 1] = target_reg;
  code->code[code->size + 2] = address_reg;
}

void emit_bytecode_write(Code_Block* code, Register source_reg, Register address_reg) {
  auto instr_size = instruction_bytes[Op_Write];
  code_block_maybe_grow(code, code->size + instr_size);

  code->code[code->size] = Op_Write;
  code->code[code->size + 1] = source_reg;
  code->code[code->size + 2] = address_reg;
}

void emit_bytecode_constant(Code_Block* code, Register reg, u32 const_index) {
  auto instr_size = instruction_bytes[Op_Constant];
  code_block_maybe_grow(code, code->size + instr_size);

  code->code[code->size] = Op_Constant;
  write_little_endian(&code->code[code->size + 1], const_index);
}

void emit_bytecode_binary_op(Code_Block* code, Register operand1, Register operand2, Opcode opcode) {
  auto instr_size = instruction_bytes[Op_Add];  // @note: assuming all the binary opearations have the same length
  code_block_maybe_grow(code, code->size + instr_size);

  code->code[code->size] = opcode;
  code->code[code->size + 1] = operand1;
  code->code[code->size + 2] = operand2;
}

// void emit_bytecode_unary_op(Code_Block* code, Register reg, Unary_Operation unop);

void emit_bytecode_return(Code_Block* code) {
  auto instr_size = instruction_bytes[Op_Ret];
  code_block_maybe_grow(code, code->size + instr_size);

  code->code[code->size] = Op_Ret;
}
