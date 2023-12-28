#include "libjsonpath/parse.hpp" // libjsonpath::Parser
#include "libjsonpath/jsonpath.hpp" // libjsonpath::parse libjsonpath::path_to_string
#include <gtest/gtest.h>            // EXPEXT_* TEST_F testing::Test
#include <string_view>              // string_view

class ParserTest : public testing::Test {
protected:
  void expect_to_string(std::string_view query, std::string_view want) {
    // `parse` convenience function.
    auto segments{libjsonpath::parse(query)};
    EXPECT_EQ(libjsonpath::to_string(segments), want);

    // `Parser.parse()` from a string.
    libjsonpath::Parser parser{};
    EXPECT_EQ(libjsonpath::to_string(parser.parse(query)), want);
  }
};

TEST_F(ParserTest, JustRoot) { expect_to_string("$", "$"); }

TEST_F(ParserTest, RootDotProperty) {
  expect_to_string("$.thing", "$['thing']");
}
TEST_F(ParserTest, SingleQuotedProperty) {
  expect_to_string("$['thing']", "$['thing']");
}
TEST_F(ParserTest, DoubleQuotedProperty) {
  expect_to_string("$[\"thing\"]", "$['thing']");
}

TEST_F(ParserTest, QuotedPropertyWithNonIdentChars) {
  expect_to_string("$[\"thing{!%\"]", "$['thing{!%']");
}

TEST_F(ParserTest, RootIndex) { expect_to_string("$[1]", "$[1]"); }
TEST_F(ParserTest, RootSlice) { expect_to_string("$[1:-1]", "$[1:-1:1]"); }

TEST_F(ParserTest, SliceWithStep) {
  expect_to_string("$[1:-1:2]", "$[1:-1:2]");
}
TEST_F(ParserTest, SliceWithEmptyStart) {
  expect_to_string("$[:-1]", "$[:-1:1]");
}
TEST_F(ParserTest, SliceWithEmptyStop) { expect_to_string("$[1:]", "$[1::1]"); }
TEST_F(ParserTest, RootDotWild) { expect_to_string("$.*", "$[*]"); }
TEST_F(ParserTest, RootBracketWild) { expect_to_string("$[*]", "$[*]"); }
TEST_F(ParserTest, SelectorList) { expect_to_string("$[1,2]", "$[1, 2]"); }

TEST_F(ParserTest, SelectorListWithSlice) {
  expect_to_string("$[1,5:-1:1]", "$[1, 5:-1:1]");
}

TEST_F(ParserTest, SelectorListWithSingleQuotesPropertyNames) {
  expect_to_string("$['some', 'thing']", "$['some', 'thing']");
}

TEST_F(ParserTest, SelectorListWithDoubleQuotesPropertyNames) {
  expect_to_string("$[\"some\", \"thing\"]", "$['some', 'thing']");
}

TEST_F(ParserTest, FilterWithRelativeQuery) {
  expect_to_string("$[?@.thing]", "$[?@['thing']]");
}

TEST_F(ParserTest, FilterWithRootQuery) {
  expect_to_string("$[?$.thing]", "$[?$['thing']]");
}

TEST_F(ParserTest, FilterEquals) {
  expect_to_string("$.some[?(@.thing == 7)]", "$['some'][?@['thing'] == 7]");
}

TEST_F(ParserTest, FilterGreaterThan) {
  expect_to_string("$.some[?(@.thing > 7)]", "$['some'][?@['thing'] > 7]");
}

TEST_F(ParserTest, FilterGreaterThanOrEquals) {
  expect_to_string("$.some[?(@.thing >= 7)]", "$['some'][?@['thing'] >= 7]");
}

TEST_F(ParserTest, FilterLessThanOrEquals) {
  expect_to_string("$.some[?(@.thing <= 7)]", "$['some'][?@['thing'] <= 7]");
}

TEST_F(ParserTest, FilterLessThan) {
  expect_to_string("$.some[?(@.thing < 7)]", "$['some'][?@['thing'] < 7]");
}

TEST_F(ParserTest, FilterNotEquals) {
  expect_to_string("$.some[?(@.thing != 7)]", "$['some'][?@['thing'] != 7]");
}

TEST_F(ParserTest, FilterBooleanLiterals) {
  expect_to_string("$.some[?true == false]", "$['some'][?true == false]");
}

TEST_F(ParserTest, FilterNullLiteral) {
  expect_to_string(
      "$.some[?(@.thing == null)]", "$['some'][?@['thing'] == null]");
}

TEST_F(ParserTest, FilterStringLiteral) {
  expect_to_string(
      "$.some[?(@.thing == 'foo')]", "$['some'][?@['thing'] == \"foo\"]");
}

TEST_F(ParserTest, FilterIntegerLiteral) {
  expect_to_string("$.some[?(@.thing == 1)]", "$['some'][?@['thing'] == 1]");
}

TEST_F(ParserTest, FilterIntegerLiteralZero) {
  expect_to_string("$.some[?(@.thing == 0)]", "$['some'][?@['thing'] == 0]");
}

TEST_F(ParserTest, FilterIntegerLiteralNegativeZero) {
  expect_to_string("$.some[?(@.thing == -0)]", "$['some'][?@['thing'] == 0]");
}

TEST_F(ParserTest, FilterFloatLiteral) {
  expect_to_string(
      "$.some[?(@.thing == 1.1)]", "$['some'][?@['thing'] == 1.1]");
}

TEST_F(ParserTest, FilterFloatLiteralWithLeadingZero) {
  expect_to_string(
      "$.some[?(@.thing == 0.1)]", "$['some'][?@['thing'] == 0.1]");
}

TEST_F(ParserTest, FilterFloatLiteralWithLeadingNegativeZero) {
  expect_to_string(
      "$.some[?(@.thing == -0.1)]", "$['some'][?@['thing'] == -0.1]");
}

TEST_F(ParserTest, FilterLogicalNot) {
  expect_to_string("$.some[?(!@.thing)]", "$['some'][?!@['thing']]");
}

TEST_F(ParserTest, FilterLogicalAnd) {
  expect_to_string(
      "$.some[?@.thing && @.other]", "$['some'][?(@['thing'] && @['other'])]");
}

TEST_F(ParserTest, FilterLogicalOr) {
  expect_to_string(
      "$.some[?@.thing || @.other]", "$['some'][?(@['thing'] || @['other'])]");
}

TEST_F(ParserTest, FilterGroupedExpression) {
  expect_to_string("$.some[?(@.thing > 1 && ($.foo || $.bar))]",
      "$['some'][?(@['thing'] > 1 && ($['foo'] || $['bar']))]");
}

TEST_F(ParserTest, SingleQuotedStringLiteralWithEscape) {
  expect_to_string("$[?@.foo == 'ba\\'r']", "$[?@['foo'] == \"ba'r\"]");
}

TEST_F(ParserTest, DoubleQuotedStringLiteralWithEscape) {
  expect_to_string("$[?@.foo == \"ba\\\"r\"]", "$[?@[\'foo\'] == \"ba\"r\"]");
}

TEST_F(ParserTest, NotBindsMoreTightlyThanAnd) {
  expect_to_string("$[?!@.a && !@.b]", "$[?(!@['a'] && !@['b'])]");
}

TEST_F(ParserTest, NotBindsMoreTightlyThanOr) {
  expect_to_string("$[?!@.a || !@.b]", "$[?(!@['a'] || !@['b'])]");
}

TEST_F(ParserTest, ControlPrecedenceWithParens) {
  expect_to_string("$[?!(@.a && !@.b)]", "$[?!(@['a'] && !@['b'])]");
}

TEST_F(ParserTest, DoubleQuotedEscapedNameSelector) {
  expect_to_string("$[\"\\u263A\"]", "$['☺']");
}

TEST_F(ParserTest, DoubleQuotedSurrogatePairNameSelector) {
  expect_to_string("$[\"\\uD834\\uDD1E\"]", "$['𝄞']");
}

TEST_F(ParserTest, RecursiveIndex) { expect_to_string("$..[1]", "$..[1]"); }

TEST_F(ParserTest, FilterFunctionComparison) {
  expect_to_string("$[?count(@..*)>2]", "$[?count(@..[*]) > 2]");
}

TEST_F(ParserTest, IntegerLiteralWithExponent) {
  expect_to_string("$[?@.a==1e2]", "$[?@['a'] == 100]");
}

TEST_F(ParserTest, IntegerLiteralWithPositiveExponent) {
  expect_to_string("$[?@.a==1e+2]", "$[?@['a'] == 100]");
}

TEST_F(ParserTest, IntegerLiteralWithNegativeExponent) {
  expect_to_string("$[?@.a==1e-2]", "$[?@['a'] == 0.01]");
}
