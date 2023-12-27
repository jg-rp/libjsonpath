#ifndef LIBJSONPATH_TOKENS_H
#define LIBJSONPATH_TOKENS_H

#include <iostream>
#include <string>

namespace libjsonpath {
enum class TokenType {
  eof_,      // EOF
  and_,      // &&
  colon,     // :
  comma,     // ,
  current,   // @
  ddot,      // ..
  dq_string, // DQ_STRING
  eq,        // ==
  error,     // ERROR
  false_,    // false
  filter_,   // FILTER
  float_,    // FLOAT
  func_,     // FUNC
  ge,        // >=
  gt,        // >
  index,     // INDEX
  int_,      // INT
  lbracket,  // [
  le,        // <=
  lparen,    // (
  lt,        // <
  name_,     // NAME
  ne,        // !=
  not_,      // !
  null_,     // null
  or_,       // ||
  rbracket,  // ]
  root,      // $
  rparen,    // )
  sq_string, // SQ_STRING
  true_,     // true
  wild,      // *
};

// Return a string representation of TokenType _tt_.
std::string token_type_to_string(TokenType tt);

std::ostream& operator<<(std::ostream& os, TokenType const& tt);

struct Token {
  TokenType type{};
  std::string_view value{};
  std::string::size_type index{};
  std::string_view query{};
};

// Return a string representation of Token _t_.
std::string token_to_string(const Token& token);

bool operator==(const Token& lhs, const Token& rhs);
std::ostream& operator<<(std::ostream& os, Token const& token);
} // namespace libjsonpath

#endif