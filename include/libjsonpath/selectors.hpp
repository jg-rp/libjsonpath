#ifndef LIBJSONPATH_SELECTORS_H
#define LIBJSONPATH_SELECTORS_H

#include "libjsonpath/tokens.hpp" // Token
#include <cstdint>                // std::int64_t
#include <memory>                 // std::move std::unique_ptr
#include <optional>               // std::optional
#include <string>                 // std::string_view
#include <variant>                // std::variant
#include <vector>                 // std::vector

namespace libjsonpath {

template <typename T> class CompoundExpression {
private:
  std::unique_ptr<T> m_ptr;

public:
  CompoundExpression(T&& expr) : m_ptr(new T(std::move(expr))) {}
  CompoundExpression(const T& expr) : m_ptr(new T(expr)) {}

  CompoundExpression(const CompoundExpression& other)
      : CompoundExpression(*other.m_ptr) {}

  CompoundExpression(CompoundExpression&& other)
      : CompoundExpression(std::move(*other.m_ptr)) {}

  CompoundExpression& operator=(const CompoundExpression& other) {
    *m_ptr = *other.m_ptr;
    return *this;
  }

  CompoundExpression& operator=(CompoundExpression&& other) {
    *m_ptr = std::move(*other.m_ptr);
    return *this;
  }

  ~CompoundExpression() = default;

  T& operator*() { return *m_ptr; }
  const T& operator*() const { return *m_ptr; }
  T* operator->() { return m_ptr.get(); }
  const T* operator->() const { return m_ptr.get(); }
};

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
    FloatLiteral, StringLiteral, CompoundExpression<LogicalNotExpression>,
    CompoundExpression<InfixExpression>, CompoundExpression<RelativeQuery>,
    CompoundExpression<RootQuery>, CompoundExpression<FunctionCall>>;

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
    SliceSelector, CompoundExpression<FilterSelector>>;

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