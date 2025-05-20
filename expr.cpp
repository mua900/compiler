#include "expr.hpp"
#include "log.hpp"

#include <cmath>

const char* expr_type_str(const Expr* expr) {
    switch (expr->type) {
        case ExprType::BINARY: return "BINARY";
        case ExprType::UNARY:  return "UNARY";
        case ExprType::GROUPING: return "GROUPING";
        case ExprType::LITERAL: return "LITERAL";
        case ExprType::VARIABLE: return "VARIABLE";
        default: panic_and_abort("Invalid expression type");
    }
}

// if we actually store or know where to find what we have from the textual input, do we need to parse everything translate to a tree and make it into a string again?
void expression_string(Expr* expression, String_Builder* builder) {
    builder->clear();

    DArray<Expr*> stack;
    if (expression)
        stack.add(expression);

    while (stack.size > 0) {
        Expr* current = stack.pop();

        switch (current->type) {
            case ExprType::BINARY: {
                auto binary = static_cast<Binary_Expr*>(current);

                if (binary->right)
                    stack.add(binary->right);
                builder->appendf(" %s ", operator_string(binary->opperator));
                if (binary->left)
                    stack.add(binary->left);
                break;
            }
            case ExprType::UNARY: {
                auto unary = static_cast<Unary_Expr*>(current);
                builder->append(operator_string(unary->opperator));
                if (unary->operand) {
                    stack.add(unary->operand);
                }
                break;
            }
            case ExprType::GROUPING: {
                builder->append("(");
                auto grouping = static_cast<Grouping_Expr*>(current);
                if (grouping->expr) {
                    stack.add(grouping->expr);
                }
                builder->append(")");
                break;
            }
            case ExprType::LITERAL: {
                auto lit = static_cast<Literal*>(current);
                builder->append(lit->value.string());  // @xxx assure this returns correct string
                break;
            }
            case ExprType::VARIABLE: {
                auto var = static_cast<Variable_Expr*>(current);
                builder->append(var->identifier.lexeme);
                break;
            }
            case ExprType::CALL: {
                auto call = static_cast<Call_Expr*>(current);
                if (call->expression) {
                    if (call->expression->type == ExprType::VARIABLE) {
                        Variable_Expr* proc = static_cast<Variable_Expr*>(call->expression);  // @xxx maybe i need to rename variable expression
                        builder->append(proc->identifier.lexeme);

                        // @hack
                        // expression string clears the string builder it receives and manages its own stack so we can't just call it again to append on top of the existing builder
                        // which is why this introduces a new sb but extra heap allocation isn't nice

                        builder->append("(");

                        String_Builder arguments_sb(512);
                        for (int i = call->arguments.size - 1; i >= 1; i--) {
                            expression_string(call->arguments.data[i], &arguments_sb);
                            builder->append(arguments_sb.to_string());
                            builder->append(",");
                        }
                        expression_string(call->arguments.data[0], &arguments_sb);
                        builder->append(arguments_sb.to_string());

                        builder->append(")");
                    }
                    else {
                        stack.add(call->expression);
                    }
                } else {
                    panic_and_abort("Internal: tree shouldn't have a call expression with null expression");
                }
                break;
            }
            case ExprType::MEMBER: {
                panic_and_abort("Member expressions not implemented");
                break;
            }
            default: panic_and_abort("Invalid expression type in expression to string");
        }
    }
}

// @xxx do we need this?
void expression_human_readable_string(Expr* expr, String_Builder* builder) {
    DArray<Expr*> stack;
    if (expr) stack.add(expr);

    int indent = 0;

    while (stack.size > 0) {
        Expr* current = stack.pop();

        builder->append("\n");
        for (int i = 0; i < indent; i++) {  // append_many or something like that could be usefull
            builder->append("\t");
        }

        switch (current->type) {
            case ExprType::BINARY: {
                auto binary = static_cast<Binary_Expr*>(current);
                builder->append("Binary expression: ");
                builder->append("operator : ");
                builder->append(operator_string(binary->opperator));
                builder->append("\n");

                if (binary->left)  stack.add(binary->left);
                if (binary->right) stack.add(binary->right);
                if (binary->left || binary->right) indent++;
                break;
            }
            case ExprType::UNARY: {
                builder->append("Unary expression: ");

                auto unary = static_cast<Unary_Expr*>(current);

                builder->append(operator_string(unary->opperator));

                if (unary->operand) {
                    builder->append("operand: ");
                    stack.add(unary->operand);
                    indent++;
                }

                break;
            }
            case ExprType::GROUPING: {
                builder->append("Grouping expression: ");

                auto grouping = static_cast<Grouping_Expr*>(current);

                if (grouping->expr) {
                    builder->append("wrapped expression: ");
                    stack.add(grouping->expr);
                    indent++;
                }

                break;
            }
            case ExprType::LITERAL: {
                auto lit = static_cast<Literal*>(current);
                builder->append("Literal expression : ");
                builder->append(lit->value.string());

                break;
            }
            case ExprType::VARIABLE: {
                auto var = static_cast<Variable_Expr*>(current);
                builder->append("Variable expression : ");
                builder->append(var->identifier.lexeme);
                break;
            }
            case ExprType::CALL: {
                auto call = static_cast<Call_Expr*>(current);
                builder->append("Call expression : ");

                if (call->expression) {
                    if (call->expression->type == ExprType::VARIABLE) {
                        Variable_Expr* proc = static_cast<Variable_Expr*>(call->expression);  // maybe i need to rename variable expression
                        builder->append("calling procedure of name: ");
                        builder->append(proc->identifier.lexeme);

                        builder->append("arguments : ");
                        for (int i = call->arguments.size - 1; i >= 0; i--) {
                            stack.add(call->arguments.get(i));  // @test
                        }
                    }
                    else {
                        builder->append("expression: \n");
                        stack.add(call->expression);
                    }
                } else {
                    panic_and_abort("Internal: tree shouldn't have a call expression with null expression");
                }
                break;
            }
            case ExprType::MEMBER: {
                auto member = static_cast<Member_Expr*>(current);
                builder->append("Member expression : ");

                if (member->expression) {
                    if (member->expression->type == ExprType::VARIABLE) {
                        Variable_Expr* var = static_cast<Variable_Expr*>(member->expression);
                        builder->append("accessing member ");
                        builder->append(member->member.lexeme);
                        builder->append(" of variable ");
                        builder->append(var->identifier.lexeme);
                    }
                    else {
                        builder->append("expression: ");
                        stack.add(member->expression);
                    }
                } else {
                    panic_and_abort("Internal: tree shouldn't have a member expression with null expression");
                }
                break;
            }
            default: panic_and_abort("Invalid expression type in expression to string");
        }
    }
}

int expr_deep(Expr* expr) {
    if (!expr) return 0;

    switch (expr->type) {
        case ExprType::BINARY: {
            auto binary = static_cast<Binary_Expr*>(expr);
            return MAX(expr_deep(binary->left), expr_deep(binary->left)) + 1;
        }
        case ExprType::UNARY: {
            auto unary = static_cast<Unary_Expr*>(expr);
            return expr_deep(unary->operand) + 1;
        }
        case ExprType::GROUPING: {
            auto grouping = static_cast<Grouping_Expr*>(expr);
            return expr_deep(grouping->expr) + 1;
        }
        case ExprType::LITERAL:
        case ExprType::VARIABLE:
            return 1;
        case ExprType::MEMBER: {
            auto member = static_cast<Member_Expr*>(expr);
            return 1 + expr_deep(member->expression);
        }
        case ExprType::CALL: {
            auto call = static_cast<Call_Expr*>(expr);
            return 1 + expr_deep(call->expression);
        }
        default:
            panic_and_abort("Unknown expr type");
    }
}

static void print_expr_real(Expr* expr, int deep);

// @cleanup
void print_expr(Expr* expr) {
    String_Builder sb(512);
    expression_human_readable_string(expr, &sb);
    printf("%s\n", sb.c_string());
    sb.free();

    // print_expr_real(expr, 0);
}

#include <stdarg.h>
void print_with_tabs(const char* format, int tabs, ...) {
    static char buff[1024];  // @xxx size
    va_list args;
    for (int i = 0; i < tabs; ++i) {
        buff[i] = '\t';
    }
    va_start(args, tabs);
    vsnprintf(buff + tabs, 1024, format, args);
    printf("%s\n", buff);
}

static void print_expr_real(Expr* expr, int deep) {
    if (!expr) {
        print_with_tabs("Empty expression", deep);
        return;
    }
#ifdef DEBUG
    print_with_tabs("FROM : %s", deep, expr->source ? expr->source : "PRIMARY");
#endif

    switch (expr->type) {
        case ExprType::BINARY: {
            auto binary = static_cast<Binary_Expr*>(expr);
            print_with_tabs("BINARY EXPR : \n", deep);
            print_expr_real(binary->left, deep + 1);
            print_with_tabs("operator: %s : ", deep, operator_string(binary->opperator));
            print_expr_real(binary->right, deep + 1);
            break;
        }
        case ExprType::UNARY: {
            auto unary = static_cast<Unary_Expr*>(expr);
            print_with_tabs("UNARY EXPR : \n", deep);
            print_with_tabs("operator : %s ", deep, operator_string(unary->opperator));
            print_expr_real(unary->operand, deep + 1);
            break;
        }
        case ExprType::LITERAL: {
            auto lit = static_cast<Literal*>(expr);
            switch (lit->value.type) {
                case Value::REAL:
                    print_with_tabs("Literal value: %f", deep, lit->value.value.real);
                    break;
                case Value::INTEGER:
                    print_with_tabs("Literal value: %ld", deep, lit->value.value.integer);
                    break;
                case Value::STRING:
                    print_with_tabs("Literal value: %s", deep, lit->value.value.string);
                    break;
                case Value::BOOLEAN:
                    print_with_tabs("Literal value: %s", deep, lit->value.value.boolean ? "true" : "false");
                    break;
                case Value::NIL:
                    print_with_tabs("Literal value: %s", deep, lit->value.string().data);
                    break;
            }
            break;
        }
        case ExprType::VARIABLE: {
            auto var = static_cast<Variable_Expr*>(expr);
            print_with_tabs("EXPRESSION VARIABLE : %s\n", deep, var->identifier.lexeme.data);
            break;
        }
        case ExprType::GROUPING: {
            auto grouping = static_cast<Grouping_Expr*>(expr);
            print_with_tabs("GROUPING EXPRESSION : \n", deep);
            print_expr_real(grouping->expr, deep + 1);
            break;
        }
        case ExprType::CALL: {
            auto call = static_cast<Call_Expr*>(expr);
            print_with_tabs("CALL EXPRESSION : \n", deep);
            print_expr_real(call->expression, deep + 1);
            break;
        }
        case ExprType::MEMBER: {
            auto member = static_cast<Call_Expr*>(expr);
            print_with_tabs("MEMBER EXPRESSION : \n", deep);
            print_expr_real(member->expression, deep + 1);
            break;
        }
        default:
            panic_and_abort("Unexpected expression type");
    }
}

// @fixme memory

// @todo cleanup and probably move this to typechecker
bool binary_expr_typecheck(Type_ID left, Type_ID right, const Binary_Expr* binary, bool (*proper_type)(Type_ID)) {
    if (!proper_type(left) || !proper_type(right)) {
        errorf(binary->location.line,"Can't use binary operator %s on given types: %s %s", operator_string(binary->opperator), type_string(left), type_string(right));
        return false;
    }

    return true;
}

Expr* collapse_expr(Expr* expr) {
    if (!expr) return NULL;

    auto line = expr->location.line;
    switch (expr->type) {
        case ExprType::BINARY: {
            auto binary = static_cast<Binary_Expr*>(expr);

            if (!binary->left)  return collapse_expr(binary->right);
            if (!binary->right) return collapse_expr(binary->left);

            Expr* left = collapse_expr(binary->left);
            Expr* right = collapse_expr(binary->right);

            if ((!left) || (!right)) {
                return NULL;
            }

            if (left->type == ExprType::LITERAL && right->type == ExprType::LITERAL) {
                Literal* result;
                auto l = static_cast<Literal*>(left);
                auto r = static_cast<Literal*>(right);

                Type_ID ltype = value_type(l->value);
                Type_ID rtype = value_type(r->value);

                auto is_numeric_type = [](Type_ID type) {
                    return type == Type::FLOAT || type == Type::INT;
                };

                switch (binary->opperator) {
                    /*
                    operators:
                      NONE
                      PLUS
                      MINUS
                      MULT
                      DIV
                      MOD
                      NOT
                      EQUALS
                      NOT_EQUALS
                      LESS
                      GREATER
                      LESS_EQUAL
                      GREATER_EQUAL
                    */
                    case Operator::PLUS: {
                        // @todo this needs more complete logic
                        if (!binary_expr_typecheck(ltype, rtype, binary, is_numeric_type)) {
                             return NULL;
                        }

                        if (l->value.type == Value::REAL) {
                            return new Literal(l->value.value.real + r->value.value.real);
                        } else if (l->value.type == Value::INTEGER) {
                            return new Literal(l->value.value.integer + r->value.value.integer);
                        }
                    }
                    case Operator::MINUS: {
                        if (!binary_expr_typecheck(ltype, rtype, binary, is_numeric_type)) {
                            return NULL;
                        }

                        if (l->value.type == Value::REAL) {
                            return new Literal(l->value.value.real - r->value.value.real);
                        } else if (l->value.type == Value::INTEGER) {
                            return new Literal(l->value.value.integer - r->value.value.integer);
                        }
                    }
                    case Operator::MULT: {
                        if (!binary_expr_typecheck(ltype, rtype, binary, is_numeric_type)) {
                            errorf(binary->location.line,"Can't use binary operator %s on given types: %s %s", operator_string(binary->opperator), type_string(ltype), type_string(rtype));
                            return NULL;
                        }

                        if (l->value.type == Value::REAL) {
                            return new Literal(l->value.value.real * r->value.value.real);
                        } else if (l->value.type == Value::INTEGER) {
                            return new Literal(l->value.value.integer * r->value.value.integer);
                        }
                    }
                    case Operator::DIV: {
                        if (!binary_expr_typecheck(ltype, rtype, binary, is_numeric_type)) {
                            errorf(binary->location.line,"Can't use binary operator %s on given types: %s %s", operator_string(binary->opperator), type_string(ltype), type_string(rtype));
                            return NULL;
                        }

                        if (r->value.value.integer == 0)
                            warningf(line, "Division by zero");

                        if (l->value.type == Value::REAL) {
                            return new Literal(l->value.value.real / r->value.value.real);
                        } else if (l->value.type == Value::INTEGER) {
                            return new Literal(l->value.value.integer / r->value.value.integer);
                        }
                    }
                    case Operator::MOD: {
                        if (binary_expr_typecheck(ltype, rtype, binary, [](Type_ID type){ return type == Type::INT; })) {
                            return new Literal(l->value.value.integer % r->value.value.integer);
                        }
                        else if (binary_expr_typecheck(ltype, rtype, binary, [](Type_ID type){ return type == Type::FLOAT; })) {
                            return new Literal(fmod(l->value.value.real, r->value.value.real));
                        }
                        else {
                            return NULL;
                        }
                    }
                    case Operator::EQUALS: {
                        if (ltype != rtype) {
                            errorf(line, "Type mismatch for 2 sides of equals operator `==` %s %s", type_string(ltype), type_string(rtype));
                            return NULL;
                        }

                        bool result = compare_value(l->value, r->value);
                        return new Literal(result);
                    }
                    case Operator::NOT_EQUALS: {
                        if (ltype != rtype) {
                            errorf(line, "Type mismatch for 2 sides of not equal operator `!=` %s %s", type_string(ltype), type_string(rtype));
                            return NULL;
                        }
                        bool result = !compare_value(l->value, r->value);
                        return new Literal(result);
                    }
                    case Operator::LESS: {
                        if (!binary_expr_typecheck(ltype, rtype, binary, is_numeric_type)) return NULL;
                    }
                    case Operator::GREATER:
                    case Operator::LESS_EQUAL:
                    case Operator::GREATER_EQUAL:
                      break;
                    case Operator::OR: {
                        if (!binary_expr_typecheck(ltype, rtype, binary, [](Type_ID type){return type == Type::BOOLEAN;}))

                        bool result = l->value.value.boolean || r->value.value.boolean;
                        return new Literal(result);
                    }
                    case Operator::AND: {
                        if (!binary_expr_typecheck(ltype, rtype, binary, [](Type_ID type){return type == Type::BOOLEAN;})) return NULL;

                        bool result = l->value.value.boolean && r->value.value.boolean;
                        return new Literal(result);
                    }
                    default: {
                        panic_and_abortf("BUG: Binary_Expr expression with non-null leafs has operator %s", operator_string(binary->opperator));  // @internal
                    }
                }

                return result;
            }

            binary->left = left;
            binary->right = right;
            return binary;  // if both are not compile time known literals not much we can do
        }
        case ExprType::UNARY: {
            auto unary = static_cast<Unary_Expr*>(expr);
            Expr* result = collapse_expr(unary->operand);
            if (unary->opperator == Operator::NONE) {
                return result;
            } else if (unary->opperator == Operator::MINUS) {

                if (result->type == ExprType::LITERAL) {
                    auto literal = static_cast<Literal*>(result);

                    auto type = value_type(literal->value);
                    if (type == Type::INT) {
                        literal->value.value.integer = - literal->value.value.integer;
                    } else if (type == Type::FLOAT) {
                        literal->value.value.real = - literal->value.value.real;
                    } else {
                        errorf(line, "Can't apply operator `-` on type : %s\n", type_string(type));
                    }

                    return literal;
                }

                unary->operand = result;
                return unary;
            } else if (unary->opperator == Operator::NOT) {

                if (result->type == ExprType::LITERAL) {
                    auto literal = static_cast<Literal*>(result);

                    auto type = value_type(literal->value);
                    if (type != Type::BOOLEAN) {
                        errorf(line, "Can't apply operator `!` on type : %s\n", type_string(type));
                    }

                    literal->value.value.boolean = !literal->value.value.boolean;
                    return literal;
                }

                unary->operand = result;
                return unary;
            } else {
                errorf(line, "Invalid unary operator : %s\n", operator_string(unary->opperator));
                return NULL;
            }
        }
        case ExprType::LITERAL: {
            return expr;
        }
        case ExprType::VARIABLE: {
            return expr;
        }
        case ExprType::GROUPING: {
            auto grouping = static_cast<Grouping_Expr*>(expr);
            grouping->expr = collapse_expr(grouping->expr);
            return grouping->expr;
        }
        case ExprType::CALL: {
            auto call = static_cast<Call_Expr*>(expr);

            call->expression = collapse_expr(call->expression);
            for (int i = 0; i < call->arguments.size; i++) {
                call->arguments.data[i] = collapse_expr(call->arguments.data[i]);
            }
            return call;
        }
        case ExprType::MEMBER: {
            auto member = static_cast<Member_Expr*>(expr);

            member->expression = collapse_expr(member->expression);
            return member;
        }
        default: {
            panic_and_abort("Invalid expression type");
        }
    }
}

// @xxx unused
ArrayView<Expr*> find_subexpressions(Expr* expr, ExprType type) {
    DArray<Expr*> subexprs;

    DArray<Expr*> stack;
    stack.add(expr);

    while (stack.size != 0) {
        auto current = stack.pop();
        if (!current) continue;

        // @note:
        // @volatile pointers
        // if we will return the raw pointers inside the expression,
        // we better be sure the expression is not modified (constant folded for example) and the pointers are still valid after calling this.

        if (current->type == type) subexprs.add(current);

        switch (current->type) {
            case ExprType::BINARY: {
                auto binary = static_cast<Binary_Expr*>(current);
                stack.add(binary->left);
                stack.add(binary->right);
                break;
            }
            case ExprType::UNARY:  {
                auto unary = static_cast<Unary_Expr*>(current);
                stack.add(unary->operand);
                break;
            }
            case ExprType::GROUPING: {
                auto grouping = static_cast<Grouping_Expr*>(current);
                stack.add(grouping->expr);
                break;
            }
            case ExprType::LITERAL:
            case ExprType::VARIABLE:
                break;
            case ExprType::MEMBER: {
                auto member = static_cast<Member_Expr*>(current);
                stack.add(member->expression);
                break;
            }
            case ExprType::CALL: {
                auto call = static_cast<Call_Expr*>(current);
                stack.add(call->expression);
                break;
            }
            default: panic_and_abort("Invalid expression type");
        }
    }

    stack.free();

    return ArrayView<Expr*>(subexprs.data, subexprs.size);
}
