#include "libjsonpath/lex.h"
#include "libjsonpath/tokens.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using tt = libjsonpath::TokenType;

class LexerTest : public testing::Test {
protected:
  void expect_tokens(
      std::string_view query, const std::vector<libjsonpath::Token>& want) {
    libjsonpath::Lexer lexer{query};
    EXPECT_EQ(lexer.query_, query);
    EXPECT_EQ(lexer.tokens().size(), 0);
    lexer.run();

    ASSERT_EQ(lexer.tokens().size(), std::size(want))
        << "tokens size mismatch, expected " << std::size(want) << ", got"
        << lexer.tokens().size();

    auto mismatch{std::mismatch(
        lexer.tokens().cbegin(), lexer.tokens().cend(), want.cbegin())};

    if (std::get<0>(mismatch) != lexer.tokens().cend()) {
      FAIL() << "expected " << *std::get<1>(mismatch) << ", found "
             << *std::get<0>(mismatch);
    }
  }
};

TEST_F(LexerTest, BasicShorthandName) {
  expect_tokens("$.foo.bar", {
                                 {tt::root, "$", 0},
                                 {tt::name, "foo", 2},
                                 {tt::name, "bar", 6},
                                 {tt::eof_, "", 9},
                             });
}

TEST_F(LexerTest, BracketedName) {
  expect_tokens("$['foo']['bar']", {
                                       {tt::root, "$", 0},
                                       {tt::lbracket, "[", 1},
                                       {tt::sq_string, "foo", 3},
                                       {tt::rbracket, "]", 7},
                                       {tt::lbracket, "[", 8},
                                       {tt::sq_string, "bar", 10},
                                       {tt::rbracket, "]", 14},
                                       {tt::eof_, "", 15},
                                   });
}

TEST_F(LexerTest, BasicIndex) {
  expect_tokens("$.foo[1]", {
                                {tt::root, "$", 0},
                                {tt::name, "foo", 2},
                                {tt::lbracket, "[", 5},
                                {tt::index, "1", 6},
                                {tt::rbracket, "]", 7},
                                {tt::eof_, "", 8},
                            });
}

TEST_F(LexerTest, MissingRootSelector) {
  expect_tokens("foo.bar", {
                               {tt::error, "expected '$', found 'f'", 0},
                           });
}
