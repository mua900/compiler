#include "ir.hpp"
#include "stmt.hpp"
#include "expr.hpp"
#include "linear_allocator.h"
#include "template.hpp"
#include "environment.hpp"

// expression to ir:
/*
    assuming the expressions which can have children are
    at the bottom and consequently only have literal operands

    binary   -> %t = operation operand1(left) operand2(right)
    unary    -> %t = operation operand1(operand)
    grouping -> forces precedence, doesn't generate anything of its own
    variable -> %t (introduce a new temporary on place)
    literal  -> value (substitute as is)
    member   -> @todo this will probably take variable amount of instructions since
                      this will involve offset calculations and possible indirections if we there are pointers
    call     -> param1 %t1
                ...
                paramN %tn
                call procid arity
    // @todo cast
*/

// @fixme so much recursion

int calculate_expression_instruction_count(Expr* expr, const Environment* scope) {
    if (!expr) {
        panic_and_abort("INTERNAL null expression on ir generation, shouldn't be on the tree at this point");
    }

    // @volatile @update expression_instruction_count
    switch (expr->type) {
        case ExprType::BINARY: {
            auto binary = static_cast<Binary_Expr*>(expr);
            // @xxx both branches should exists, if we disable collapsing expression this is going to be a problem
            return calculate_expression_instruction_count(binary->left, scope) +
                   calculate_expression_instruction_count(binary->right, scope) + 1;
        }
        case ExprType::UNARY: {
            auto unary = static_cast<Unary_Expr*>(expr);
            return calculate_expression_instruction_count(unary->operand, scope) + 1;
        }
        case ExprType::GROUPING: {
            auto grouping = static_cast<Grouping_Expr*>(expr);
            return calculate_expression_instruction_count(grouping->expr, scope);
        }
        case ExprType::LITERAL:
        case ExprType::VARIABLE:
            return 1;
        case ExprType::MEMBER: {
            auto member = static_cast<Member_Expr*>(expr);
            return 0;  // @fixme @todo structures in ir
        }
        case ExprType::CALL: {
            auto call = static_cast<Call_Expr*>(expr);
            Procedure called_procedure = scope->get_proc_from_id(call->proc_id);
            return called_procedure.parameters.count + 1;
        }
        default:
            panic_and_abort("Unknown expr type");
    }
}

// to 3AC
ArrayView<IR_Instr> translate_expression(Expr* expr, const Environment* scope) {
    // first calculate the amount of instructions needed for the expression then actually translate
    int count = calculate_expression_instruction_count(expr, scope);
    int curr = count - 1;

    DArray<Expr*> stack;
    stack.add(expr);

    // is this right?
    while (stack.size > 0) {
        auto top = stack.pop();
        switch (top->type) {
            case ExprType::BINARY: {
                auto binary = static_cast<Binary_Expr*>(expr);

                switch (binary->opperator) {
                    case Operator::NONE: {
                        // this shouldn't be in the tree at this point (should already be collapsed)
                        panic_and_abort("INTERNAL Unfilled binary expression in tree on ir generation");
                    }
                    case Operator::PLUS: {

                        break;
                    }
                    case Operator::MINUS: {
                        break;
                    }
                    case Operator::MULT: {
                        break;
                    }
                    case Operator::DIV: {
                        break;
                    }
                    case Operator::MOD: {
                        break;
                    }
                    case Operator::NOT: {
                        break;
                    }
                    case Operator::EQUALS: {
                        break;
                    }
                    case Operator::NOT_EQUALS: {
                        break;
                    }
                }
            }
            case ExprType::UNARY: {
                auto unary = static_cast<Unary_Expr*>(expr);
                break;
            }
            case ExprType::GROUPING: {
                auto grouping = static_cast<Grouping_Expr*>(expr);
                break;
            }
            case ExprType::LITERAL: {
                break;
            }
            case ExprType::VARIABLE: {
                break;
            }
            case ExprType::MEMBER: {
                auto member = static_cast<Member_Expr*>(expr);
                break;
            }
            case ExprType::CALL: {
                auto call = static_cast<Call_Expr*>(expr);
                break;
            }
            default:
                panic_and_abort("Unknown expression type");
        }

        curr--;
    }

    stack.free();

    return ArrayView<IR_Instr>(NULL, 0);
}

ArrayView<IR_Instr> translate(ArrayView<Stmt*> program, ArrayView<Environment> decls) {
    DArray<IR_Instr> nodes;

    for (auto stmt : program) {
        switch (stmt->kind) {
            case StmtKind::DECL_VAR: {
                auto decl_var = static_cast<Decl_Var_Stmt*>(stmt);
                break;
            }
            case StmtKind::DECL_PROC: {
                auto decl_proc = static_cast<Decl_Proc_Stmt*>(stmt);
                break;
            }
            case StmtKind::ASSIGN: {
                auto assign = static_cast<Assign_Stmt*>(stmt);
                break;
            }
            case StmtKind::BLOCK: {
                auto block = static_cast<Block_Stmt*>(stmt);
                break;
            }
            case StmtKind::IF: {
                auto ifs = static_cast<If_Stmt*>(stmt);
                break;
            }
            case StmtKind::FOR: {
                auto fors = static_cast<For_Stmt*>(stmt);
                break;
            }
        }
    }

    return ArrayView<IR_Instr>(nodes.data, nodes.size);
}
