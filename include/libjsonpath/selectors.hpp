#ifndef LIBJSONPATH_SELECTORS_H_
#define LIBJSONPATH_SELECTORS_H_

#include "libjsonpath/tokens.hpp" // Token
#include <cstdint>                // int64_t
#include <format>                 // format
#include <memory>                 // unique_ptr
#include <optional>               // optional
#include <string>                 // string_view
#include <variant>                // variant
#include <vector>                 // vector

namespace libjsonpath {

enum class BinaryOperator {
  none,
  logical_and,
  logical_or,
  eq,
  ge,
  gt,
  le,
  lt,
  ne,
};

struct Segment; // forward declaration
struct RecursiveSegment;

using segments_t = std::vector<std::variant<Segment, RecursiveSegment>>;

struct NullLiteral; // forward declaration
struct BooleanLiteral;
struct IntegerLiteral;
struct FloatLiteral;
struct StringLiteral;
struct LogicalNotExpression;
struct InfixExpression;
struct RelativeQuery;
struct RootQuery;
struct FunctionCall;

using expression_t = std::variant<NullLiteral, BooleanLiteral, IntegerLiteral,
    FloatLiteral, StringLiteral, std::unique_ptr<LogicalNotExpression>,
    std::unique_ptr<InfixExpression>, std::unique_ptr<RelativeQuery>,
    std::unique_ptr<RootQuery>, std::unique_ptr<FunctionCall>>;

struct NullLiteral {
  Token token{};
};

struct BooleanLiteral {
  Token token{};
  bool value{};
};

struct IntegerLiteral {
  Token token{};
  std::int64_t value{};
};

struct FloatLiteral {
  Token token{};
  double value{};
};

struct StringLiteral {
  Token token{};
  std::string value{};
};

struct LogicalNotExpression {
  Token token{};
  expression_t right{};
};

struct InfixExpression {
  Token token{};
  expression_t left{};
  BinaryOperator op{};
  expression_t right{};
};

struct RelativeQuery {
  Token token{};
  segments_t query{};
};

struct RootQuery {
  Token token{};
  segments_t query{};
};

struct FunctionCall {
  Token token{};
  std::string_view name{};
  std::vector<expression_t> args{};
};

struct NameSelector {
  Token token{};
  std::string name{};
  bool shorthand{false};
};

struct IndexSelector {
  Token token{};
  std::int64_t index{};
};

struct WildSelector {
  Token token{};
  bool shorthand{};
};

struct SliceSelector {
  Token token{};
  std::optional<std::int64_t> start{};
  std::optional<std::int64_t> stop{};
  std::optional<std::int64_t> step{};
};

struct FilterSelector {
  Token token{};
  expression_t expression{};
};

using selector_t = std::variant<NameSelector, IndexSelector, WildSelector,
    SliceSelector, std::unique_ptr<FilterSelector>>;

struct Segment {
  Token token;
  std::vector<selector_t> selectors{};
};

struct RecursiveSegment {
  Token token;
  std::vector<selector_t> selectors{};
};

using segment_t = std::variant<std::monostate, Segment, RecursiveSegment>;

} // namespace libjsonpath

#endif