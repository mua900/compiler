#pragma once

// utility that uses graphiz to create visuallizations for several things we would like to do that to.

struct Expr;
bool expression_tree_to_dot(Expr* root, const char* filename);
