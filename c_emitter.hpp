#pragma once

#include "template.hpp"
struct Stmt;
struct Environment;

void output_c_code(ArrayView<Stmt*> program, ArrayView<Environment> decls, FILE* output_file);
