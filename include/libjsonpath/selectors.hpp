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

// A wrapper around std::unique_ptr that helps us manage our recursive data
// structure. Thanks go to a post by Jonathan for explaining this approach.
// See https://www.foonathan.net/2022/05/recursive-variant-box/.
template <typename T> class Box {
private:
  std::unique_ptr<T> m_ptr;

public:
  Box(T&& expr) : m_ptr(new T(std::move(expr))) {}
  Box(const T& expr) : m_ptr(new T(expr)) {}

  Box(const Box& other) : Box(*other.m_ptr) {}

  Box(Box&& other) : Box(std::move(*other.m_ptr)) {}

  Box& operator=(const Box& other) {
    *m_ptr = *other.m_ptr;
    return *this;
  }

  Box& operator=(Box&& other) {
    *m_ptr = std::move(*other.m_ptr);
    return *this;
  }

  ~Box() = default;

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

using expression_t =
    std::variant<NullLiteral, BooleanLiteral, IntegerLiteral, FloatLiteral,
        StringLiteral, Box<LogicalNotExpression>, Box<InfixExpression>,
        Box<RelativeQuery>, Box<RootQuery>, Box<FunctionCall>>;

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
    SliceSelector, Box<FilterSelector>>;

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

#endif // LIBJSONPATH_SELECTORS_H