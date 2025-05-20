#include "parser.hpp"

#include "environment.hpp"

#include "token.hpp"
#include "log.hpp"
#include "expr.hpp"

Parser::Parser(ArrayView<Token> tokens) : tokens(tokens) {}

ArrayView<Stmt*> Parser::parse(bool* error) {
    DArray<Stmt*> statements;
    while (current < tokens.count) {
        Token curr_token = tokens.get(current);
        if (curr_token.type == TokenType::END) break;

        Stmt* statement = parse_statement();
        if (!statement) {
            continue;
        }

        statements.add(statement);
    }

    if (current_scope_depth < 0)
        parse_error("Mismatched parenthesis you need to add %d more {", abs(current_scope_depth));
    else if (current_scope_depth > 0)
        parse_error("Mismatched parenthesis you need to add %d more }", abs(current_scope_depth));

    *error = had_parse_error;
    return ArrayView<Stmt*>(statements.data, statements.size);
}

static bool starts_statement(const TokenType type) {
    // @update TokenType
    switch (type) {
        case TokenType::IF:
        case TokenType::FOR:
        case TokenType::IDENTIFIER:
        case TokenType::VAR:
        case TokenType::PROC:
        case TokenType::BRACE_LEFT:
        case TokenType::RETURN:
        case TokenType::IMPORT:
            return true;
        default:
            return false;
    }
}

Stmt* Parser::parse_statement() {
    switch (tokens.get(current).type) {
        case TokenType::IF: {
            return if_stmt();
        }
        case TokenType::FOR: {
             return for_stmt();
        }
        case TokenType::IDENTIFIER: {
            return parse_after_identifier();  // this does some lookup and branches to several cases
        }
        case TokenType::VAR: {
            return decl_var_stmt();
        }
        case TokenType::PROC: {
            return decl_proc_stmt();
        }
        case TokenType::BRACE_LEFT: {
            return block_stmt();
        }
        case TokenType::RETURN: {
            return return_stmt();
        }
        case TokenType::IMPORT: {
            return import_stmt();
        }
        case TokenType::SEMICOLON:  // this is to allow empty statements
            // @todo @fixme warn
            advance();
            return NULL;
        default: {  // a non recognized token to start a statement
            error(tokens.get(current), "Expected statement");
            while (current < tokens.count) {
                auto type = tokens.get(current).type;
                if (starts_statement(type) || type == TokenType::END) {
                    return NULL;
                }
                advance();  // this is so that we don't issue more than one expected statement errors sequentially
            }

            panic_and_abort("Bug in the parser, reached the end of token stream without finding an end marker token");
        }
    }
}

// @xxx there is a lot of dynamic array usage here. if we could replace those use cases with non-dynamic allocation versions we would be happy
// @fixme @fixme we are leaking memory

// { statements* }
Block_Stmt* Parser::block_stmt() {
    advance();  // {

    DArray<Stmt*> statements;
    while (tokens.get(current).type != TokenType::END && tokens.get(current).type != TokenType::BRACE_RIGHT) {
        Stmt* stmt = parse_statement();
        if (!stmt) return NULL;
        statements.add(stmt);
    }

    // the only way the condition here is true is that the current token is the end
    if (!eat_token(TokenType::BRACE_RIGHT, "Expected `}` at the end of block statement")) {
        return NULL;
    }

    return new Block_Stmt(statements.data, statements.size);
}

// if cond-expr then_stmt else else_stmt?
If_Stmt* Parser::if_stmt() {
    If_Stmt* stmt = new If_Stmt;

    advance(); // if token

    stmt->cond = parse_expression();
    if (!stmt->cond) return NULL;

    stmt->then_stmt = parse_statement();
    if (!stmt->then_stmt) return NULL;

    if (tokens.get(current).type == TokenType::ELSE) {
        advance();
        stmt->else_stmt = parse_statement();
        if (!stmt->else_stmt) return NULL;
    }

    return stmt;
}

// for init_expr ; cond_expr ; end_expr loop_body
For_Stmt* Parser::for_stmt() {
    For_Stmt* stmt = new For_Stmt;

    advance();  // for token
    auto line = tokens.get(current).line;

    stmt->condition = parse_expression();
    if (!stmt->condition) return NULL;
    if (!eat_token(TokenType::SEMICOLON, "Expected semicolon after the condition of the for loop")) return NULL;

    stmt->body = parse_statement();
    if (!stmt->body) return NULL;

    return stmt;
}

Stmt* Parser::parse_after_identifier() {
    assert(tokens.get(current).type == TokenType::IDENTIFIER);

    Token token = peek();
    switch (token.type) {
        case TokenType::EQUAL:
            return assign_stmt();
        case TokenType::DOT:         // member access
        case TokenType::PAREN_LEFT:  // function call
            return expr_stmt();  // @test
        default: {
            char proc_name[1024];
            null_terminate(token.lexeme, proc_name);
            error_tokenf(token, "Expected either `.`, `(` or `=` after identifier for a valid statement but found %s instead", proc_name);

            advance();
            return NULL;
        }
    }
}

// identifier = expr;
Assign_Stmt* Parser::assign_stmt() {
    Assign_Stmt* stmt = new Assign_Stmt;

    stmt->target = tokens.get(current);
    advance();

    if (!eat_token(TokenType::EQUAL, "Expected `=` after identifier in assignment")) {
        skip_past(TokenType::SEMICOLON);
        return NULL;
    }

    stmt->rhs = parse_expression();
    if (!stmt->rhs) {
        skip_past(TokenType::SEMICOLON);
        return NULL;
    }

    return stmt;
}

// var identifier : type (= initializer);
Decl_Var_Stmt* Parser::decl_var_stmt() {
    advance();  // var keyword

    Decl_Var var_decl;

    // name : type

    Token name = tokens.get(current);
    if (name.type != TokenType::IDENTIFIER) {
        parse_error("Expected variable name after `var` keyword in variable declaration");
        return NULL;
    }

    advance();

    if (tokens.get(current).type != TokenType::COLON) {
        parse_error("Expected `:` after variable name in variable declaration");
        return NULL;
    }
    advance();  // :

    Token type_ident = tokens.get(current);
    advance();

    if (is_basic_type(type_ident.type) || type_ident.type == TokenType::IDENTIFIER) {
        var_decl.name = name;
        var_decl.type = get_basic_type(type_ident.type);
    } else {
        parse_error("Expected type name after `:` in variable declaration");
        return NULL;
    }

    // @todo type inference

    Expr* initializer = NULL;
    if (tokens.get(current).type == TokenType::EQUAL) {
        advance();

        initializer = parse_expression();
        if (!initializer) {
            skip_past(TokenType::SEMICOLON);
            return NULL;
        }
    }

    if (!eat_token(TokenType::SEMICOLON, "Expected `;` at the end of variable declaration")) {
        return NULL;
    }

    Decl_Var_Stmt* stmt = new Decl_Var_Stmt;
    stmt->decl        = var_decl;
    stmt->initializer = initializer;
    return stmt;
}

void Parser::skip_to_global_scope() {
    while (tokens.get(current).type != TokenType::END && current_scope_depth != 0) {
        advance();
    }

    if (current_scope_depth != 0) {
        // @fixme errors
        if (current_scope_depth < 0)
            parse_error("Mismatched parenthesis you need to add %d more");
        else if (current_scope_depth > 0)
            parse_error("Mismatched parenthesis you need to add %d more");
        return;
    }
}

// proc name(params) { body }
// params -> (identifier(,identifier))
// or
// proc name { body }
Decl_Proc_Stmt* Parser::decl_proc_stmt() {
    auto good = true;

    advance();  // proc keyword

    if (!(tokens.get(current).type == TokenType::IDENTIFIER)) {
        parse_error("Expected function name after func keyword");
        return NULL;
    }

    Token name = tokens.get(current);
    advance();

    ArrayView<Decl_Var> parameters = ArrayView<Decl_Var>(NULL, 0);

    Token after_ident = tokens.get(current);

    if (after_ident.type != TokenType::PAREN_LEFT && after_ident.type != TokenType::BRACE_LEFT) {
        skip_to_global_scope();
        return NULL;
    }

    char proc_name[1024];
    null_terminate(name.lexeme, proc_name);

    if (after_ident.type == TokenType::PAREN_LEFT) {
        advance();  // (
        DArray<Decl_Var> params;  // @fixme memory leak on some control paths

        Decl_Var param;

        do {
            if (tokens.get(current).type == TokenType::PAREN_RIGHT) {
                break;
            }

            if (tokens.get(current).type != TokenType::IDENTIFIER) {
                parse_error("Expected parameter name in parameter list of the procedure declaration for %s", proc_name);
                skip_to_global_scope();
                return NULL;
            }
            param.name = tokens.get(current);
            advance();

            if (!eat_token(TokenType::COLON, "Expected `:` after parameter name in parameter list of the procedure")) {
                skip_to_global_scope();
                return NULL;
            }

            auto type = tokens.get(current);
            if (type.type != TokenType::IDENTIFIER && !is_basic_type(type.type)) {
                parse_error("Expected type name in parameter list of the procedure declaration for %s", proc_name);
                skip_to_global_scope();
                return NULL;
            }
            param.type = get_basic_type(tokens.get(current).type);
            advance();

            params.add(param);

            if (tokens.get(current).type != TokenType::COMMA) break;
            advance();
        } while (true);

        if (!eat_token(TokenType::PAREN_RIGHT, "Expected closing paranthesis after argument list of the function")) {
            skip_to_global_scope();
            return NULL;
        }

        parameters = ArrayView<Decl_Var>(params.data, params.size);
    }

    // @todo debug
    DArray<Decl_Var> rets;
    while (tokens.get(current).type != TokenType::BRACE_LEFT) {
        // @xxx this can have better error reporting with some effort
        Decl_Var ret;
        ret.name = Token();  // a default non-named return value

        if (tokens.get(current).type != TokenType::IDENTIFIER && !is_basic_type(tokens.get(current).type)) {
            parse_error("Expected type name in return type list of the procedure declaration for %s", proc_name);
            skip_to_global_scope();
            return NULL;
        }

        if (peek().type == TokenType::COLON) {
            ret.name = tokens.get(current);
            advance();  // name
            advance();  // :
            Token type = tokens.get(current);
            if (type.type != TokenType::IDENTIFIER) {
                parse_error("Expected typename after `:` in return list of procedure %s", proc_name);
                skip_to_global_scope();
                return NULL;
            }

            ret.type = get_basic_type(type.type);
            advance();  // type
        } else {
            ret.type = get_basic_type(tokens.get(current).type);
            advance();
        }

        rets.add(ret);

        if (tokens.get(current).type == TokenType::COMMA) {
            advance();  // ,
        }
    }

    if (tokens.get(current).type != TokenType::BRACE_LEFT) {
        error(tokens.get(current).line, "Expected `{` at the start of the procedure body");
        good = false;
        return NULL;
    }

    advance();  // {

    DArray<Stmt*> body;
    while (tokens.get(current).type != TokenType::END && tokens.get(current).type != TokenType::BRACE_RIGHT) {
        auto stmt = parse_statement();
        if (!stmt) {
            good = false;
            continue;
        }

        body.add(stmt);
    }

    if (!eat_token(TokenType::BRACE_RIGHT, "Expected closing `}` at the end of procedure body")) {
        good = false;
    }

    auto* stmt = new Decl_Proc_Stmt;
    stmt->body = ArrayView<Stmt*>(body.data, body.size);
    stmt->name = name;
    stmt->parameters = parameters;
    stmt->returns = ArrayView<Decl_Var>(rets.data, rets.size);

    if (!good) {
        body.free();
        return NULL;
    }
    return stmt;
}

Expr_Stmt* Parser::expr_stmt() {
    Expr* expr = parse_expression();  // function call etc. are here
    if (!expr) skip_past(TokenType::SEMICOLON);
    if (tokens.get(current).type != TokenType::SEMICOLON) {  // @todo expression type
        parse_error("Expceted ´;´ after expression statement");
    }
    advance();
    return new Expr_Stmt(expr);
}

Import_Stmt* Parser::import_stmt() {
    advance();  // import

    Token mod = tokens.get(current);
    if (mod.type != TokenType::IDENTIFIER) {
        parse_error("Expected module name in import statement");
        skip_past(TokenType::SEMICOLON);
        return NULL;
    }
    advance();

    if (!eat_token(TokenType::SEMICOLON, "Expected `;` at the end of import statement")) {
        return NULL;
    }

    return new Import_Stmt(mod);
}

// return expr ; (; is optional)
Return_Stmt* Parser::return_stmt() {
    advance();  // return

    Expr* expr = parse_expression();
    if (tokens.get(current).type == TokenType::SEMICOLON)
        advance();
    if (!expr) {
        skip_past(TokenType::SEMICOLON);
        return NULL;
    }

    return new Return_Stmt(expr);
}

// expressions
/*
  1 grouping
  2 unary
  3 arithmetic
  4 comparison
  5 land
  6 lor
*/

Expr* Parser::parse_expression() {
    int line = tokens.get(current).line;
    Expr* expr = logical_or_expr();
    expr = collapse_expr(expr);
    if (expr) expr->location.line = line;
    return expr;
}

Expr* Parser::logical_or_expr() {
    auto left = logical_and_expr();
    if (!(tokens.get(current).type == TokenType::OR)) {
        return left;
    }

    Binary_Expr* lor = new Binary_Expr;
    lor->left = left;

#ifdef DEBUG
    lor->source = "EXPR_OR";
#endif
    while (tokens.get(current).type == TokenType::OR) {
        lor->opperator = token_to_operator(TokenType::OR);
        advance();

        lor->right = logical_or_expr();
    }

    return lor;
}

Expr* Parser::logical_and_expr() {
    auto left = arithmetic_expr();
    if (!(tokens.get(current).type == TokenType::AND)) {
        return left;
    }

    Binary_Expr* land = new Binary_Expr;
    land->left = left;

#ifdef DEBUG
    land->source = "EXPR_AND";
#endif
    while (tokens.get(current).type == TokenType::AND) {
        land->opperator = token_to_operator(TokenType::AND);
        advance();

        land->right = logical_and_expr();
    }

    return land;
}

// multiplication, division ...
Expr* Parser::arithmetic_expr() {
    auto left = factor_expr();
    if (!(tokens.get(current).type == TokenType::MINUS || tokens.get(current).type == TokenType::PLUS)) {
        return left;
    }

    Binary_Expr* arith = new Binary_Expr;
    arith->left = left;

#ifdef DEBUG
    arith->source = "EXPR_ARITH";
#endif
    while (tokens.get(current).type == TokenType::MINUS || tokens.get(current).type == TokenType::PLUS) {
        arith->opperator = token_to_operator(tokens.get(current).type);
        advance();

        arith->right = arithmetic_expr();
    }

    return arith;
}

Expr* Parser::factor_expr() {
    auto left = comparison_expr();
    if (!(tokens.get(current).type == TokenType::STAR || tokens.get(current).type == TokenType::SLASH)) {
        return left;
    }

    Binary_Expr* factor = new Binary_Expr;
    factor->left = left;

#ifdef DEBUG
    factor->source = "EXPR_FACTOR";
#endif
    while (tokens.get(current).type == TokenType::STAR || tokens.get(current).type == TokenType::SLASH) {
        factor->opperator = token_to_operator(tokens.get(current).type);
        advance();
        factor->right = factor_expr();
    }

    return factor;
}

Expr* Parser::comparison_expr() {
    Expr* comp_eq = comparison_equality_expr();
    if (!(tokens.get(current).type == TokenType::LESS
        || tokens.get(current).type == TokenType::GREATER
        || tokens.get(current).type == TokenType::LESS_EQUAL
        || tokens.get(current).type == TokenType::GREATER_EQUAL)) {
      return comp_eq;
    }

    Binary_Expr* comp = new Binary_Expr;

    comp->left = comp_eq;

#ifdef DEBUG
    comp->source = "EXPR_COMP";
#endif

    while (tokens.get(current).type == TokenType::LESS
        || tokens.get(current).type == TokenType::GREATER
        || tokens.get(current).type == TokenType::LESS_EQUAL
        || tokens.get(current).type == TokenType::GREATER_EQUAL) {
        comp->opperator = token_to_operator(tokens.get(current).type);
        advance();

        comp->right = comparison_equality_expr();
    }

    return comp;
}

Expr* Parser::comparison_equality_expr() {
    Expr* unary = unary_expr();
    if (!(tokens.get(current).type == TokenType::EQUAL_EQUAL || tokens.get(current).type == TokenType::EXCLAMATION_EQUAL)) {
      return unary;
    }

    Binary_Expr* comp = new Binary_Expr;

    comp->left = unary;

#ifdef DEBUG
    comp->source = "EXPR_COMP_EQ";
#endif
    while (tokens.get(current).type == TokenType::EQUAL_EQUAL || tokens.get(current).type == TokenType::EXCLAMATION_EQUAL) {
        comp->opperator = token_to_operator(tokens.get(current).type);
        advance();

        comp->right = unary_expr();  // don't allow 3 != 4 == 5
    }

    return comp;
}

Expr* Parser::unary_expr() {
    if (tokens.get(current).type == TokenType::MINUS || tokens.get(current).type == TokenType::EXCLAMATION) {
        if (tokens.get(current).type == TokenType::MINUS || tokens.get(current).type == TokenType::EXCLAMATION) {
            parse_error("Nested unary operators are not supported\n");  // @xxx maybe we want nested unary operators
            while (tokens.get(current).type == TokenType::MINUS || tokens.get(current).type == TokenType::EXCLAMATION) {
                advance();
            }

            return NULL;
        }

        Unary_Expr* unary = new Unary_Expr;
#ifdef DEBUG
        unary->source = "EXPR_UNARY";
#endif
        unary->opperator = token_to_operator(tokens.get(current).type);
        advance();

        unary->operand = call_expr();
        return unary;
    }

    return call_expr();
}

Expr* Parser::call_expr() {
    Expr* expr = member_expr();

    if (tokens.get(current).type == TokenType::PAREN_LEFT) {
        Call_Expr* call = new Call_Expr;
#ifdef DEBUG
        call->source = "EXPR_CALL";
#endif
        call->expression = expr;
        advance();

        DArray<Expr*> arguments;
        while (current < tokens.count && tokens.get(current).type != TokenType::PAREN_RIGHT) {
            Expr* argument = parse_expression();
            if (!argument) {
                parse_error("Faulty expression for call argument");
                Token curr = tokens.get(current);
                while (!(starts_statement(curr.type) || curr.type == TokenType::PAREN_RIGHT)) {
                    advance();
                }
                return NULL;
            }
            arguments.add(argument);

            if (tokens.get(current).type != TokenType::COMMA) {
                break;
            }
            advance();

        }

        if (tokens.get(current).type != TokenType::PAREN_RIGHT) {
            parse_error("Reached end of input while parsing call arguments");
            return NULL;
        }

        advance();
        call->arguments = Array(arguments.data, arguments.size);

        return call;
    }

    return expr;
}

Expr* Parser::member_expr() {
    Expr* expr = grouping_expr();
    if (tokens.get(current).type == TokenType::DOT) {
        advance();
        if (tokens.get(current).type != TokenType::IDENTIFIER) {
            parse_error("Expected member name after `.` in expression");
            advance();
            return NULL;
        }

        Member_Expr* member = new Member_Expr;
#ifdef DEBUG
        member->source = "EXPR_MEMBER";
#endif

        member->expression = expr;
        member->member = tokens.get(current);
        advance();
        return member;
    }

    return expr;
}

Expr* Parser::grouping_expr() {
    if (tokens.get(current).type == TokenType::PAREN_LEFT) {
        advance(); // (
        Expr* expr = parse_expression();
        if (tokens.get(current).type != TokenType::PAREN_RIGHT) {
            error_token(tokens.get(current), "Unmatched parentheses");
        }

        return new Grouping_Expr(expr);
    } else {
        return primary_expr();
    }
}

Expr* Parser::primary_expr() {
    switch (tokens.get(current).type) {
        case TokenType::NUMERIC_LITERAL:
        case TokenType::STRING_LITERAL:
            advance();
            return new Literal(previous().value);
        case TokenType::IDENTIFIER:
            advance();
            return new Variable_Expr(tokens.get(current-1));
        case TokenType::TRUE:
            advance();
            return new Literal(Value(true));
        case TokenType::FALSE:
            advance();
            return new Literal(Value(false));
        default:
            parse_error("Unrecognized token sequence");  // @fixme this should be more helpfull
            return NULL;
    }
}

Token Parser::previous() {
    if (!current) panic_and_abort("Parser trying to reach before the Token stream");
    return tokens.get(current - 1);
}

void Parser::advance() {
    if (current >= tokens.count) {
        printf("Exhausted the token stream\n");  // if we try to access after this it will panic and thats probably what we want.
    }

    if (tokens.get(current).type == TokenType::BRACE_LEFT) {
        current_scope_depth++;
    }
    else if (tokens.get(current).type == TokenType::BRACE_RIGHT) {
        current_scope_depth--;
    }

    current++;
}

Token Parser::peek() {
    if (current + 1 >= tokens.count) {
        printf("Peeking beyond the token stream\n");
    }

    return tokens.get(current + 1);
}

void Parser::parse_error(char const * const msg, ...) {
    had_parse_error = true;

    char formatted_msg[1024];
    va_list args;
    va_start(args, msg);
    vsnprintf(formatted_msg, sizeof(formatted_msg), msg, args);
    va_end(args);

    error(tokens.get(current).line, formatted_msg);
}

bool Parser::eat_token(TokenType type, char const * const msg) {
    if (tokens.get(current).type != type) {
        parse_error(msg);
        return false;
    }

    advance();
    return true;
}

void Parser::skip_past(TokenType type) {
    while (current < tokens.count) {
        auto curr_type = tokens.get(current).type;
        advance();
        if (curr_type == type || starts_statement(curr_type)) break;
    }
}

void print_ast(ArrayView<Stmt*> program, const char* name) {
    printf("Ast of the program %s\n", name);
    printf("Program has %zu top level statements\n", program.count);

    for (auto s : program) {
        print_stmt(s);
    }
}
