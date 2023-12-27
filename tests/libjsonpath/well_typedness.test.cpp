#include "libjsonpath/exceptions.hpp" // libjsonpath::TypeError
#include "libjsonpath/jsonpath.hpp" // libjsonpath::parse libjsonpath::to_string
#include <gtest/gtest.h>            // EXPEXT_* TEST_F testing::Test
#include <string>                   // std::string
#include <string_view>              // std::string_view

// TODO: define mock function extensions to test.

class WellTypedTest : public testing::Test {
protected:
  void expect_type_error(std::string_view query, std::string_view message) {
    EXPECT_THROW(libjsonpath::parse(query), libjsonpath::TypeError);
    try {
      libjsonpath::parse(query);
    } catch (const libjsonpath::TypeError& e) {
      EXPECT_EQ(std::string{e.what()}, message);
    }
  }

  void expect_to_string(std::string_view query, std::string_view want) {
    auto segments{libjsonpath::parse(query)};
    EXPECT_EQ(libjsonpath::to_string(segments), want);
  }
};

TEST_F(WellTypedTest, LengthSingularQuery) {
  expect_to_string("$[?length(@) < 3]", "$[?length(@) < 3]");
}

TEST_F(WellTypedTest, LengthNonSingularQuery) {
  expect_type_error("$[?length(@.*) < 3]",
      "length() argument 0 must be of ValueType ('$[?length(@.*) < 3]':3)");
}

TEST_F(WellTypedTest, CountIntArg) {
  expect_type_error("$[?count(1) == 1]",
      "count() argument 0 must be of NodesType ('$[?count(1) == 1]':3)");
}
