#include "common.hpp"

#ifdef _WIN32
#warning "INFO: panic stack traces are not implemented for windows yet"
#else
#include <execinfo.h>  // this is probably fine for the most part
#endif

void stack_trace() {
#ifdef _WIN32
#else
  static void* ptrs[100];

  int count = backtrace(ptrs, 100);
  char** symbols = backtrace_symbols(ptrs, count);
  for (int i = 0; i < count - 1; i++) {  // print everything except this one itself
    fprintf(stderr, "%s\n", symbols[i]);
  }

  free(symbols);  // if we call this we are panicing so we will exit the program anyways so do we need to free?
#endif
}

[[noreturn]]
void panic_and_abort(char const * const message) {
  fprintf(stderr, "PANIC : %s\n", message);
  fprintf(stderr, "stack trace of the compiler: \n");
  stack_trace();
  exit(1);
}

[[noreturn]] void panic_and_abortf(char const * const message, ...) {
  static char formatted_msg[1024];

  va_list args;
  va_start(args, message);
  vsnprintf(formatted_msg, sizeof(formatted_msg), message, args);
  va_end(args);

  panic_and_abort(formatted_msg);
}

static String_Builder scratch;
String_Builder* scratch_string_builder() {
  if (scratch.buffer == NULL)
    scratch = String_Builder(512);

  return &scratch;
}

size_t string_length(char const * s) {
  size_t len = 0;
  while (*s) {
    s++;
    len++;
  }

  return len;
}

void print_string(String string) {
    printf("%*.s", (int)string.size + 1, string.data);
}

void null_terminate(String string, char* buff) {
    snprintf(buff, (int)string.size + 1, "%s", string.data); // snprintf does it for us (+1 is for the terminator)
}

String take_input() {
  static char buffer[1024];
  fgets(buffer, 1024, stdin);
  String content = String(buffer);

  return content;
}

char* number_to_string(double number, int precision /* after decimal point */) {
  // decimal

  int range = 0;
  int pow = 1;
  while ((int)number / pow) {
    range++;
    pow *= 10;
  }

  size_t size = range + 2;
  if (precision) {
    size += precision;
  }
  char* buffer = (char*)malloc(size);

  size_t cursor = 0;
  if (number < 0) {
    buffer[0] = '-';
    cursor++;
  }

  if (!range) {
    buffer[cursor] = '0';
    cursor++;
  }
  long n = (long)number;
  pow /= 10;
  for (int i = 0; i < range; i++) {
    buffer[cursor] = ((n / pow) % 10) + '0';
    cursor++;
    pow /= 10;
  }

  if (precision) {
    buffer[cursor] = '.';
    cursor++;
  }

  double mantissa = (number - n);
  for (int i = 0; i < precision; i++) {
    mantissa *= 10;
    buffer[cursor] = ((long)(mantissa) % 10) + '0';
    cursor++;
  }

  buffer[cursor] = '\0';

  return buffer;
}

bool compare_string(String s1, String s2) {
  if (s1.size != s2.size) return false;
  for (size_t i = 0; i < s1.size; i++) {
    if (s1.data[i] != s2.data[i]) return false;
  }

  return true;
}

bool compare_value(const Value& v1, const Value& v2) {
  if (v1.type != v2.type) return false;

  auto type = v1.type;
  auto v = v1.value;
  auto u = v2.value;

  if (type == Value::STRING) {
    return compare_string(v.string, u.string);
  } else if (type == Value::REAL) {
    return v.real == u.real;  // @xxx floating point compare @fixme
  } else if (type == Value::INTEGER) {
    return v.integer == u.integer;
  } else if (type == Value::BOOLEAN) {
    return v.boolean == u.boolean;
  } /*else if (type == Value::NIL) {
    return false;
  }
  */

  return false;
}

void* malloc_or_die(size_t size) {
  void* mem = malloc(size);
  if (!mem) panic_and_abort("Failed malloc");

  return mem;
}

int hash_string(const String& string) {
    int result = 0;
    int pow = 1;

    for (auto i = string.size - 1; i > 0; i--, pow *= 31) {
        result += string.data[i] * pow;
    }

    result += string.data[0] * pow;

    return result;
}

const char* ordinal_string(int n) {
    static char buffer[16];  // enough for large int + suffix + null
    const char *suffix = "th";
    int last_two = n % 100;

    if (last_two < 11 || last_two > 13) {
        switch (n % 10) {
            case 1: suffix = "st"; break;
            case 2: suffix = "nd"; break;
            case 3: suffix = "rd"; break;
        }
    }

    snprintf(buffer, sizeof(buffer), "%d%s", n, suffix);
    return buffer;
}

const size_t wordsize = sizeof(void*);
uint64_t next_multiple_of_wordsize(uint64_t n) {
  return ((n-1)|(wordsize-1)) + 1;
}
