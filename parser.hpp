#pragma once

#include "common.hpp"
#include "token.hpp"
#include "stmt.hpp"
#include "expr.hpp"

/*
  precedence:

  highest to lowest:
  * grouping
  * member
  * call
  * unary
  * arithmetic
  * factor
  * comparison
  * eq_comparison
  * land
  * lor
*/

struct Parser {
  ArrayView<Token> tokens;
  size_t current = 0;
  Parser(ArrayView<Token>);

  int current_scope_depth = 0;
  bool had_parse_error = false;

  ArrayView<Stmt*> parse(bool* error);
  Stmt* parse_statement();
  Expr* parse_expression();
private:
  void skip_to_global_scope();
  void advance();

  Token peek();
  Token previous();

  void skip_past(TokenType type);

  [[nodiscard]]
  bool eat_token(TokenType, char const * const);

  void parse_error(char const * const, ...);
  bool error_if_match(const char* error, Array<TokenType> match_seq);

  Stmt* statement();
  Block_Stmt* block_stmt();
  If_Stmt* if_stmt();
  For_Stmt* for_stmt();
  Assign_Stmt* assign_stmt();
  Decl_Var_Stmt* decl_var_stmt();
  Decl_Proc_Stmt* decl_proc_stmt();
  Expr_Stmt* expr_stmt();
  Import_Stmt* import_stmt();
  Return_Stmt* return_stmt();

  Stmt* parse_after_identifier();

  Expr* logical_or_expr();
  Expr* logical_and_expr();
  Expr* arithmetic_expr();
  Expr* factor_expr();
  Expr* grouping_expr();
  Expr* unary_expr();
  Expr* comparison_expr();
  Expr* comparison_equality_expr();
  Expr* call_expr();
  Expr* member_expr();
  Expr* primary_expr();
};

void print_ast(ArrayView<Stmt*> program);