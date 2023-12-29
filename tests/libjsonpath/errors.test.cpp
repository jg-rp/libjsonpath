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

  void expect_type_error(std::string_view query, std::string_view message) {
    EXPECT_THROW(libjsonpath::parse(query), libjsonpath::TypeError);
    try {
      libjsonpath::parse(query);
    } catch (const libjsonpath::TypeError& e) {
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

TEST_F(ErrorTest, IntLiteralWithLeadingZero) {
  expect_syntax_error("$.some[?(@.thing == 01)]",
      "integers with a leading zero are not "
      "allowed ('$.some[?(@.thing == 01)]':20)");
}

TEST_F(ErrorTest, NegativeIntLiteralWithLeadingZero) {
  expect_syntax_error("$.some[?(@.thing == -01)]",
      "integers with a leading zero are not "
      "allowed ('$.some[?(@.thing == -01)]':20)");
}

TEST_F(ErrorTest, ArrayIndexWithLeadingZero) {
  expect_syntax_error("$.foo[01]",
      "array indicies with a leading zero are not allowed ('$.foo[01]':6)");
}

TEST_F(ErrorTest, NameSelectorInvalidCharacter) {
  expect_syntax_error("$[\"\x01"
                      "\"]",
      "invalid character in string literal ('$[\"\x01"
      "\"]':3)");
}

TEST_F(ErrorTest, ResultMustBeCompared) {
  expect_syntax_error("$[?count(@..*)]",
      "result of count() must be compared ('$[?count(@..*)]':3)");
}

TEST_F(ErrorTest, ResultIsNotComparable) {
  expect_type_error("$[?match(@.a, 'a.*')==true]",
      "result of match() is not comparable ('$[?match(@.a, 'a.*')==true]':3)");
}