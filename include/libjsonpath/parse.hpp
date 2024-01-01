#ifndef LIBJSONPATH_PARSE_H
#define LIBJSONPATH_PARSE_H

#include "libjsonpath/selectors.hpp"
#include "libjsonpath/tokens.hpp"
#include <deque>         // std::deque
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set

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

// Possible types that a JSONPath function extension can accept as
// arguments or return as its result.
enum class ExpressionType {
  value,
  logical,
  nodes,
};

// The argument and result types for a JSONPath function extension.
struct FunctionExtensionTypes {
  std::vector<ExpressionType> args;
  ExpressionType res;
};

const std::unordered_map<std::string, FunctionExtensionTypes>
    DEFAULT_FUNCTION_EXTENSIONS{
        {"count", {{ExpressionType::nodes}, ExpressionType::value}},
        {"length", {{ExpressionType::value}, ExpressionType::value}},
        {"match", {{ExpressionType::value, ExpressionType::value},
                      ExpressionType::logical}},
        {"search", {{ExpressionType::value, ExpressionType::value},
                       ExpressionType::logical}},
        {"value", {{ExpressionType::nodes}, ExpressionType::value}},
    };

// The JSONPath query expression parser.
//
// An instance of _libjsonpath::Parser_ does not maintain any state, so
// repeated calls to _Parser.parse()_ are OK and, in fact, encouraged.
class Parser {
public:
  Parser() : m_function_extensions{DEFAULT_FUNCTION_EXTENSIONS} {};
  Parser(std::unordered_map<std::string, FunctionExtensionTypes>
          function_extensions)
      : m_function_extensions{function_extensions} {}

  // Parse tokens from _tokens_ and return a sequence of segments making up
  // the JSONPath.
  segments_t parse(const Tokens& tokens) const;
  segments_t parse(std::string_view s) const;

protected:
  std::unordered_map<std::string, FunctionExtensionTypes> m_function_extensions;
  segments_t parse_path(TokenIterator& tokens) const;
  segments_t parse_filter_path(TokenIterator& tokens) const;
  segment_t parse_segment(TokenIterator& tokens) const;

  std::vector<selector_t> parse_bracketed_selection(
      TokenIterator& tokens) const;

  FilterSelector parse_filter_selector(TokenIterator& tokens) const;
  SliceSelector parse_slice_selector(TokenIterator& tokens) const;

  NullLiteral parse_null_literal(TokenIterator& tokens) const;
  BooleanLiteral parse_boolean_literal(TokenIterator& tokens) const;
  StringLiteral parse_string_literal(TokenIterator& tokens) const;
  IntegerLiteral parse_integer_literal(TokenIterator& tokens) const;
  FloatLiteral parse_float_literal(TokenIterator& tokens) const;

  expression_t parse_logical_not(TokenIterator& tokens) const;
  expression_t parse_infix(TokenIterator& tokens, expression_t left) const;
  expression_t parse_root_query(TokenIterator& tokens) const;
  expression_t parse_relative_query(TokenIterator& tokens) const;
  expression_t parse_function_call(TokenIterator& tokens) const;
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
  BinaryOperator get_binary_operator(const Token& t) const;

  // Decode unicode escape sequences and, when given a single quoted string
  // token, normalize escaped quotes within the string to be suitable for
  // output as a double quoted string.
  std::string decode_string_token(const Token& t) const;

  // Throw a SyntaxError if _expr_ is not a singular query, or a TypeError
  // if _expr_ is a function returning a non ValueType result.
  void throw_for_non_comparable(const expression_t& expr) const;

private:
  // Convert a Token's value to an int. It is assumed that the view is
  // composed of digits with the possibility of a leading minus sign, as
  // one would get from a _Token_ of type _index_.
  std::int64_t token_to_int(const Token& t) const;

  // Convert a Token's value to a double. It is assumed that the view contains
  // a valid string representation of a float, as we'd get from a `Token` of
  // type `float_`.
  double token_to_double(const Token& t) const;

  // Replace all occurrences of _substring_ with _replacement_ in _original_.
  void replaceAll(std::string& original, std::string_view substring,
      std::string_view replacement) const;

  // Return a copy of _sv_ with all `\uXXXX` escape sequences replaced with
  // their equivalent UTF-8 encoded bytes.
  std::string unescape_json_string(
      std::string_view sv, const Token& token) const;

  // Return the unicode code point _code_point_ encoded in UTF-8.
  std::string encode_utf8(std::int32_t code_point, const Token& token) const;

  // Return the result type for the function extension named _name_.
  // Throws a TypeError if _name_ does not exist in function_extensions.
  ExpressionType function_result_type(std::string name, const Token& t) const;

  // Throw a Syntax error if _args_ are not valid for the function extension
  // named by token _t_.
  void throw_for_function_signature(
      const Token& t, const std::vector<expression_t>& args) const;
};

} // namespace libjsonpath

#endif
