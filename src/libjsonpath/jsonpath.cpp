#include "libjsonpath/jsonpath.hpp"
#include "libjsonpath/exceptions.hpp" // libjsonpath::SyntaxError
#include "libjsonpath/lex.hpp"        // libjsonpath::Lexer
#include "libjsonpath/parse.hpp"      // libjsonpath::Parser
#include "libjsonpath/tokens.hpp"     // libjsonpath::TokenType
#include <string>                     // std::string
#include <variant>                    // std::visit

namespace libjsonpath {

using namespace std::string_literals;

segments_t parse(std::string_view s) {
  Lexer lexer{s};
  lexer.run();
  auto tokens{lexer.tokens()};
  if (tokens.size() && tokens.back().type == TokenType::error) {
    throw SyntaxError(tokens.back().value, tokens.back());
  }
  Parser parser{};
  return parser.parse(tokens);
}

segments_t parse(std::string_view s,
    std::unordered_map<std::string, FunctionExtensionTypes> function_extensions) {
  Lexer lexer{s};
  lexer.run();
  auto tokens{lexer.tokens()};
  if (tokens.size() && tokens.back().type == TokenType::error) {
    throw SyntaxError(tokens.back().value, tokens.back());
  }
  Parser parser{function_extensions};
  return parser.parse(tokens);
}

std::string to_string(const segments_t& path) {
  std::string rv{"$"};
  for (const auto& segment : path) {
    rv += std::visit(SegmentToStringVisitor(), segment);
  }
  return rv;
}

std::string SelectorToStringVisitor::operator()(
    const NameSelector& selector) const {
  return "'"s + selector.name + "'"s;
}

std::string SelectorToStringVisitor::operator()(
    const IndexSelector& selector) const {
  return std::to_string(selector.index);
}

std::string SelectorToStringVisitor::operator()(const WildSelector&) const {
  return "*";
}

std::string SelectorToStringVisitor::operator()(
    const SliceSelector& selector) const {
  return (selector.start ? std::to_string(selector.start.value()) : ""s) + ":" +
         (selector.stop ? std::to_string(selector.stop.value()) : ""s) + ":" +
         (selector.step ? std::to_string(selector.step.value()) : "1"s);
}

std::string SelectorToStringVisitor::operator()(
    const Box<FilterSelector>& selector) const {
  return "?" + std::visit(ExpressionToStringVisitor(), selector->expression);
}

std::string SegmentToStringVisitor::operator()(const Segment& segment) const {
  std::string rv{"["};
  for (const auto& selector : segment.selectors) {
    rv += std::visit(SelectorToStringVisitor(), selector);
    rv += ", ";
  }
  rv.erase(rv.end() - 2, rv.end()); // remove the trailing comma and space
  rv += "]";
  return rv;
}

std::string SegmentToStringVisitor::operator()(
    const RecursiveSegment& segment) const {
  std::string rv{"..["};
  for (const auto& selector : segment.selectors) {
    rv += std::visit(SelectorToStringVisitor(), selector);
    rv += ", ";
  }
  rv.erase(rv.end() - 2, rv.end()); // remove the trailing comma and space
  rv += "]";
  return rv;
}

std::string ExpressionToStringVisitor::operator()(const NullLiteral&) const {
  return "null";
}

std::string ExpressionToStringVisitor::operator()(
    const BooleanLiteral& expression) const {
  return expression.value ? "true" : "false";
}

std::string ExpressionToStringVisitor::operator()(
    const IntegerLiteral& expression) const {
  return std::to_string(expression.value);
}

std::string ExpressionToStringVisitor::operator()(
    const FloatLiteral& expression) const {
  // NOTE: std::to_string adds trailing zeros
  std::string rv = std::to_string(expression.value);
  rv.erase(rv.find_last_not_of('0') + 1, std::string::npos);
  rv.erase(rv.find_last_not_of('.') + 1, std::string::npos);
  return rv;
}

std::string ExpressionToStringVisitor::operator()(
    const StringLiteral& expression) const {
  return "\""s + expression.value + "\"";
}

std::string ExpressionToStringVisitor::operator()(
    const Box<LogicalNotExpression>& expression) const {
  return "!" + std::visit(ExpressionToStringVisitor(), expression->right);
}

static std::string binary_operator_to_string(BinaryOperator op) {
  switch (op) {
  case BinaryOperator::logical_and:
    return "&&";
  case BinaryOperator::logical_or:
    return "||";
  case BinaryOperator::eq:
    return "==";
  case BinaryOperator::ge:
    return ">=";
  case BinaryOperator::gt:
    return ">";
  case BinaryOperator::le:
    return "<=";
  case BinaryOperator::lt:
    return "<";
  case BinaryOperator::ne:
    return "!=";
  default:
    return "OPERATOR ERROR";
  }
}

std::string ExpressionToStringVisitor::operator()(
    const Box<InfixExpression>& expression) const {
  if (expression->op == BinaryOperator::logical_and ||
      expression->op == BinaryOperator::logical_or) {
    return "("s + std::visit(ExpressionToStringVisitor(), expression->left) +
           " " + binary_operator_to_string(expression->op) + " " +
           std::visit(ExpressionToStringVisitor(), expression->right) + ")"s;
  }
  return std::visit(ExpressionToStringVisitor(), expression->left) + " " +
         binary_operator_to_string(expression->op) + " " +
         std::visit(ExpressionToStringVisitor(), expression->right);
}

std::string ExpressionToStringVisitor::operator()(
    const Box<RelativeQuery>& expression) const {
  auto path_string{to_string(expression->query)};
  path_string[0] = '@';
  return path_string;
}

std::string ExpressionToStringVisitor::operator()(
    const Box<RootQuery>& expression) const {
  return to_string(expression->query);
}

std::string ExpressionToStringVisitor::operator()(
    const Box<FunctionCall>& expression) const {
  std::string rv{expression->name};
  rv.push_back('(');
  for (const auto& arg : expression->args) {
    rv.append(std::visit(ExpressionToStringVisitor(), arg));
    rv.append(", ");
  }
  rv.erase(rv.end() - 2, rv.end()); // remove the trailing comma and space
  rv.push_back(')');
  return rv;
}

} // namespace libjsonpath