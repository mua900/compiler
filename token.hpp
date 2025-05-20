#pragma once

#include "common.hpp"

enum class TokenType {
  NONE,

  // single character
  COMMA, DOT, COLON, SEMICOLON,
  PAREN_LEFT, PAREN_RIGHT, BRACE_LEFT, BRACE_RIGHT, SQUARE_LEFT, SQUARE_RIGHT,
  EXCLAMATION, EQUAL, GREATER, LESS,
  SLASH, PERCENT, PLUS, MINUS, STAR, HASH,

  // double character
  EXCLAMATION_EQUAL, EQUAL_EQUAL, GREATER_EQUAL, LESS_EQUAL,

  // keywords
  VAR, FOR, WHILE, TRUE, FALSE, RETURN, OR, AND, IF, ELSE, PROC, IMPORT,
  // types
  INT, FLOAT, STRING,

  NUMERIC_LITERAL, STRING_LITERAL, IDENTIFIER,

  END,

  COUNT
};

struct Token {
  String lexeme;
  TokenType type;
  Value value;
  int line;
  int offset;

  Token() = default;
  Token(String lexeme, TokenType type, Value value, int line, int offset) : lexeme(lexeme), type(type), value(value), line(line), offset(offset) {}

  void print() const;
};

const char* token_type_str(TokenType type);

enum class Operator {
  NONE = (int)TokenType::NONE,

  PLUS = (int)TokenType::PLUS,  // binary
  MINUS = (int)TokenType::MINUS,  // binary + unary
  MULT = (int)TokenType::STAR,  // binary
  DIV = (int)TokenType::SLASH,  // binary
  MOD = (int)TokenType::PERCENT,  // binary
  NOT = (int)TokenType::EXCLAMATION,  // unary

  EQUALS = (int)TokenType::EQUAL_EQUAL,  // binary
  NOT_EQUALS = (int)TokenType::EXCLAMATION_EQUAL,  // binary
  LESS = (int)TokenType::LESS,  // binary
  GREATER = (int)TokenType::GREATER,  // binary
  LESS_EQUAL = (int)TokenType::LESS_EQUAL,  // binary
  GREATER_EQUAL = (int)TokenType::GREATER_EQUAL,  // binary

  // @todo bitwise operations

  // logical
  OR = (int)TokenType::OR, // binary
  AND = (int)TokenType::AND,  // binary
};

Operator token_to_operator(TokenType type);

const char* operator_string(Operator op);

String token_to_string(Token token);
