#pragma once

#include <cstddef>
#include <cstdint>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t s32;

// - statically check opcode-operand count
// - statically check constant indexing
// - statically check memory accesses
// - check for returns, jumps and control flow correctness

#include "template.hpp"

// zero terminated
struct Code_Block {
  u8* code;
  int size;
  int storage;

  const char* name = NULL;
};

Code_Block make_code_block(int storage);

// @speed make access to this alligned
struct Basic_Block {
  u8* code;
  Basic_Block* jump = NULL;
  int size;
};

Array<Basic_Block> cut_to_basic_blocks(const Code_Block block);

struct Memory {
  s32* memory;
  s32 size;

  Memory(s32 size);
  void write(s32 value, s32 address) const;
  s32 read(s32 address) const;
};

struct Stack {
  s32* data;
  s32 size;
  s32 current = 0;

  Stack(s32 size);
  s32* allocate(s32 alloc_size);
  void push(s32 value);
  s32 pop();
  void deallocate(s32 pop_size);
};

typedef enum : u8 {
  R_INVALID = 0, R1 = 1, R2 = 2, R3 = 3, R4 = 4, R5 = 5, R6 = 6, R7 = 7, R8 = 8, R9 = 9, R10 = 10
} Register;

struct VM;

struct Processor {
  s32 r1 = 0;
  s32 r2 = 0;
  s32 r3 = 0;
  s32 r4 = 0;
  s32 r5 = 0;
  s32 r6 = 0;
  s32 r7 = 0;
  s32 r8 = 0;
  s32 r9 = 0;
  s32 r10 = 0;

  s32 pc = 0;

  u8 Negative : 1,
     Zero : 1;

  s32* get_register(u8 reg, VM* vm, Code_Block* code);  // the extra arguments are for error reporting
  void print() const;
};

struct VM {
  Stack stack = Stack(1024);
  Memory memory = Memory(1024);
  Stack constants = Stack(1024);
  Processor processor;

  Code_Block* current_codeblock = NULL;  // @hack this is just here because we want to get location information and bytecode to disassemble from within the methods of this

  u32 current_instruction;  // the index of the start of the instruction we are currently on

  VM() = default;
  VM(Processor proc) : processor(proc) {}
  VM(Stack constants) : constants(constants) {}

  s32 get_constant(int const_index) const;
  int add_constant(s32 value);
  void print() const;
};

void bytecode_run(VM* vm, Code_Block* block);

// @xxx how jumps should work in bytecode?
// maybe the jumps shouldn't jump to arbitrary indexes in the codeblock but we could have labels
// in the codeblock and jumps are only allowed to jump to those known locations
//

// 16 and 32 bit values which take up multiple bytes are laid out little endian
enum Opcode : u8 {
  // 0 is an invalid opcode
  // and also the termination character for code blocks

  // mov reg s32 -> 6 bytes
  Op_Mov = 1,
  // const reg constant_index(16) -> 4
  Op_Constant,
  // op push/pop reg -> 3
  Op_Push, Op_Pop,
  // binary_operation reg1 reg2  (reg1 = reg1 binop reg2) -> 3
  Op_Add, Op_Sub, Op_Mult, Op_Div, Op_Mod,
  Op_And, Op_Or, Op_Xor,

  // @todo unary negate
  // Op_Negate

  // read target_register address_register -> 3
  Op_Read,
  // write source_register address_register -> 3
  Op_Write,

  // op_jmp address(16) -> 3
  Op_Jmp, Op_Jz, Op_Jnz, Op_Jn, Op_Jnn,  // @xxx does the addresses need to be a 16 bits or does it need to be 32 bits?

  // op_ret -> 1
  Op_Ret,
};

bool analyze_codeblock(const Code_Block* block, int constant_count);
void disassemble(Code_Block block);
void print_instruction(u8* code, int index);

enum class Binary_Operation {
  Add, Sub, Mult, Div, Mod, And, Or, Xor,
};

enum class Jump_Condition {
  NEGATIVE, N_NEGATIVE, ZERO, N_ZERO
};

void test_bytecode();

void run_bytecode(Code_Block* blocks, size_t count);
