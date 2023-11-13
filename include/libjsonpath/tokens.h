#ifndef LIBJSONPATH_TOKENS_H_
#define LIBJSONPATH_TOKENS_H_

#include <iostream>
#include <string>

namespace libjsonpath {
enum class TokenType {
  and_,      // &&
  colon,     // :
  comma,     // ,
  current,   // @
  ddot,      // ..
  dq_string, // DQ_STRING
  eof_,      // EOF
  eq,        // ==
  error,     // ERROR
  false_,    // false
  filter,    // FILTER
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
  name,      // NAME
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

std::ostream& operator<<(std::ostream& os, TokenType const& token_type);

struct Token {
  TokenType type;
  std::string_view value;
  std::string::size_type index;
};

bool operator==(const Token& lhs, const Token& rhs);
std::ostream& operator<<(std::ostream& os, Token const& token);
} // namespace libjsonpath

#endif