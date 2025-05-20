#pragma once

#include "common.hpp"
#include "template.hpp"

enum class TokenType;

// the highest 2 bits are reserved for identifying the kind of type the type_id is referring to:
// 00 -> primitives
// 01 -> procedure types
// 10 -> structures
typedef uint64_t Type_ID;

enum Type : Type_ID {
    NONE = 0,
    INT, FLOAT, STRING, BOOLEAN, NIL,
};

// @todo rename
enum Type_Type_Masks : Type_ID {
    TYPE_PROCEDURE = ((uint64_t)1) << 62,
    TYPE_STRUCTURE = ((uint64_t)1) << 63
};

bool is_procedure_type(Type_ID type);
bool is_structure_type(Type_ID type);

const char* type_string(Type_ID type);

Type_ID get_basic_type(const TokenType type);
Type_ID value_type(const Value& value);

bool is_basic_type(TokenType type);
bool is_numeric_type(Type_ID type);

struct Variable;

// this is a type like proc(int, float) -> int, bool
struct Proc_Type {
    DArray<Variable> parameters;
    DArray<Variable> returns;
};

// get corresponding actuall proc type from the id
Proc_Type get_proc_type(Type_ID type);