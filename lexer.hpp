#pragma once

#include <cstdlib>
#include <cassert>

#include "log.hpp"
#include "token.hpp"
#include "common.hpp"

struct Source {
  String source;
  size_t current = 0;
  int line = 1;

  bool at_end = false;

  Source(String source) : source(source) {}

  char advance_if_match(char c) {
    if (!(current < source.size)) return '\0';


    if (source.get(current) == c) {
      current++;
      if (c == '\n') line++;
      return c;
    }

    return '\0';
  }

  char next() {
    if (!(current < source.size)) {
      at_end = true;
      return '\0';
    }

    if (source.get(current) == '\n') line++;
    current++;
    return source.get(current);
  }

  char advance() {
    if (!(current < source.size)) {
      at_end = true;
      return '\0';
    }

    if (source.get(current) == '\n') line++;
    current++;
    return source.get(current - 1);
  }

  char peek() const {
    if (!(current + 1 < source.size)) return '\0';

    return source.get(current + 1);
  }

  char get() const {
    if (!(current < source.size)) return '\0';

    return source.get(current);
  }

  char previous() const {
    return current ? source.get(current - 1) : '\0';
  }
};

static inline bool is_digit_ascii(char c) {
  return c <= '9' && c >= '0';
}

static inline bool is_alpha_ascii(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool is_alpha_numeric_ascii(char c) {
  return is_digit_ascii(c) || is_alpha_ascii(c);
}

static inline bool is_valid_identifier_char(char c) {
  return is_alpha_numeric_ascii(c) || c == '_';
}

static inline bool is_valid_identifier_start(char c) {
  return is_alpha_ascii(c) || c == '_';
}

void lex_number(DArray<Token>&, Source&);
void lex_ident(DArray<Token>&, Source&);
void lex_string_literal(DArray<Token>&, Source&);
void multilinecomment(Source&);

bool handle_character(DArray<Token>& tokens, Source& source, char c);

ArrayView<Token> lex(String src, bool* error) {
  DArray<Token> tokens;

  Source source(src);

  while (source.get() != '\0') {
    char c = source.get();
    if (!handle_character(tokens, source, c))
      *error = true;
  }

  tokens.add(Token(String(), TokenType::END, Value(), source.line, (int)source.current));

  return ArrayView<Token>(tokens.data, tokens.size);
}

static inline void add_simple_token(Token&& token, DArray<Token>& tokens, Source& source) {
  tokens.add(token);
  source.advance();
}

// return success code
bool handle_character(DArray<Token>& tokens, Source& source, char c) {
    switch (c) {
    case ' ':
    case '\t':
    case '\n':
      source.advance();
      break;

    case '.':
      add_simple_token(Token(String(".", 1), TokenType::DOT, Value(), source.line, (int)source.current), tokens, source);
      break;
    case '%':
      add_simple_token(Token(String("%", 1), TokenType::PERCENT, Value(), source.line,(int)source.current), tokens, source);
      break;

    case '-':
      add_simple_token(Token(String("-", 1), TokenType::MINUS, Value(), source.line,(int)source.current), tokens, source);
      break;
    case '+':
      add_simple_token(Token(String("+", 1), TokenType::PLUS, Value(), source.line,(int)source.current), tokens, source);
      break;
    case '*':
      add_simple_token(Token(String("*", 1), TokenType::STAR, Value(), source.line,(int)source.current), tokens, source);
      break;

    case ',':
      add_simple_token(Token(String(",", 1), TokenType::COMMA, Value(), source.line,(int)source.current), tokens, source);
      break;
    case ':':
      add_simple_token(Token(String(":", 1), TokenType::COLON, Value(), source.line,(int)source.current), tokens, source);
      break;
    case '(':
      add_simple_token(Token(String("(",1), TokenType::PAREN_LEFT, Value(), source.line,(int)source.current), tokens, source);
      break;
    case ')':
      add_simple_token(Token(String(")",1), TokenType::PAREN_RIGHT, Value(), source.line,(int)source.current), tokens, source);
      break;
    case '{':
      add_simple_token(Token(String("{",1), TokenType::BRACE_LEFT, Value(), source.line, (int)source.current), tokens, source);
      break;
    case '}':
      add_simple_token(Token(String("}"), TokenType::BRACE_RIGHT, Value(), source.line, (int)source.current), tokens, source);
      break;
    case '[':
      add_simple_token(Token(String("["), TokenType::SQUARE_LEFT, Value(), source.line, (int)source.current), tokens, source);
      break;
    case ']':
      add_simple_token(Token(String("]"), TokenType::SQUARE_RIGHT, Value(), source.line, (int)source.current), tokens, source);
      break;
    case ';':
      add_simple_token(Token(String(";"), TokenType::SEMICOLON, Value(), source.line, (int)source.current), tokens, source);
      break;
    case '!': {
      source.advance();
      if (source.get() == '=') {
        add_simple_token(Token(String("!="), TokenType::EXCLAMATION_EQUAL, Value(), source.line, (int)source.current), tokens, source);
      }
      else tokens.add(Token(String("!"), TokenType::EXCLAMATION, Value(), source.line, (int)source.current));
    } break;
    case '=': {
      source.advance();
      if (source.get() == '=') {
        add_simple_token(Token(String("==",2), TokenType::EQUAL_EQUAL, Value(), source.line, (int)source.current), tokens, source);
      }
      else tokens.add({"=", TokenType::EQUAL, Value(), source.line, (int)source.current});
    } break;
    case '>': {
      source.advance();
      if (source.get() == '=') {
        add_simple_token({">=", TokenType::GREATER_EQUAL, Value(), source.line, (int)source.current}, tokens, source);
      }
      else tokens.add({">", TokenType::GREATER, Value(), source.line, (int)source.current});
    } break;
    case '<': {
      source.advance();
      if (source.get() == '=') {
        add_simple_token({"<=", TokenType::LESS_EQUAL, Value(), source.line, (int)source.current}, tokens, source);
      }
      else {
        tokens.add({"<", TokenType::LESS, Value(), source.line, (int)source.current});
      }
    } break;
    case '#': {
      add_simple_token({"#", TokenType::HASH, Value(), source.line, (int)source.current}, tokens, source);
      break;
    }
    case '"': {
      lex_string_literal(tokens, source);
    } break;
    case '/': {
      if (source.peek() == '/') {
        while (source.advance() != '\n') {
          if (source.get() == '\0') break;
        }
        if (source.previous() != '\n') {
          warning(source.line, "No newline found at the end while processing comment");
        }
      }
      else if (source.get() == '*') {
        multilinecomment(source);
      }
      else {
        add_simple_token(Token(String("/"), TokenType::SLASH, Value(), source.line, (int)source.current), tokens, source);
      }
    } break;
    default: {
      if (is_digit_ascii(c)) {
        lex_number(tokens, source);
      }
      else if (is_valid_identifier_start(c)) {
        lex_ident(tokens, source);
      }
      else {
        static int count = 0;
        errorf(source.line, "Unexpected character : %c", source.get());
        source.advance();
        count++;
        if (count > 100) panic_and_abort("Bad input string");
        return false;
      }
    } break;
    }

    return true;
}

void lex_number(DArray<Token>& tokens, Source& source) {
  size_t start = (int)source.current;
  while (is_digit_ascii(source.next())) {}
  if (source.get() == '.') {  // floating point
    while (is_digit_ascii(source.next())) {}
    Mutable_String number_str = Mutable_String(source.source.data, start, (int)source.current);
    double result = strtod(number_str.data, NULL);   // TODO do this yourself
    tokens.add(Token(String(number_str.data, number_str.size), TokenType::NUMERIC_LITERAL, Value(result), source.line, source.current));
  } else {
    Mutable_String number_str = Mutable_String(source.source.data, start, (int)source.current);
    long result = atoi(number_str.data);
    tokens.add(Token(String(number_str.data, number_str.size), TokenType::NUMERIC_LITERAL, Value(result), source.line, source.current));
  }
}

struct token_type_string {
    TokenType type;
    const char* string;
};

token_type_string reserved[] = {
    {TokenType::VAR, "var"},
    {TokenType::FOR, "for"},
    {TokenType::WHILE, "while"},
    {TokenType::TRUE, "true"},
    {TokenType::FALSE, "false"},
    {TokenType::RETURN, "return"},
    {TokenType::OR, "or"},
    {TokenType::AND, "and"},
    {TokenType::IF, "if"},
    {TokenType::ELSE, "else"},
    {TokenType::PROC, "proc"},
    {TokenType::IMPORT, "import"},
    {TokenType::INT, "int"},
    {TokenType::FLOAT, "float"},
    {TokenType::STRING, "string"},
};

void lex_ident(DArray<Token>& tokens, Source& source) {
    size_t start = (int)source.current;
    while (is_valid_identifier_char(source.next())) {}

    String ident = String(Mutable_String(source.source.data, start, (int)source.current));
    for (size_t i = 0; i < ARRAY_SIZE(reserved); i++) {
        if (ident.equals(reserved[i].string)) {
          tokens.add({reserved[i].string, reserved[i].type, Value(), source.line, (int)source.current});
          return;
        }
    }

    Token identifier = Token(String(ident.data, ident.size), TokenType::IDENTIFIER, Value(), source.line, (int)source.current);
    tokens.add(identifier);
}

void lex_string_literal(DArray<Token>& tokens, Source& source) {
  // current should be pointing at the starting quote
  assert(source.advance() == '"');
  size_t start = (int)source.current;
  while (source.get() && source.advance() != '"') {}

  if (source.get() != '"' && source.at_end) {
    error(source.line, "Unterminated string literal at the end of input");
    return;
  }

  Mutable_String str_lit = Mutable_String(source.source.data, start, (int)source.current - 1);
  // NOTE we chose to not put a dumy lexeme here which means the entire program is not allowed to touch a tokens lexeme without checking its type
  tokens.add(Token((const char*)NULL /* no lexeme for string literals */, TokenType::STRING_LITERAL, Value(String(str_lit.data, str_lit.size)), source.line, (int)source.current));
}

void multilinecomment(Source& source) {
  // assert(source.get() == '/' && source.peek() == '*');
  int nest_count = 1;
  while (source.get()) {
    if (!nest_count) break;
    if (source.advance_if_match('\n') == '\n') {}
    else if (source.get() == '*' && source.peek() == '/') {
      source.advance();
      nest_count--;
    }
    else if (source.get() == '/' && source.peek() == '*') {
      source.advance();
      nest_count++;
    }

    source.advance();
  }
}
