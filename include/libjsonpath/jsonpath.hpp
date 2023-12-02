#ifndef LIBJSONPATH_JSONPATH_H
#define LIBJSONPATH_JSONPATH_H

#include "libjsonpath/selectors.hpp"
#include <string>
#include <string_view>

namespace libjsonpath {

// Return a sequence of JSONPath segments. If a segment contains a filter
// selector, the selector's _expression_ member will effectively be the root
// of a parse tree for the filter expression.
//
// See libjsonpath::selectors.hpp for segment, selector and filter expression
// node definitions.
segments_t parse(std::string_view s);

// Return a canonical string representation of a sequence of JSONPath segments.
std::string to_string(const segments_t& path);

// A _selector_t_ visitor returning a string representation of the the selector
// held by the variant. Each _operator()_ overload returns the canonical
// representation of the selector, replacing shorthand selectors with their
// bracketed equivalents.
struct SelectorToStringVisitor {
  std::string operator()(const NameSelector& selector) const;
  std::string operator()(const IndexSelector& selector) const;
  std::string operator()(const WildSelector&) const;
  std::string operator()(const SliceSelector& selector) const;
  std::string operator()(const std::unique_ptr<FilterSelector>& selector) const;
};

// A _segments_t_ visitor returning a string representation of each segment in
// the sequence. Each _operator()_ overload delegates to the selector string
// visitor, returning the canonical representation of the segment.
struct SegmentToStringVisitor {
  std::string operator()(const Segment& segment) const;
  std::string operator()(const RecursiveSegment& segment) const;
};

// An _expression_t_ visitor returning a string representation of all nodes in
// the filter expression parse tree.
struct ExpressionToStringVisitor {
  std::string operator()(const NullLiteral& expression) const;
  std::string operator()(const BooleanLiteral& expression) const;
  std::string operator()(const IntegerLiteral& expression) const;
  std::string operator()(const FloatLiteral& expression) const;
  std::string operator()(const StringLiteral& expression) const;

  std::string operator()(
      const std::unique_ptr<LogicalNotExpression>& expression) const;

  std::string operator()(
      const std::unique_ptr<InfixExpression>& expression) const;

  std::string operator()(
      const std::unique_ptr<RelativeQuery>& expression) const;

  std::string operator()(const std::unique_ptr<RootQuery>& expression) const;
  std::string operator()(const std::unique_ptr<FunctionCall>& expression) const;
};

} // namespace libjsonpath

#endif