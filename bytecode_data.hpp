#pragma once

// internal file

// both bytecode emitter and bytecode implementations need this

static const int OP_COUNT = (int)Op_Ret;

// @volatile instructions
// @update instruction_bytes
static const int instruction_bytes[OP_COUNT + 1] = {
  0, 6, 4,       // invalid mov const
  2, 2,          // push pop
  3, 3, 3, 3, 3, 3, 3, 3, // binary (8 of them)
  3, 3,          // read write
  3, 3, 3, 3, 3, // jumps (5 of them)
  1              // ret
};
