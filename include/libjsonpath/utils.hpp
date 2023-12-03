#ifndef LIBJSONPATH_UTILS_H
#define LIBJSONPATH_UTILS_H

#include "libjsonpath/selectors.hpp"

namespace libjsonpath {

// A _segment_t_ visitor that returns _true_ if the segment is a singular
// query, or _false_ otherwise.
struct SingularSegmentVisitor {
  bool operator()(const Segment& segment) const;
  bool operator()(const RecursiveSegment& segment) const;
};

// Return true if the JSONPath represented by _segments_ is a singular
// query, as defined by the JSONPath spec.
bool singular_query(const segments_t& segments);

} // namespace libjsonpath

#endif // LIBJSONPATH_UTILS_H