#include "typechecker.hpp"
#include "resolve.hpp"

bool is_basic_type(const TokenType type) {
  // @update is_basic_type
  return (type == TokenType::INT || type == TokenType::FLOAT || type == TokenType::STRING);
}

bool type_is_numeric(Type_ID type) {
    return type == Type::INT || type == Type::FLOAT;
}

bool is_procedure_type(Type_ID type) {
    return type & TYPE_PROCEDURE;
}

bool is_structure_type(Type_ID type) {
    return type & TYPE_STRUCTURE;
}

bool type_convertable_to_boolean(Type_ID type) {
    return (!is_structure_type(type)) && (!is_procedure_type(type));  // @fixme
}

Type_ID implicit_convert(Type_ID left_type, Type_ID right_type) {
    return Type::NONE;  // @todo
}

Type_ID value_type(const Value& value) {
    const float eps = 1e-3;
    switch (value.type) {
        case Value::REAL:
            return Type::FLOAT;
        case Value::INTEGER:
            return Type::INT;
        case Value::STRING:
            return Type::STRING;
        case Value::BOOLEAN:
            return Type::BOOLEAN;
        case Value::NIL:
            return Type::NIL;
        default:
            panic_and_abort("Unexpected value type");
    }
}

const char* type_string(Type_ID type) {
    switch (type) {
    case Type::INT:
        return "int";
    case Type::FLOAT:
        return "float";
    case Type::STRING:
        return "string";
    case Type::BOOLEAN:
        return "boolean";
    case Type::NIL:
        return "nil";
    default:
        return "non-basic-type";  // @fixme
    }
}

Type_ID Typechecker::typecheck_expr(Expr* expr) {
    // @fixme location info
    // @fixme better error messages
    switch (expr->type) {
        case ExprType::BINARY: {
            auto binary = static_cast<Binary_Expr*>(expr);
            // by the time we reach here this should be collapsed so that is why we can assert that both branches exist
            assert(binary->left && binary->right);
            Type_ID left_type = typecheck_expr(binary->left);
            Type_ID right_type = typecheck_expr(binary->right);
            if (left_type == right_type) return left_type;

            // should return Type::NONE if they are not convertable
            Type_ID type = implicit_convert(left_type, right_type);
            if (type == Type::NONE) {
                errorf(0, "Types %s and %s are not implicitly convertable inside binary expression", type_string(left_type), type_string(right_type));
            }

            return type;
        }
        case ExprType::UNARY: {
            auto unary = static_cast<Unary_Expr*>(expr);
            Type_ID type = typecheck_expr(unary->operand);

            switch (unary->opperator) {
                case Operator::MINUS:
                    if (!type_is_numeric(type)) {
                        error(0, "Can't negate non-numeric type");
                        return Type::NONE;
                    }

                    return type;
                case Operator::NOT:
                    if (!type_convertable_to_boolean(type)) {
                        errorf(0, "Can't cast type %s to boolean, you can't use unary not", type_string(type));
                        return Type::NONE;
                    }

                    return Type::BOOLEAN;
                default: panic_and_abort("INTERNAL Unexpected operator type for unary expressions during typechecking, this should not be on the tree");
            }
        }
        case ExprType::GROUPING: {
            return typecheck_expr(static_cast<Grouping_Expr*>(expr)->expr);
        }
        case ExprType::VARIABLE: {
            auto var_expr = static_cast<Variable_Expr*>(expr);
            auto variable = curr_env->get_var_from_id(var_expr->var_id);
            printf("%s\n", type_string(variable.type));
            return variable.type;
        }
        case ExprType::LITERAL: {
            auto lit = static_cast<Literal*>(expr);
            return value_type(lit->value);
        }
        case ExprType::CALL: {
    	    // @todo
            // we need types containing detail about return types and argument types of the procedure.
            // a procedure type doesn't do, you need a type like: proc(int, float) -> int, bool;
            panic_and_abort("Typechecking procedure calls not implemented");
            auto call = static_cast<Call_Expr*>(expr);

            Procedure called_proc;

            // the only way its an expression that evaluates to a procedure is that that expression to be a procedure that returns a procedure

            // we need the procedure name to get the procedure from the environment and we need to get some context
            DArray<Expr*> stack;
            stack.add(call);
            Array<Expr*> arguments = call->arguments;
            while (true) {
                auto proc_expr = *stack.last();

                if (proc_expr->type == ExprType::VARIABLE) {
                    Variable_Expr* proc_name = static_cast<Variable_Expr*>(proc_expr);

                    // @todo find a way to get these by index instead of dealing with strings somehow
                    const Procedure* proc = curr_env->get_procedure(proc_name->identifier.lexeme);

                    if (!proc) {
                        panic_and_abortf("Couldn't get procedure %s should not happen after the resolve stage");
                    }

                    if (!is_procedure_type(proc->return_type)) {
                        errorf(0, "The procedure doesn't return a procedure in a call expression chain");  // @fixme error message
                    }

                    // ret_func()(a, b, c)(x, y, z)
                    // variable_expr, call, call

                    stack.pop();  // pop the identifier up top

                    while (stack.size > 0) {
                        // all of them should be call expressions
                        auto top = stack.pop();
                        assert(top->type == ExprType::CALL);
                        auto upper_call = static_cast<Call_Expr*>(top);

                        auto called_proc = curr_env->get_proc_from_id(upper_call->proc_id);

                        if (upper_call->arguments.size != proc->parameters.count) {
                            errorf(0, "Expected %d arguments but got %d", proc->parameters.count, upper_call->arguments.size);
                        }

                        for (int i = 0; i < proc->parameters.count; i++) {
                            Type_ID arg_type = typecheck_expr(upper_call->arguments.get(i));
                            if (proc->parameters.get(i).type != arg_type) {
                                errorf(0, "Type mismatch on %d%s argument of the procedure call to procedure", ordinal_string(i));  // @fixme error message
                                stack.free();
                                return Type::NONE;
                            }
                        }
                    }

                    break;  // the outer while loop
                } else if (proc_expr->type == ExprType::CALL) {
                    auto call_expr = static_cast<Call_Expr*>(proc_expr);
                    stack.add(call_expr->expression);
                } else {
                    panic_and_abort("Internal: call.expression typechecked to procedure but is not itself a procedure call, this should be a bug");
                }
            }

            stack.free();
            return called_proc.return_type;
        }
        case ExprType::MEMBER: {
            panic_and_abort("add structs to the language");
        }
        default: panic_and_abort("BUG : Unhandled expression type in the typecheck_expr");
    }
}

bool Typechecker::typecheck(ArrayView<Stmt*> program, ArrayView<Environment> declarations) {
    bool success = true;

    for (auto stmt : program) {
        bool res = typecheck_statement(stmt);
        if (!res) {
            success = false;
        }
    }

    return success;
}

bool Typechecker::typecheck_statement(Stmt* stmt) {
    switch (stmt->kind) {
        case StmtKind::DECL_VAR: {
            auto decl_var = static_cast<Decl_Var_Stmt *>(stmt);

            Type_ID declared_type = decl_var->decl.type;

            if (decl_var->initializer) {
                Type_ID type = typecheck_expr(decl_var->initializer);

                if (declared_type != type) {
                    errorf(decl_var->decl.name.line, "Expected type %s but initializer is of type %s", type_string(declared_type), type_string(type));
                }
            }

            return true;
        }
        case StmtKind::DECL_PROC: {
            auto decl_proc = static_cast<Decl_Proc_Stmt *>(stmt);

            Procedure proc = decl_proc->scope->get_proc_from_id(decl_proc->proc_id);

            bool success = true;
            for (auto stmt : proc.body) {
                // @xxx @fixme this is not correct for nested scopes inside the procedure
                if (!typecheck_statement(stmt)) {
                    success = false;
                }
            }

            return success;
        }
        case StmtKind::ASSIGN: {
            auto assign = static_cast<Assign_Stmt *>(stmt);

            Variable var = assign->scope->get_var_from_id(assign->var_id);

            Type_ID expr_type = typecheck_expr(assign->rhs);

            bool success = true;
            if (var.type != expr_type) {
                char buff[1024];
                null_terminate(assign->target.lexeme, buff);
                errorf(assign->target.line,
                    "types of left and right hand sides of the assignment doesn't match, variable %s is expected to be of type %s but initializer is of type %s",
                    buff,
                    type_string(var.type), type_string(expr_type));

                success = false;
            }

            return success;
        }
        case StmtKind::BLOCK: {
            auto block = static_cast<Block_Stmt*>(stmt);

            bool success = true;
            for (auto stmt : block->body) {
                if (!typecheck_statement(stmt))
                    success = false;
            }

            return success;
        }
        case StmtKind::IF: {
            auto if_stmt = static_cast<If_Stmt*>(stmt);

            Type_ID cond_type = typecheck_expr(if_stmt->cond);
            bool thenb = typecheck_statement(if_stmt->then_stmt);
            if (!thenb) return false;

            if (if_stmt->else_stmt) {
                bool elseb = typecheck_statement(if_stmt->else_stmt);
                if (!elseb) return false;
            }

            return true;
        }
        case StmtKind::FOR: {
            auto for_stmt = static_cast<For_Stmt*>(stmt);
            return false;
        }
        case StmtKind::EXPRESSION: {
            auto expr_stmt = static_cast<Expr_Stmt*>(stmt);
            return true;
        }
        case StmtKind::RETURN: {
            auto ret_stmt = static_cast<Return_Stmt*>(stmt);
            for (auto ret : ret_stmt->returns) {
                typecheck_expr(ret);
            }

            return true;
        }
        case StmtKind::IMPORT:
            return true;
        default:
            panic_and_abort("BUG : Unhandled statement kind in the typecheck");
    }
}
