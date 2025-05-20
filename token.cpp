#include "token.hpp"

#include <cstdio>

void Token::print() const {
    char lex[512];
    null_terminate(lexeme, lex);
    char str[512];
    null_terminate(value.string(), str);
    if (type == TokenType::IDENTIFIER) {
        printf("Token : %s | %s | %s | %d\n", lex, token_type_str(type), str, line);
    } else {
        printf("Token : %s | %s | %d\n", token_type_str(type), str, line);
    }
}

// @volatile should match TokenType enum
static const char* TokenTypeStr[(int)TokenType::COUNT] = {
  "NONE",
  "COMMA", "DOT", "COLON", "SEMICOLON", "PAREN_LEFT", "PAREN_RIGHT",
  "BRACE_LEFT", "BRACE_RIGHT", "SQUARE_LEFT", "SQUARE_RIGHT",
  "EXCLAMATION", "EQUAL", "GREATER", "LESS", "SLASH", "PERCENT", "PLUS", "MINUS", "STAR", "HASH",
  "EXCLAMATION_EQUAL", "EQUAL_EQUAL", "GREATER_EQUAL", "LESS_EQUAL",

  "VAR", "FOR", "WHILE", "TRUE", "FALSE", "RETURN", "OR", "AND", "IF", "ELSE", "PROC", "IMPORT",
  "INT", "FLOAT", "STRING",

  "NUMERIC_LITERAL", "STRING_LITERAL", "IDENTIFIER",
  "END"
};

Operator token_to_operator(TokenType type) {
  switch (type) {
     case TokenType::NONE: return Operator::NONE;
     case TokenType::PLUS: return Operator::PLUS;
     case TokenType::MINUS: return Operator::MINUS;
     case TokenType::STAR: return Operator::MULT;
     case TokenType::SLASH: return Operator::DIV;
     case TokenType::PERCENT: return Operator::MOD;
     case TokenType::EXCLAMATION: return Operator::NOT;

     case TokenType::EQUAL_EQUAL: return Operator::EQUALS;
     case TokenType::EXCLAMATION_EQUAL: return Operator::NOT_EQUALS;

     case TokenType::LESS: return Operator::LESS;
     case TokenType::LESS_EQUAL: return Operator::LESS_EQUAL;
     case TokenType::GREATER: return Operator::GREATER;
     case TokenType::GREATER_EQUAL: return Operator::GREATER_EQUAL;

     case TokenType::OR: return Operator::OR;
     case TokenType::AND: return Operator::AND;
     default:
      panic_and_abort("Unknown operator");
  }
}

const char* token_type_str(TokenType type) {
    return TokenTypeStr[(int)type];
}

const char* operator_string(Operator op) {
  // @update Operator
  switch (op) {
    case Operator::NONE: return "NONE";
    case Operator::PLUS: return "PLUS";
    case Operator::MINUS: return "MINUS";
    case Operator::MULT: return "MULT";
    case Operator::DIV: return "DIV";
    case Operator::MOD: return "MOD";
    case Operator::NOT: return "NOT";
    case Operator::EQUALS: return "EQUALS";
    case Operator::NOT_EQUALS: return "NOT_EQUALS";
    case Operator::LESS: return "LESS";
    case Operator::GREATER: return "GREATER";
    case Operator::LESS_EQUAL: return "LESS_EQUAL";
    case Operator::GREATER_EQUAL: return "GREATER_EQUAL";
    case Operator::OR: return "OR";
    case Operator::AND: return "AND";
    default:
      panic_and_abort("Unexpected operator");
  }
}

// @fixme memory leak
String token_to_string(Token token) {
  String_Builder sb = String_Builder(200);

  sb.append(String("lexeme : "));
  sb.append(token.lexeme);
  sb.append(String(" type : "));
  sb.append(token_type_str(token.type));
  sb.append(String(" value : "));
  sb.append(token.value.string());
  sb.append(String(" line : "));
  sb.appendf("%d", token.line);

  return sb.to_string();
}
