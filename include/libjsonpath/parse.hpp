#ifndef LIBJSONPATH_PARSE_H_
#define LIBJSONPATH_PARSE_H_

#include "libjsonpath/selectors.hpp"
#include "libjsonpath/tokens.hpp"
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace libjsonpath {

using Tokens = std::deque<Token>;
using TokenIterator = std::deque<Token>::const_iterator;

// JSONPath filter expression operator precedence. These constants are passed
// to `parse_filter_expression()` when parsing prefix and infix expressions.
constexpr int PRECEDENCE_LOWEST = 1;
constexpr int PRECEDENCE_LOGICAL_AND = 4;
constexpr int PRECEDENCE_LOGICAL_OR = 5;
constexpr int PRECEDENCE_COMPARISON = 6;
constexpr int PRECEDENCE_PREFIX = 7;

// A mapping of token types to their precedence in a filter expression.
const std::unordered_map<TokenType, int> PRECEDENCES{
    {TokenType::and_, PRECEDENCE_LOGICAL_AND},
    {TokenType::eq, PRECEDENCE_COMPARISON},
    {TokenType::ge, PRECEDENCE_COMPARISON},
    {TokenType::gt, PRECEDENCE_COMPARISON},
    {TokenType::le, PRECEDENCE_COMPARISON},
    {TokenType::lt, PRECEDENCE_COMPARISON},
    {TokenType::ne, PRECEDENCE_COMPARISON},
    {TokenType::not_, PRECEDENCE_PREFIX},
    {TokenType::or_, PRECEDENCE_LOGICAL_OR},
    {TokenType::rparen, PRECEDENCE_LOWEST},
};

// A mapping of token types to their equivalent binary operator used when
// building an infix expression.
const std::unordered_map<TokenType, BinaryOperator> BINARY_OPERATORS{
    {TokenType::and_, BinaryOperator::logical_and},
    {TokenType::eq, BinaryOperator::eq},
    {TokenType::ge, BinaryOperator::ge},
    {TokenType::gt, BinaryOperator::gt},
    {TokenType::le, BinaryOperator::le},
    {TokenType::lt, BinaryOperator::lt},
    {TokenType::ne, BinaryOperator::ne},
    {TokenType::or_, BinaryOperator::logical_or},
};

// The JSONPath quey expression parser.
//
// An instance of _libjsonpath::Parser_ does not maintain any state, so
// repeated calls to _Parser.parse()_ are OK and, in fact, encouraged.
class Parser {
public:
  // Parse tokens from _tokens_ and return a sequence of segments making up
  // the JSONPath.
  segments_t parse(const Tokens& tokens) const;

protected:
  segments_t parse_path(TokenIterator& tokens, bool in_filter) const;
  segment_t parse_segment(TokenIterator& tokens) const;

  std::vector<selector_t> parse_bracketed_selection(
      TokenIterator& tokens) const;

  std::unique_ptr<FilterSelector> parse_filter_selector(
      TokenIterator& tokens) const;

  SliceSelector parse_slice_selector(TokenIterator& tokens) const;

  NullLiteral parse_null_literal(TokenIterator& tokens) const;
  BooleanLiteral parse_boolean_literal(TokenIterator& tokens) const;
  StringLiteral parse_string_literal(TokenIterator& tokens) const;
  IntegerLiteral parse_integer_literal(TokenIterator& tokens) const;
  FloatLiteral parse_float_literal(TokenIterator& tokens) const;

  std::unique_ptr<LogicalNotExpression> parse_logical_not(
      TokenIterator& tokens) const;

  std::unique_ptr<InfixExpression> parse_infix(
      TokenIterator& tokens, expression_t left) const;

  std::unique_ptr<RootQuery> parse_root_query(TokenIterator& tokens) const;

  std::unique_ptr<RelativeQuery> parse_relative_query(
      TokenIterator& tokens) const;

  std::unique_ptr<FunctionCall> parse_function_call(
      TokenIterator& tokens) const;

  expression_t parse_filter_token(TokenIterator& tokens) const;
  expression_t parse_grouped_expression(TokenIterator& tokens) const;

  expression_t parse_filter_expression(
      TokenIterator& tokens, int precedence) const;

  // Assert that the current token in _it_ has a type matching _tt_.
  void expect(TokenIterator it, TokenType tt) const;

  // Assert that the next token in _it_ has a type matching _tt_.
  void expect_peek(TokenIterator it, TokenType tt) const;

  // Return the precedence given a token type. We use the PRECEDENCES map and
  // fall back to PRECEDENCE_LOWEST if the map does not contain the token type.
  int get_precedence(TokenType tt) const noexcept;

  // Return the binary operator for the given token type or raise an exception
  // if the token type does not represent a binary operator.
  BinaryOperator get_binary_operator(TokenType tt) const;

  // Decode unicode escape sequences and, when given a single quoted string
  // token, normalize escaped quotes within the string to be suitable for
  // output as a double quoted string.
  std::string decode_string_token(const Token& t) const;

private:
  // Convert a string view to an integer. It is assumed that the view is
  // composed of digits with the possibility of a leading minus sign, as
  // one would get from a _Token_ of type _index_.
  std::int64_t svtoi(std::string_view sv) const;

  // Convert a string view to a double. It is assumed that the view contains a
  // valid string representation of a float, as we'd get from a `Token` of type
  // `float_`.
  double svtod(std::string_view sv) const;

  // Replace all occurrences of _substring_ with _replacement_ in _original_.
  void replaceAll(std::string& original, std::string_view substring,
      std::string_view replacement) const;
};

} // namespace libjsonpath

#endif