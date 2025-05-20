#pragma once

#include "template.hpp"

struct Stmt;
struct Environment;

void semantic_analysis(ArrayView<Stmt*>, ArrayView<Environment>);