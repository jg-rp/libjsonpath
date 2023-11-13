#include "libjsonpath/tokens.h"

namespace libjsonpath {
std::ostream& operator<<(std::ostream& os, TokenType const& token_type) {
  switch (token_type) {
  case TokenType::float_:
    return os << "FLOAT";
  case TokenType::false_:
    return os << "false";
  case TokenType::true_:
    return os << "true";
  case TokenType::and_:
    return os << "&&";
  case TokenType::or_:
    return os << "||";
  case TokenType::colon:
    return os << ":";
  case TokenType::comma:
    return os << ",";
  case TokenType::current:
    return os << "@";
  case TokenType::ddot:
    return os << "..";
  case TokenType::eof_:
    return os << "EOF";
  case TokenType::eq:
    return os << "==";
  case TokenType::error:
    return os << "ERROR";
  case TokenType::filter:
    return os << "FILTER";
  case TokenType::func_:
    return os << "FUNC";
  case TokenType::index:
    return os << "INDEX";
  case TokenType::int_:
    return os << "INT";
  case TokenType::ge:
    return os << ">=";
  case TokenType::gt:
    return os << ">";
  case TokenType::le:
    return os << "<=";
  case TokenType::lt:
    return os << "<";
  case TokenType::name:
    return os << "NAME";
  case TokenType::ne:
    return os << "!=";
  case TokenType::null_:
    return os << "null";
  case TokenType::lbracket:
    return os << "[";
  case TokenType::rbracket:
    return os << "]";
  case TokenType::lparen:
    return os << "(";
  case TokenType::rparen:
    return os << ")";
  case TokenType::dq_string:
    return os << "DQ_STRING";
  case TokenType::sq_string:
    return os << "SQ_STRING";
  case TokenType::root:
    return os << "ROOT";
  case TokenType::wild:
    return os << "*";

  default:
    return os << "<?>";
  }
}

bool operator==(const Token& lhs, const Token& rhs) {
  return lhs.type == rhs.type && lhs.value == rhs.value &&
         lhs.index == rhs.index;
}

std::ostream& operator<<(std::ostream& os, Token const& token) {
  return os << "Token{type=" << token.type << ", value=\"" << token.value
            << "\""
            << ", index=" << token.index << "}";
};
} // namespace libjsonpath