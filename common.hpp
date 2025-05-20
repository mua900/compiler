#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#define MAX(x,y) (((x) < (y)) ? (x) : (y))
#define MIN(x,y) (((x) < (y)) ? (y) : (x))

[[noreturn]] void panic_and_abort(char const * const message);
[[noreturn]] void panic_and_abortf(char const * const message, ...);

size_t string_length(char const * s);

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

void* malloc_or_die(size_t size);
uint64_t next_multiple_of_wordsize(uint64_t n);

typedef struct {
  int offset;
  int line;
} location_t;

// owns its own memory
struct Mutable_String {
  char* data;
  size_t size = 0;
  size_t capacity;

  Mutable_String() : data(NULL), size(0), capacity(0) {}
  explicit Mutable_String(size_t init_cap) : capacity(init_cap) {
    data = new char[init_cap];
  }

  // @xxx bug prone?
  Mutable_String(char* data, size_t size, size_t capacity) : data(data), size(size), capacity(capacity) {}
  Mutable_String(const char* source, size_t start, size_t end) {
    assert(end >= start);
    size = end - start;
    data = new char[size];
    capacity = size;
    memcpy(data, &source[start], size);
  }

  void free() {
      delete[] data;
  }

  // @cleanup
  bool equals(const char* s) {
    size_t i = 0;
    while (*s) {
      if (!(i < size)) return false;
      if (*s != data[i]) {
        return false;
      }
      i++;
      s++;
    }

    if (i != size) return false;

    return true;
  }
};

struct String {
  char const * data = NULL;
  size_t size = 0;

  String() = default;
  String(Mutable_String mut) : data(mut.data), size(mut.size) {}
  String(char const * data, size_t size) : data(data), size(size) {}
  constexpr String(char const * data) : data(data) {  // there is a lot of implicit use of this
    if (data) size = string_length(data);
  }

  void trim_character(char c) {
    if (data[size - 1] == c) {
      size--;
    }
  }

  inline char get(size_t i) const {
    if (i >= size) return '\0';
    return data[i];
  }

  void trim(char c) {
    if (data[size - 1] == c) {
      size--;
    }
  }

  bool equals(const char* s) {
    size_t i = 0;
    while (*s) {
      if (!(i < size)) return false;
      if (*s != data[i]) {
        return false;
      }
      i++;
      s++;
    }

    if (i != size) return false;

    return true;
  }
};

void print_string(String string);
// @cleanup most usage of null_terminate
void null_terminate(String string, char* buffer);

struct String_Builder {
  char* buffer = NULL;
  size_t buffer_capacity;
  size_t cursor = 0;

  String_Builder(size_t initial_size) {
    buffer = new char[initial_size];
    buffer_capacity = initial_size;
    buffer[0] = '\0';
  }

  // string is equevilant of string_view, mutable_string is a string that owns its own memory

  // very bad if you free after calling these
  String to_string() {
    return String(buffer, cursor);
  }

  Mutable_String to_mut_string() {
    return Mutable_String((char*)buffer, cursor, buffer_capacity);
  }

  void appendf(const char* s, ...) {
    va_list args;
    va_start(args, s);

    int size = string_length(s);
    grow_to_size(cursor + size);

    int writen = vsnprintf(buffer + cursor, size, s, args);
/*
    if (writen < size) {
      panic_and_abort("Internal : Couldn't write the entire string in string builder appendf");
    }
*/

    va_end(args);
  }

  void print() {
    printf("%s", buffer);
  }

  void clear_and_append(String s) {
    cursor = 0;
    append(s);
  }

  const char* c_string() {
    buffer[cursor] = '\0';
    return buffer;
  }

  void resize() {
      char* nbuff = new char[buffer_capacity * 2];
      memcpy(nbuff, buffer, cursor);
      buffer = nbuff;
      buffer_capacity *= 2;
  }

  void grow_to_size(size_t size) {
    int count = 0;
    while (cursor + size >= buffer_capacity) {
      resize();
      count++;
      if (count > 5) {
        panic_and_abortf("String builder buffer resize failed repeatedly: Possible memory allocation issue or corrupted buffer state.\n"
                        "Relevant: buffer_capacity: %zu, cursor: %zu, provided string size: %zu",
                        buffer_capacity, cursor, size);
      }
    }
  }

  void append(String s) {
    grow_to_size(cursor + s.size);

    memcpy(buffer + cursor, s.data, s.size);
    cursor += s.size;
  }

  void free() {
    delete[] buffer;
  }

  void clear() {
    cursor = 0;
    buffer[0] = '\0';
  }
};

bool compare_string(String, String);

String take_input();

char* number_to_string(double number, int precision /* after decimal point */);

typedef uint8_t nil_t;
struct Value {
  enum {
    INTEGER, REAL, STRING, BOOLEAN, NIL
  } type;
  union {
    long integer;
    double real;
    String string;
    bool boolean;
    nil_t nil;
  } value;

  Value(const Value& val) : type(val.type), value(val.value) {}

  Value() : type(NIL), value({}) {
    value.nil = 0;
  }

  Value(long i) : type(INTEGER), value({}) {
    value.integer = i;
  }

  Value(bool b) : type(BOOLEAN), value({}) {
    value.boolean = b;
  }

  Value(double n) : type(REAL), value({}) {
      value.real = n;
  }

  Value(String s) : type(STRING), value({}) {
    value.string = s;
  }

  void print() {
    switch (type) {
    case REAL:
      printf("Real value %f\n", value.real);
      break;
    case INTEGER:
      printf("Integer value %ld\n", value.integer);
      break;
    case STRING:
      printf("String value %s\n", value.string.data);
      break;
    case BOOLEAN:
      printf("Boolean value %s\n", value.boolean ? "true" : "false");
      break;
    case NIL:
      printf("Nil value\n");
      break;
    }
  }

  // @fixme we already have the original string, we shouldn't need this remove
  String string() const {
    switch (type) {
    case REAL: {
      char* buff = (char*)malloc(100);
      int writen = snprintf(buff, 100, "%f", value.real);
      return String(buff, writen);
    }
    case INTEGER: {
      char* buff = (char*)malloc(100);
      int writen = snprintf(buff, 100, "%ld", value.integer);
      return String(buff, writen);
    }
    case STRING:
      return value.string;
    case BOOLEAN:
      return value.boolean ? String("true", 4) : String("false", 5);
    case NIL:
      return String("nil", 3);
    }

    panic_and_abort("Invalid value type");
  }
};

bool compare_value(const Value&, const Value&);

int hash_string(const String& string);
const char* ordinal_string(int n);
