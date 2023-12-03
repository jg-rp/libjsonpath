#include "libjsonpath/utils.hpp"
#include <variant> // std::visit std::holds_alternative
#include <vector>  // std::vector

namespace libjsonpath {

bool SingularSegmentVisitor::operator()(const Segment& segment) const {
  return segment.selectors.size() == 1 &&
         (std::holds_alternative<NameSelector>(segment.selectors.front()) ||
             std::holds_alternative<IndexSelector>(segment.selectors.front()));
}

bool SingularSegmentVisitor::operator()(const RecursiveSegment&) const {
  return false;
}

bool singular_query(const segments_t& segments) {
  for (const auto& segment : segments) {
    if (!std::visit(SingularSegmentVisitor(), segment)) {
      return false;
    }
  }
  return true;
}
} // namespace libjsonpath