#include "libjsonpath/tokens.hpp"
#include <sstream> // std::ostringstream

namespace libjsonpath {

std::string token_type_to_string(TokenType tt) {
  switch (tt) {
  case TokenType::float_:
    return "FLOAT";
  case TokenType::false_:
    return "FALSE";
  case TokenType::true_:
    return "TRUE";
  case TokenType::and_:
    return "AND";
  case TokenType::or_:
    return "OR";
  case TokenType::colon:
    return "COLON";
  case TokenType::comma:
    return "COMMA";
  case TokenType::current:
    return "CURRENT";
  case TokenType::ddot:
    return "DOTDOT";
  case TokenType::eof_:
    return "EOF";
  case TokenType::eq:
    return "EQ";
  case TokenType::error:
    return "ERROR";
  case TokenType::filter_:
    return "FILTER";
  case TokenType::func_:
    return "FUNC";
  case TokenType::index:
    return "INDEX";
  case TokenType::int_:
    return "INT";
  case TokenType::ge:
    return "GE";
  case TokenType::gt:
    return "GT";
  case TokenType::le:
    return "LE";
  case TokenType::lt:
    return "LT";
  case TokenType::name_:
    return "NAME";
  case TokenType::ne:
    return "NE";
  case TokenType::null_:
    return "NULL";
  case TokenType::lbracket:
    return "LBRACKET";
  case TokenType::rbracket:
    return "RBRACKET";
  case TokenType::lparen:
    return "LPAREN";
  case TokenType::rparen:
    return "RPAREN";
  case TokenType::dq_string:
    return "DQ_STRING";
  case TokenType::sq_string:
    return "SQ_STRING";
  case TokenType::root:
    return "ROOT";
  case TokenType::wild:
    return "WILD";
  default:
    return "UNDEFINED";
  }
}

std::ostream& operator<<(std::ostream& os, TokenType const& tt) {
  return os << token_type_to_string(tt);
}

bool operator==(const Token& lhs, const Token& rhs) {
  return lhs.type == rhs.type && lhs.value == rhs.value &&
         lhs.index == rhs.index && lhs.query == rhs.query;
}

std::string token_to_string(const Token& token) {
  std::ostringstream rv{};
  rv << "Token{type=" << token.type << ", value=\"" << token.value << "\""
     << ", index=" << token.index << "}";
  return rv.str();
}

std::ostream& operator<<(std::ostream& os, Token const& token) {
  return os << token_to_string(token);
}

} // namespace libjsonpath