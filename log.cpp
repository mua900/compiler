#include "log.hpp"

// @todo cleanup this entire file

static void report(int line, char const * const where, char const * const msg) {
  fprintf(stderr, "[line:%d], %s: %s\n", line, where, msg);
}

static void reportf(int line, char const * const where, char const * const fmsg, ...) {
  char formatted_msg[1024];

  va_list args;
  va_start(args, fmsg);
  vsnprintf(formatted_msg, sizeof(formatted_msg), fmsg, args);
  va_end(args);

  fprintf(stderr, "[line:%d], Error %s %s\n", line, where, formatted_msg);
}

// this changes order of arguments because varargs are terrible
static void reportf_where(int line, char const * const msg, char const * const fwhere, ...) {
  char formatted_where[1024];

  va_list args;
  va_start(args, fwhere);
  vsnprintf(formatted_where, sizeof(formatted_where), fwhere, args);
  va_end(args);

  fprintf(stderr, "[line:%d], Error %s: %s\n", line, formatted_where, msg);
}

void errorf(int line, char const * const fmsg, ...) {
  char formatted_msg[1024];

  printf("ERROR: \n");

  va_list args;
  va_start(args, fmsg);
  vsnprintf(formatted_msg, sizeof(formatted_msg), fmsg, args);
  va_end(args);
  report(line, "", formatted_msg);
}

void error(int line, char const * const msg) {
  report(line, "", msg);
}

void error(Token t, char const * const msg) {
  char buff[1024];
  null_terminate(t.lexeme, buff);
  fprintf(stderr, "ERROR: At token: %s | %s | line: %d  error: %s\n", buff, token_type_str(t.type), t.line, msg);
  t.value.print();
}

void warning(int line, char const * const msg) {
  fprintf(stderr, "WARNING: at line %d %s\n", line, msg);
}

void warningf(int line, char const * const fmsg, ...) {
  char formatted_msg[1024];

  printf("WARNING: at line %d ", line);

  va_list args;
  va_start(args, fmsg);
  vsnprintf(formatted_msg, sizeof(formatted_msg), fmsg, args);
  va_end(args);
  report(line, "", formatted_msg);
}

void error(char const * const msg) {
  report(0, "", msg);     // FIXME location info
}

void log(char const * const msg) {
  fprintf(stdout, "%s\n", msg);
}

void error_token(const Token& token, char const*const msg) {
  token.print();
  if (token.type == TokenType::END) {
    report(token.line, "at end", msg);
  }
  else {
    reportf_where(token.line, msg, "at %s", token.lexeme);
  }
}

void error_tokenf(const Token& token, char const*const fmsg, ...) {
  char formatted[1024];

  va_list args;
  va_start(args, fmsg);
  vsnprintf(formatted, sizeof(formatted), fmsg, args);
  va_end(args);

  token.print();
  if (token.type == TokenType::END) {
    reportf(token.line, "at end", formatted);
  }
  else {
    reportf_where(token.line, formatted, "at %s", token.lexeme);
  }
}

void report_info(char const * const format, ...) {
  char formatted_msg[1024];

  va_list args;
  va_start(args, format);
  vsnprintf(formatted_msg, sizeof(formatted_msg), format, args);
  va_end(args);

  fprintf(stderr, "Info:  %s\n", formatted_msg);
}
