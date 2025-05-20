#include "type.hpp"
#include "token.hpp"

// returns basic type if it is a basic type, NONE otherwise (to be resolved)
Type_ID get_basic_type(const TokenType type) {
    switch (type) {
        case TokenType::INT: return Type::INT;
        case TokenType::FLOAT: return Type::FLOAT;
        case TokenType::STRING: return Type::STRING;
        default: return Type::NONE;
    }
}
