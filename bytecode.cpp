#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>

#include "common.hpp"
#include "bytecode.hpp"
#include "template.hpp"

#include "bytecode_data.hpp"

// dump the code_block that caused the error, error location and so on.
[[noreturn]]
static void bytecode_error(const VM* vm, const Code_Block* block, char const * const msg, int location_byte_index) {
  printf("BYTECODE ERROR: \n");
  printf("VM state: \n");
  vm->print();
  printf("\nThe part of the code block that caused the crash: \n");
  disassemble(*block);
  fprintf(stderr, "Bytecode Error on %s: %s\n", block->name ? block->name : "(Unnamed block)", msg);
  exit(1);
}

Code_Block make_code_block(int storage) {
  int size = (int)next_multiple_of_wordsize(storage + 2);
  u8* memory = (u8*)malloc_or_die(size);
  Code_Block block = (Code_Block){.code = memory, .size = 0, .storage = size, .name = NULL};
  return (block);
}

Memory::Memory(s32 size) : size(size) {
  memory = (s32*)calloc(size, sizeof(s32));
}

void Memory::write(s32 value, s32 address) const {
  if (address > size) {
    panic_and_abort("Memory write out of bounds");  // @todo when we have a static analysis before run that can detect this, this may become redundant
  }

  memory[address] = value;
}

s32 Memory::read(s32 address) const {
  if (address > size) {
    panic_and_abort("Memory read out of bounds");
  }

  return memory[address];
}

Stack::Stack(s32 size) : size(size) {
  data = (s32*)calloc(size, sizeof(s32));
}

s32* Stack::allocate(s32 alloc_size) {
  if (current + alloc_size >= size) {
    panic_and_abort("Stack overflow");
  }

  current += alloc_size;

  return data + current - alloc_size;
}

void Stack::push(s32 value) {
  s32* mem = allocate(1);

  *mem = value;
}

s32 Stack::pop() {
  if (current < 1) {
    panic_and_abort("Stack underflow");
  }

  s32 value = data[current - 1];

  current -= 1;

  return value;
}

void Stack::deallocate(s32 pop_size) {
  if (current < pop_size) {
    panic_and_abort("Stack underflow");
  }

  current -= pop_size;
}

s32 VM::get_constant(int const_index) const {
  if (constants.current < const_index) {
    printf("index : %d  sp : %d\n", const_index, constants.current);
    bytecode_error(this, current_codeblock, "Reaching empty contant index", this->processor.pc);
  }

  return constants.data[const_index];
}

int VM::add_constant(s32 value) {
  constants.push(value);
  return constants.current - 1;  // return constant index
}

s32* Processor::get_register(u8 reg, VM* vm, Code_Block* block) {
  switch (reg) {
    case R1: return &r1;
    case R2: return &r2;
    case R3: return &r3;
    case R4: return &r4;
    case R5: return &r5;
    case R6: return &r6;
    case R7: return &r7;
    case R8: return &r8;
    case R9: return &r9;
    case R10: return &r10;

    default:
      printf("Register : %d\n", (int)reg);  // @hack
      bytecode_error(vm, block, "Unknown register", vm->processor.pc);
  }
}

void Processor::print() const {
  printf("Processor state:\n");
  printf("Registers: \n");
  printf("%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n", r1, r2, r3, r4, r5, r6, r7, r8, r9, r10);
  printf("Program counter: %d\n", pc);
}

void VM::print() const {
  processor.print();
  printf("Stack size : %d\nStack pointer : %d\n", stack.size, stack.current);
  printf("Memory size : %d\n", memory.size);
}

static void push_register(VM* vm, Code_Block* block) {
  auto code = block->code;
  s32* reg = vm->processor.get_register(code[vm->processor.pc + 1], vm, block);
  vm->stack.push(*reg);
}

static void pop_register(VM* vm, Code_Block* block) {
  auto code = block->code;
  s32 pop = vm->stack.pop(); // the underflow etc. kind of stuff checking should happen here so we can report better errors with location information
  s32* reg = vm->processor.get_register(code[vm->processor.pc + 1], vm, block);
  *reg = pop;
}

static void binary_operation(VM* vm, Code_Block* block, u8 opcode) {
  auto processor = &vm->processor;
  auto code = block->code;
  s32* reg1 = processor->get_register(code[vm->processor.pc + 1], vm, block);
  s32* reg2 = processor->get_register(code[vm->processor.pc + 2], vm, block);

  switch (opcode) {
    case Op_Add:
      *reg1 = *reg1 + *reg2; break;
    case Op_Sub:
      *reg1 = *reg1 - *reg2; break;
    case Op_Mult:
      *reg1 = *reg1 * *reg2; break;
    case Op_Div:
      *reg1 = *reg1 / *reg2; break;
    case Op_Mod:
      *reg1 = *reg1 % *reg2; break;
    case Op_And:
      *reg1 = *reg1 & *reg2; break;
    case Op_Or:
      *reg1 = *reg1 | *reg2; break;
    case Op_Xor:
      *reg1 = *reg1 ^ *reg2; break;
    default: panic_and_abort("Unexpected opcode in binary_operation");
  }

  if (*reg1) {
    processor->Zero = 0;

    if (*reg1 < 0) {
      processor->Negative = 1;
    } else processor->Negative = 0;
  }
  else {
    processor->Zero = 1;
  }
}

static void write_memory(VM* vm, Code_Block* block) {
  const auto code = block->code;
  const s32* source_register  = vm->processor.get_register(code[vm->processor.pc + 1], vm, block);
  const s32* address_register = vm->processor.get_register(code[vm->processor.pc + 2], vm, block);

  vm->memory.write(*source_register, *address_register);
}

static void read_memory(VM* vm, Code_Block* block) {
  const auto code = block->code;
  s32* target_register  = vm->processor.get_register(code[vm->processor.pc + 1], vm, block);
  const s32* address_register = vm->processor.get_register(code[vm->processor.pc + 2], vm, block);

  *target_register = vm->memory.read(*address_register);
}

static u32 read_little_endian32(const u8* ptr) {
  return ptr[0] | ptr[1] << 8 | ptr[2] << 16 | ptr[3] << 24;
}

static u16 read_little_endian16(const u8* ptr) {
  return (ptr[0] | ptr[1] << 8);
}

static void get_to_next_instruction(Processor* proc, Code_Block* code) {
  proc->pc += instruction_bytes[code->code[proc->pc]];
}

void bytecode_run(VM* vm, Code_Block* block) {
  vm->current_codeblock = block;

  s32* pc = &vm->processor.pc;
  const auto code = block->code;

  const Processor* processor = &vm->processor;

  while (*pc < block->size) {
    const auto offset = instruction_bytes[code[*pc]];

    switch (code[*pc]) {
    // mov reg s32
    case Op_Mov: {
      s32* reg = vm->processor.get_register(code[*pc + 1], vm, block);
      u32 value = read_little_endian32(&code[*pc + 2]);
      *reg = (s32) value;
      break;
    }
    // const reg constant_index(16)
    case Op_Constant: {
      s32* reg = vm->processor.get_register(code[*pc + 1], vm, block);
      u16 constant_index = read_little_endian16(&code[*pc + 2]);
      *reg = vm->get_constant(constant_index);
      break;
    }
    // op push reg
    case Op_Push:
      push_register(vm, block);
      break;
    // op pop reg
    case Op_Pop:
      pop_register(vm, block);
      break;
    // operator reg1 reg2
    case Op_Add:
    case Op_Sub:
    case Op_Mult:
    case Op_Div:
    case Op_Mod:
    case Op_And:
    case Op_Or:
    case Op_Xor:
      binary_operation(vm, block, code[*pc]);  // switches a second time, is this good
      break;
    // op_jmp address(16)
    case Op_Jmp: {
      *pc = read_little_endian16(&code[*pc + 1]);
      continue;
    }
    case Op_Jz: {
      if (processor->Zero) {
        *pc = read_little_endian16(&code[*pc + 1]);
        continue;
      }
      break;
    }
    case Op_Jnz: {
      if (!processor->Zero) {
        *pc = read_little_endian16(&code[*pc + 1]);
        continue;
      }
      break;
    }
    case Op_Jn: {
      if (processor->Negative) {
        *pc = read_little_endian16(&code[*pc + 1]);
        continue;
      }
      break;
    }
    case Op_Jnn: {
      if (!processor->Negative) {
        *pc = read_little_endian16(&code[*pc + 1]);
        continue;
      }
      break;
    }
    // write source_register address_register
    case Op_Write:
      write_memory(vm, block);
      break;
    // read target_register address_register
    case Op_Read:
      read_memory(vm, block);
      break;
    case Op_Ret:
      *pc += 1;
      return;
    default:
      bytecode_error(vm, block, "Undefined opcode", *pc);
    }

    *pc += offset;
  }

  bytecode_error(vm, block, "Error: Reached end of code block before returning", *pc);
}

Register get_register(const u8* code, int index) {
  Register reg = (Register) code[index];
  if (!(R1 <= reg && R10 >= reg)) {
    printf("\nInvalid register id %d\n", (int)reg);
    return R_INVALID;
  }

  return reg;
}

bool valid_opcode(Opcode opcode) {
  // @volatile opcodes
  return Op_Mov <= opcode && opcode <= Op_Ret;
}

bool validate_register(const u8* code, size_t index, const char* name) {
    Register reg = get_register(code, index);
    if (reg == R_INVALID) {
        printf("Invalid register id %d at offset %zu in codeblock %s\n", code[index], index, name);
        return false;
    }
    return true;
}

// @xxx use this
static inline u8* next_instruction(u8* code) {
  return code + instruction_bytes[*code];
}

bool is_jump_instruction(Opcode op) {
  // @update
  return op >= Op_Jmp && op <= Op_Jnn;
}

/*
Array<Basic_Block> cut_to_basic_blocks(const Code_Block block) {
  u8* instr = block.code;

  Array<Basic_Block> blocks;

  {
    int count = 0;

    while (instr != 0) {
      count++;
      instr = next_instruction(instr);
    }

    blocks.data = (Basic_Block*)malloc_or_die(sizeof(Basic_Block)*count);;
    blocks.size = count;
  }

  int current_block_size = 0;

  for (int i = 0; i < blocks.size; i++) {
    if (is_jump_instruction((Opcode)*instr)) {
      blocks.data[i] = (Basic_Block){instr, NULL, current_block_size};
    }

    instr += instruction_bytes[*instr];
    // instr = next_instruction(instr);
  }


  for (int i = 0; i < blocks.size; i++) {
    auto size = instruction_bytes[*instr];
    current_block_size += size;

    // instr = next_instruction(instr);
    instr += size;
  }

  return Array<Basic_Block>(blocks.data, blocks.size);
}
*/

bool analyze_codeblock(const Code_Block* block, const int constant_count) {
  const char* name = block->name ? block->name : "(unnamed)";

  if (!block->code)
    panic_and_abortf("codeblock %s has no code", name);

  bool success = true;
  size_t index = 0;

  // @todo detect dead code
  // @todo statically detect read from uninitialized registers
  // @todo a linear walk doesn't actually do it, go into branches and analyze each possible control flow path
  // @todo infinite loops
  // @todo analyze how instructions modify processor flags

  DArray<u16> jump_addresses;

  DArray<u16> instruction_start_points;
  int stack_pointer = 0;

  auto code = block->code;

  // @update stack_size
  const int stack_size = 1024;

  Opcode opcode = (Opcode)code[index];
  while (index < block->size) {
      instruction_start_points.add(index);
      if (!valid_opcode(opcode)) {
          printf("Invalid opcode 0x%02X at offset %zu in codeblock %s\n", code[index], index, name);
          return false;  // abort at this point since we lost track of where the correct boundaries are
      }

      auto offset = instruction_bytes[code[index]];

      if (index + offset > block->size) {
        printf("block %s ends with trailing bytes at the end\n", name);
        success = false;
      }

      index += offset;
      opcode = (Opcode)code[index];
      if (opcode == 0) break;  // null termination
  }

  for (auto instruction_index : instruction_start_points) {
      index = instruction_index;

      Opcode opcode = (Opcode)code[index];

      // @update opcode
      switch (opcode) {
        case Op_Mov: {
          if (!validate_register(code, index + 1, name)) success = false;
          break;
        }
        case Op_Constant: {
          if (!validate_register(code, index + 1, name)) success = false;

          u16 const_index = read_little_endian16(&code[index + 2]);
          if (const_index > constant_count) {
            printf("Reaching out of bounds constant index in codeblock %s offset %d\n", name, const_index);
            success = false;
          }

          break;
        }
        case Op_Push:
        case Op_Pop: {
          if (!validate_register(code, index + 1, name)) success = false;

          if (opcode == Op_Push) stack_pointer += 1;
          else                   stack_pointer -= 1;

          if (stack_pointer < 0) {
            printf("Stack underflow in codeblock %s offset %ld\n", name, index);
            success = false;
          }
          if (stack_pointer > stack_size) {
            printf("Stack overflow in codeblock %s offset %ld\n", name, index);
            success = false;
          }

          break;
        }
        case Op_Add:
        case Op_Sub:
        case Op_Mult:
        case Op_Div:
        case Op_Mod:
        case Op_And:
        case Op_Or:
        case Op_Xor:

        case Op_Read:
        case Op_Write:
        {
          if (!validate_register(code, index + 1, name)) success = false;
          if (!validate_register(code, index + 2, name)) success = false;

          // @todo inspect memory addresses?
          break;
        }

        case Op_Jmp:
        case Op_Jz:
        case Op_Jnz:
        case Op_Jn:
        case Op_Jnn:
        {
          u16 address = read_little_endian16(&code[index + 1]);
          bool found = false;
          for (auto valid_jump : instruction_start_points) {
            if (valid_jump == address) {
              found = true;
            }
          }

          if (!found) {
            printf("Invalid jump address! Jump index %d in codeblock %s\n", address, name);
            success = false;
          }

          jump_addresses.add(index);
          break;
        }

        case Op_Ret:
          if (code[index + 1] != 0) {
            printf("Codeblock %s has instructions after return opcode\n", name);
            success = false;
          }
          break;
    }
  }

  return success;
}

const char* instruction_string(Opcode opcode) {
  switch (opcode) {
    // @update opcode
    case Op_Mov: return "Op_Mov";
    case Op_Constant: return "Op_Constant";
    case Op_Push: return "Op_Push";
    case Op_Pop: return "Op_Pop";
    case Op_Add: return "Op_Add";
    case Op_Sub: return "Op_Sub";
    case Op_Mult: return "Op_Mult";
    case Op_Div: return "Op_Div";
    case Op_Mod: return "Op_Mod";
    case Op_And: return "Op_And";
    case Op_Or: return "Op_Or";
    case Op_Xor: return "Op_Xor";
    case Op_Read: return "Op_Read";
    case Op_Write: return "Op_Write";
    case Op_Jmp: return "Op_Jmp";
    case Op_Jz: return "Op_Jz";
    case Op_Jnz: return "Op_Jnz";
    case Op_Jn: return "Op_Jn";
    case Op_Jnn: return "Op_Jnn";
    case Op_Ret: return "Op_Ret";
    default: panic_and_abortf("Unknown opcode : %d", opcode);
  }
}

const char* register_string(Register reg) {
  switch (reg) {
  case R1: return "r1";
  case R2: return "r2";
  case R3: return "r3";
  case R4: return "r4";
  case R5: return "r5";
  case R6: return "r6";
  case R7: return "r7";
  case R8: return "r8";
  case R9: return "r9";
  case R10: return "r10";
  default: panic_and_abort("Unknown register in register_string");
  }
}

void disassemble(Code_Block block) {
    const char* name = block.name ? block.name : "(unnamed)";
    if (!block.code) panic_and_abortf("codeblock %s has no code", name);

    size_t index = 0;

    printf("\nCode Block %s of size %d\n", name, block.size);

    auto code = block.code;
    while (index < block.size) {

      auto offset = instruction_bytes[code[index]];

      if (index + offset > block.size) {
        printf("In disassembly of the code block, block %s ends with invalid bytes at the end\n", name);
        return;
      }

      Opcode opcode = (Opcode)code[index];
      if (opcode == 0) {
        printf("End of block %s\n", name);
        return;
      }

      if (!valid_opcode(opcode)) {
        printf("Invalid opcode %d\n", (int)opcode);
        printf("Disassembly failed\n");
        return;
      }

      // @todo don't print these out just here but use a string builder, also these shouldn't be printed if there is an error anyways
      printf("%04x: ", (u32)index);

      // @volatile opcodes
      // @update opcode
      switch (opcode) {
        case Op_Mov: {
          Register reg = get_register(code, index + 1);
          if (reg == R_INVALID) return;
          printf("Op_Mov r%d %d\n", (int)reg, read_little_endian32(&code[index + 2]));
          break;
        }
        case Op_Constant: {
          Register reg = get_register(code, index + 1);
          if (reg == R_INVALID) return;
          printf("Op_Constant r%d %d\n", (int)reg, read_little_endian16(&code[index + 2]));
          break;
        }
        case Op_Push:
        case Op_Pop: {
          Register reg = get_register(code, index + 1);
          if (reg == R_INVALID) return;
          printf("%s r%d\n", instruction_string(opcode), reg);
          break;
        }
        case Op_Add:
        case Op_Sub:
        case Op_Mult:
        case Op_Div:
        case Op_Mod:
        case Op_And:
        case Op_Or:
        case Op_Xor:

        case Op_Read:
        case Op_Write:

        {
          Register reg1 = get_register(code, index + 1);
          Register reg2 = get_register(code, index + 2);
          if (reg1 == R_INVALID || reg2 == R_INVALID) return;
          printf("%s r%d r%d\n", instruction_string(opcode), (int)reg1, (int)reg2);
          break;
        }

        case Op_Jmp:
        case Op_Jz:
        case Op_Jnz:
        case Op_Jn:
        case Op_Jnn:

        {
          u16 address = read_little_endian16(&code[index + 1]);
          printf("%s %x\n", instruction_string(opcode), address);
          break;
        }

        case Op_Ret:
          printf("Op_Ret\n");
          break;
    }

    index += offset;
  }
}

// @test
#include "./test/test_bytecode.hpp"

void test_bytecode() {
  for (int i = 0; i < ARRAY_SIZE(Tests); i++) {
      if (!analyze_codeblock(&Tests[i], 0)) {
        return;
      }
      disassemble(Tests[i]);
  }

  run_all_tests();
}

// @todo
void run_bytecode(Code_Block* blocks, size_t count) {}
