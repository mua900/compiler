#include <cstdio>
#include <cstddef>
#include <cstring>

#define LA_IMPLEMENTATION
#include "linear_allocator.h"

#include "graph.hpp"
#include "stmt.hpp"
#include "parser.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "common.hpp"
#include "resolve.hpp"
#include "sema.hpp"
#include "type.hpp"
#include "typechecker.hpp"
#include "ir.hpp"
#include "c_emitter.hpp"
#include "bytecode.hpp"
#include "bytecode_emitter.hpp"

struct Options {
  bool verbose = false;   // @unused
  bool c_output = false;  // transpile to C
  bool stdout = false;
  // @todo bool tree_walk = false;  // don't compile interpret
  bool generate_dot_file = false;  // generate graphiz dot file.

  bool dump_lexer_output = false;  // dump the token stream outputed by the lexer
  bool parse_expr = false;  // only parse a single expression (for repl)
  bool lexer_only = false;  // stop after lexer
  bool parse_only = false;  // stop after parsing
  bool print_ast = false;  // print the resulting ast

  bool test_bytecode = false;
  bool test_typecheck = false;
  bool test_name_resolution = false;
};

struct File {
  FILE* handle = NULL;
  //bool open : 1; if the handle is null or not
  char* name = NULL;
  bool has_main = false;
  // int package;  @todo
};

struct Context {
  FILE* output_file = NULL;
  const char* output_file_name = NULL;

  DArray<File> files;

  const char* dot_file_name = NULL;
};

static int active_options(Options ops) {
  int count = 0;
  if (ops.c_output) count++;
  if (ops.generate_dot_file) count++;
  if (ops.dump_lexer_output) count++;
  if (ops.parse_expr) count++;
  if (ops.lexer_only) count++;
  if (ops.parse_only) count++;
  if (ops.print_ast) count++;
  if (ops.test_bytecode) count++;
  if (ops.test_typecheck) count++;

  return count;
}

// @update
void print_configuration(Options ops, Context ctx) {
  printf("Output file name: %s\n", ctx.output_file_name);
  if (ops.generate_dot_file) printf("Dot file name: %s\n", ctx.dot_file_name);

  if (active_options(ops)) {
    printf("Collected command line options:\n");
  }
  if (ops.c_output) printf("c_output\n");
  if (ops.generate_dot_file) printf("generate_dot_file\n");
  if (ops.dump_lexer_output) printf("dump_lexer_output\n");
  if (ops.parse_expr) printf("parse_expr\n");
  if (ops.lexer_only) printf("lexer_only\n");
  if (ops.parse_only) printf("parse_only\n");
  if (ops.print_ast) printf("print_ast\n");
  if (ops.test_bytecode) printf("test_bytecode\n");
  if (ops.test_typecheck) printf("test_typecheck\n");
  printf("\n");
}

void compile(String source, const Options* options, const Context* context);
static bool command_line_argument(char* arg, Options* options);
static DArray<char*> command_line_arguments(int arg_count, char** args, Options* options, Context* context);

bool prompt_iteration(const Options* options, Context* context) {
  printf("> ");
  auto input = take_input();
  input.trim('\n');
  if (input.equals("q") || input.equals("quit")) return false;
  compile(input, options, context);
  return true;
}

void run_prompt(const Options* options, Context* context) {
  if (options->generate_dot_file) {
    prompt_iteration(options, context);
    return;
  }

  while (true) {
    if (!prompt_iteration(options, context)) { break; }
  }

  printf("\nBye\n");
}

size_t filelen(FILE* file) {
  long position = ftell(file);
  fseek(file, 0, SEEK_END);
  long filelen = ftell(file);
  fseek(file, 0, position);
  return filelen;
}

void handle_file(char* path, Options* options, Context* context) {
  FILE* file = fopen(path, "rb");

  if (!file) {
    fprintf(stderr, "Couldn't open file: %s\n", path);
    return;
  }

  context->files.add(File{ .handle = file, .name = path, .has_main = false });

  size_t flen = filelen(file);
  Mutable_String content = Mutable_String(flen);
  size_t read = fread(content.data, 1, flen, file);
  content.size = read;

  if (read != flen) {
    fprintf(stderr, "Couldn't read the entire file %s  %zu out of %zu\n", path, read, flen);
    return;
  }

  fclose(file);

  options->parse_expr = false;
  compile(String(content.data, content.size), options, context);
}

void compile(const String source, const Options* options, const Context* context) {
  bool error = false;
  ArrayView<Token> tokens = lex(source, &error);

  if (options->dump_lexer_output) {
    printf("Collected %zu tokens\n", tokens.count);
    for (const Token& token : tokens) {
      token.print();
    }
  }

  if (error) return;  // if we encountered an error during lexing don't continue
  if (options->lexer_only) return;

  Parser parser = Parser(tokens);

  if (options->parse_expr) {
    Expr* expr = parser.parse_expression();
    print_expr(expr);
    printf("\n");

    if (options->generate_dot_file) {
      if (!expression_tree_to_dot(expr, context->dot_file_name)) {
        printf("Failed to generate dot file\n");
      }
    }

    return;
  }

  ArrayView<Stmt*> statements = parser.parse(&error);
  if (error) {
    return;
  }

  if (options->print_ast) {
    print_ast(statements, "program_name");
  }

  if (options->parse_only)
    return;

  Resolver resolver = Resolver(statements);
  ArrayView<Environment> declarations = resolver.resolve();

  if (options->test_name_resolution) {
    resolver.dump_environments();
    return;
  }

  Typechecker typechecker = Typechecker(declarations);
  bool typecheck_result = typechecker.typecheck(statements, declarations);

  if (!typecheck_result)
    return;

  // @todo
  //semantic_analysis(statements, declarations);

  if (options->c_output) {
    output_c_code(statements, declarations, context->output_file);
    return;
  }

  // @todo ir -> bytecode -> run bytecode, backend codegen
}

int main(int argc, char** argv) {
  Options options;
  Context context;
  String self_name = String(argv[0]);

  DArray<char*> files = command_line_arguments(argc - 1, argv+1, &options, &context);
  if (options.stdout) {
    context.output_file = stdout;
    context.output_file_name = "stdout";  // overwrite if both stdout and an output file name is specified
  } else {
    if (!context.output_file_name) {
      context.output_file_name = "output";
    }

    context.output_file = fopen(context.output_file_name, "w");
    if (!context.output_file) {
      // @todo better error
      fprintf(stderr, "Couldn't open file %s for writing\n", context.output_file_name);
      return 1;
    }
  }

  assert(context.output_file_name);

  print_configuration(options, context);

  if (options.test_bytecode) {
    test_bytecode();
    return 0;
  }

  if (files.size == 0) {
    printf("No input files provided\n");
    run_prompt(&options, &context);
  }
  else {
    context.files = DArray<File>(files.size);  // preallocate

    for (auto file : files) {
      printf("processing file : %s\n", file);

      handle_file(file, &options, &context);
    }
  }

  return 0;
}

// @update
static void usage() {
  printf("compiler usage:\n");
  printf("<compiler> [options] filename\n");

  printf("options: \n");
  printf("  --help\n");
  printf("  -verbose or -v\n");
  printf("  -o <output-name>\n");
  printf("  -c-output\n");
  printf("  -stdout\n");
  printf("  -generate-dot-file\n");

  printf("\n");
  printf("  -dump-lexer-output\n");
  printf("  -parse-expr\n");
  printf("  -lexer-only\n");
  printf("  -parse-only\n");
  printf("  -print-ast\n");

  printf("\n");
  printf("  -test-bytecode\n");
  printf("  -test-typecheck\n");
  printf("  -test-name-resolution\n");
  exit(1);
}

// return is the collected filenames
static DArray<char*> command_line_arguments(int arg_count, char** args, Options* options, Context* context) {
  DArray<char*> filenames;

  for (int i = 0; i < arg_count; i++) {
    char* arg = args[i];

    String argument = String(arg);

    if (compare_string(argument, String("--help"))) {
      usage();
    }

    if (compare_string(argument, String("-o"))) {
      if (i + 1 == arg_count) {
        fprintf(stderr, "Usage Error: Expected output file name after -o as an argument\n");
      }

      if (context->output_file_name) {
        fprintf(stderr, "More than one different output filenames provided, the later one (%s) owerwrites the previous one\n", args[i+1]);
      }

      context->output_file_name = args[i+1];
      i += 1;
      continue;
    }

    if (compare_string(argument,          String("-dump-lexer-output"))) {
      options->dump_lexer_output = true;
    } else if (compare_string(argument, String("-stdout"))) {
      options->stdout = true;
    } else if (compare_string(argument, String("-verbose")) || compare_string(argument, String("-v"))) {
      options->verbose = true;
    } else if (compare_string(argument, String("-test-name-resolution"))) {
      options->test_name_resolution = true;
    } else if (compare_string(argument,   String("-ast"))) {
      options->print_ast = true;
    } else if (compare_string(argument,   String("-lexer-only"))) {
      options->lexer_only = true;
    } else if (compare_string(argument,   String("-parse-expr"))) {
      options->parse_expr = true;
    } else if (compare_string(argument,   String("-test-bytecode"))) {
      options->test_bytecode = true;
    } else if (compare_string(argument,   String("-test-typecheck"))) {
      options->test_typecheck = true;
    } else if (compare_string(argument,   String("-parse-only"))) {
      options->parse_only = true;
    } else if (compare_string(argument,   String("-c-output"))) {
      options->c_output = true;
    } else if (compare_string(argument,   String("-generate-dot-file"))) {
      if (i + 1 >= arg_count) {  // if we are at the end we couldn't find the file name as expected
        fprintf(stderr, "Usage Error: Expected filename after -generate-dot-file as an argument\n");
        continue;
      }

      ++i;
      char* filename = args[i];
      context->dot_file_name = filename;

      options->generate_dot_file = true;
    } else {
      filenames.add(arg);
    }
  }

  return filenames;
}
