#include "libjsonpath/exceptions.hpp" // libjsonpath::SyntaxError
#include "libjsonpath/jsonpath.hpp"   // libjsonpath::parse
#include <gtest/gtest.h>              // EXPEXT_* TEST_F testing::Test
#include <string>                     // std::string
#include <string_view>                // std::string_view

class ErrorTest : public testing::Test {
protected:
  void expect_syntax_error(std::string_view query, std::string_view message) {
    EXPECT_THROW(libjsonpath::parse(query), libjsonpath::SyntaxError);
    try {
      libjsonpath::parse(query);
    } catch (const libjsonpath::SyntaxError& e) {
      EXPECT_EQ(std::string{e.what()}, message);
    }
  }
};

TEST_F(ErrorTest, LeadingWhitespace) {
  expect_syntax_error("  $.foo", "expected '$', found ' ' ('  $.foo':0)");
}

TEST_F(ErrorTest, ShorthandIndex) {
  expect_syntax_error("$.1", "unexpected shorthand selector '1' ('$.1':2)");
}

TEST_F(ErrorTest, ShorthandSymbol) {
  expect_syntax_error("$.&", "unexpected shorthand selector '&' ('$.&':2)");
}

TEST_F(ErrorTest, EmptyBracketedSegment) {
  expect_syntax_error("$.foo[]", "empty bracketed segment ('$.foo[]':5)");
}

TEST_F(ErrorTest, NonSingularQueryInComparison) {
  expect_syntax_error(
      "$[?@[*]==0]", "non-singular query is not comparable ('$[?@[*]==0]':3)");
}