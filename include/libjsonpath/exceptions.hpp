#ifndef LIBJSONPATH_EXCEPTIONS_H
#define LIBJSONPATH_EXCEPTIONS_H
#include "libjsonpath/tokens.hpp"
#include <algorithm>   // std::count
#include <exception>   // std::exception
#include <iterator>    // std::advance
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view

namespace libjsonpath {

static std::string format_exception(
    std::string_view message, const Token& token) {
  std::ostringstream rv{};
  rv << message << " ('" << token.query << "':" << token.index << ")";
  return rv.str();
}

// Base class for all exceptions thrown from libjsonpath.
class Exception : public std::exception {
public:
  Exception(std::string_view message, const Token& token)
      : m_message{format_exception(message, token)}, m_token{token} {};

  const char* what() const noexcept override { return m_message.c_str(); };
  const Token& token() const noexcept { return m_token; };

private:
  std::string m_message{};
  Token m_token{};

  std::string::difference_type line_number() {
    auto it{m_token.query.cbegin()};
    std::advance(it, m_token.index);
    return std::count(m_token.query.cbegin(), it, '\n') + 1;
  };
};

// An exception thrown due to an error during JSONPath tokenization.
//
// A LexerError indicates a bug in the Lexer class. During normal operation,
// invalid syntax found when scanning a JSONPath query string will leave an
// error token in the resulting _tokens_ collection.
class LexerError : public Exception {
public:
  LexerError(std::string_view message, const Token& token)
      : Exception{message, token} {};
};

// An exception thrown due to invalid JSONPath query syntax.
class SyntaxError : public Exception {
public:
  SyntaxError(std::string_view message, const Token& token)
      : Exception{message, token} {};
};
} // namespace libjsonpath

#endif // LIBJSONPATH_EXCEPTIONS_H
