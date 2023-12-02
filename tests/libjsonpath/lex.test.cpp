#include "libjsonpath/lex.hpp"    // libjsonpath::Lexer
#include "libjsonpath/tokens.hpp" // libjsonpath::Token
#include <algorithm>              // std::mismatch
#include <gtest/gtest.h>          // EXPEXT_* TEST_F testing::Test
#include <string>                 // std::string_view
#include <vector>                 // std::vector

using tt = libjsonpath::TokenType;

class LexerTest : public testing::Test {
protected:
  void expect_tokens(
      std::string_view query, const std::vector<libjsonpath::Token>& want) {
    libjsonpath::Lexer lexer{query};
    EXPECT_EQ(lexer.query, query);
    EXPECT_EQ(lexer.tokens().size(), 0);

    lexer.run();
    auto tokens{lexer.tokens()};

    EXPECT_EQ(std::size(lexer.tokens()), std::size(want));

    if (std::size(tokens) < std::size(want)) {
      auto mismatch{std::mismatch(want.cbegin(), want.cend(), tokens.cbegin())};

      if (std::get<0>(mismatch) != want.cend()) {
        FAIL() << query << std::endl
               << "expected: " << *std::get<0>(mismatch) << std::endl
               << "found:    " << *std::get<1>(mismatch);
      }
    } else {
      auto mismatch{
          std::mismatch(tokens.cbegin(), tokens.cend(), want.cbegin())};

      if (std::get<0>(mismatch) != tokens.cend()) {
        FAIL() << query << std::endl
               << "expected: " << *std::get<1>(mismatch) << std::endl
               << "found:    " << *std::get<0>(mismatch);
      }
    }
  }
};

TEST_F(LexerTest, BasicShorthandName) {
  expect_tokens("$.foo.bar", {
                                 {tt::root, "$", 0, "$.foo.bar"},
                                 {tt::name, "foo", 2, "$.foo.bar"},
                                 {tt::name, "bar", 6, "$.foo.bar"},
                                 {tt::eof_, "", 9, "$.foo.bar"},
                             });
}

TEST_F(LexerTest, BracketedName) {
  expect_tokens(
      "$['foo']['bar']", {
                             {tt::root, "$", 0, "$['foo']['bar']"},
                             {tt::lbracket, "[", 1, "$['foo']['bar']"},
                             {tt::sq_string, "foo", 3, "$['foo']['bar']"},
                             {tt::rbracket, "]", 7, "$['foo']['bar']"},
                             {tt::lbracket, "[", 8, "$['foo']['bar']"},
                             {tt::sq_string, "bar", 10, "$['foo']['bar']"},
                             {tt::rbracket, "]", 14, "$['foo']['bar']"},
                             {tt::eof_, "", 15, "$['foo']['bar']"},
                         });
}

TEST_F(LexerTest, BasicIndex) {
  expect_tokens("$.foo[1]", {
                                {tt::root, "$", 0, "$.foo[1]"},
                                {tt::name, "foo", 2, "$.foo[1]"},
                                {tt::lbracket, "[", 5, "$.foo[1]"},
                                {tt::index, "1", 6, "$.foo[1]"},
                                {tt::rbracket, "]", 7, "$.foo[1]"},
                                {tt::eof_, "", 8, "$.foo[1]"},
                            });
}

TEST_F(LexerTest, MissingRootSelector) {
  expect_tokens(
      "foo.bar", {
                     {tt::error, "expected '$', found 'f'", 0, "foo.bar"},
                 });
}

TEST_F(LexerTest, RootPropertySelectorWithoutDot) {
  expect_tokens("$foo",
      {
          {tt::root, "$", 0, "$foo"},
          {tt::error, "expected '.', '..' or a bracketed selection, found 'f'",
              1, "$foo"},
      });
}

TEST_F(LexerTest, WhitespaceAfterRoot) {
  expect_tokens("$ .foo.bar", {
                                  {tt::root, "$", 0, "$ .foo.bar"},
                                  {tt::name, "foo", 3, "$ .foo.bar"},
                                  {tt::name, "bar", 7, "$ .foo.bar"},
                                  {tt::eof_, "", 10, "$ .foo.bar"},
                              });
}

TEST_F(LexerTest, WhitespaceBeforeDotProperty) {
  expect_tokens("$. foo.bar",
      {
          {tt::root, "$", 0, "$. foo.bar"},
          {tt::error, "unexpected whitespace after dot", 3, "$. foo.bar"},
      });
}

TEST_F(LexerTest, whitespaceAfterDotProperty) {
  expect_tokens("$.foo .bar", {
                                  {tt::root, "$", 0, "$.foo .bar"},
                                  {tt::name, "foo", 2, "$.foo .bar"},
                                  {tt::name, "bar", 7, "$.foo .bar"},
                                  {tt::eof_, "", 10, "$.foo .bar"},
                              });
}

TEST_F(LexerTest, BasicDotWild) {
  expect_tokens("$.foo.*", {
                               {tt::root, "$", 0, "$.foo.*"},
                               {tt::name, "foo", 2, "$.foo.*"},
                               {tt::wild, "*", 6, "$.foo.*"},
                               {tt::eof_, "", 7, "$.foo.*"},
                           });
}

TEST_F(LexerTest, BasicRecurse) {
  expect_tokens("$..foo", {
                              {tt::root, "$", 0, "$..foo"},
                              {tt::ddot, "..", 1, "$..foo"},
                              {tt::name, "foo", 3, "$..foo"},
                              {tt::eof_, "", 6, "$..foo"},
                          });
}

TEST_F(LexerTest, BasicRecurseWithTrailingDot) {
  expect_tokens(
      "$...foo", {
                     {tt::root, "$", 0, "$...foo"},
                     {tt::ddot, "..", 1, "$...foo"},
                     {tt::error, "unexpected descendant selection token '.'", 3,
                         "$...foo"},
                 });
}

TEST_F(LexerTest, ErroneousDoubleRecurse) {
  expect_tokens(
      "$....foo", {
                      {tt::root, "$", 0, "$....foo"},
                      {tt::ddot, "..", 1, "$....foo"},
                      {tt::error, "unexpected descendant selection token '.'",
                          3, "$....foo"},
                  });
}

TEST_F(LexerTest, BracketedNameSelectorDoubleQuotes) {
  expect_tokens(
      "$.foo[\"bar\"]", {
                            {tt::root, "$", 0, "$.foo[\"bar\"]"},
                            {tt::name, "foo", 2, "$.foo[\"bar\"]"},
                            {tt::lbracket, "[", 5, "$.foo[\"bar\"]"},
                            {tt::dq_string, "bar", 7, "$.foo[\"bar\"]"},
                            {tt::rbracket, "]", 11, "$.foo[\"bar\"]"},
                            {tt::eof_, "", 12, "$.foo[\"bar\"]"},
                        });
}

TEST_F(LexerTest, BracketedNameSelectorSingleQuotes) {
  expect_tokens("$.foo['bar']", {
                                    {tt::root, "$", 0, "$.foo['bar']"},
                                    {tt::name, "foo", 2, "$.foo['bar']"},
                                    {tt::lbracket, "[", 5, "$.foo['bar']"},
                                    {tt::sq_string, "bar", 7, "$.foo['bar']"},
                                    {tt::rbracket, "]", 11, "$.foo['bar']"},
                                    {tt::eof_, "", 12, "$.foo['bar']"},
                                });
}

TEST_F(LexerTest, MultipleSelectors) {
  expect_tokens("$.foo['bar', 123, *]",
      {
          {tt::root, "$", 0, "$.foo['bar', 123, *]"},
          {tt::name, "foo", 2, "$.foo['bar', 123, *]"},
          {tt::lbracket, "[", 5, "$.foo['bar', 123, *]"},
          {tt::sq_string, "bar", 7, "$.foo['bar', 123, *]"},
          {tt::comma, ",", 11, "$.foo['bar', 123, *]"},
          {tt::index, "123", 13, "$.foo['bar', 123, *]"},
          {tt::comma, ",", 16, "$.foo['bar', 123, *]"},
          {tt::wild, "*", 18, "$.foo['bar', 123, *]"},
          {tt::rbracket, "]", 19, "$.foo['bar', 123, *]"},
          {tt::eof_, "", 20, "$.foo['bar', 123, *]"},
      });
}

TEST_F(LexerTest, Slice) {
  expect_tokens("$.foo[1:3]", {
                                  {tt::root, "$", 0, "$.foo[1:3]"},
                                  {tt::name, "foo", 2, "$.foo[1:3]"},
                                  {tt::lbracket, "[", 5, "$.foo[1:3]"},
                                  {tt::index, "1", 6, "$.foo[1:3]"},
                                  {tt::colon, ":", 7, "$.foo[1:3]"},
                                  {tt::index, "3", 8, "$.foo[1:3]"},
                                  {tt::rbracket, "]", 9, "$.foo[1:3]"},
                                  {tt::eof_, "", 10, "$.foo[1:3]"},
                              });
}

TEST_F(LexerTest, Filter) {
  expect_tokens("$.foo[?@.bar]", {
                                     {tt::root, "$", 0, "$.foo[?@.bar]"},
                                     {tt::name, "foo", 2, "$.foo[?@.bar]"},
                                     {tt::lbracket, "[", 5, "$.foo[?@.bar]"},
                                     {tt::filter, "?", 6, "$.foo[?@.bar]"},
                                     {tt::current, "@", 7, "$.foo[?@.bar]"},
                                     {tt::name, "bar", 9, "$.foo[?@.bar]"},
                                     {tt::rbracket, "]", 12, "$.foo[?@.bar]"},
                                     {tt::eof_, "", 13, "$.foo[?@.bar]"},
                                 });
}

TEST_F(LexerTest, FilterParenthesizedExpression) {
  expect_tokens(
      "$.foo[?(@.bar)]", {
                             {tt::root, "$", 0, "$.foo[?(@.bar)]"},
                             {tt::name, "foo", 2, "$.foo[?(@.bar)]"},
                             {tt::lbracket, "[", 5, "$.foo[?(@.bar)]"},
                             {tt::filter, "?", 6, "$.foo[?(@.bar)]"},
                             {tt::lparen, "(", 7, "$.foo[?(@.bar)]"},
                             {tt::current, "@", 8, "$.foo[?(@.bar)]"},
                             {tt::name, "bar", 10, "$.foo[?(@.bar)]"},
                             {tt::rparen, ")", 13, "$.foo[?(@.bar)]"},
                             {tt::rbracket, "]", 14, "$.foo[?(@.bar)]"},
                             {tt::eof_, "", 15, "$.foo[?(@.bar)]"},
                         });
}

TEST_F(LexerTest, TwoFilters) {
  expect_tokens("$.foo[?@.bar, ?@.baz]",
      {
          {tt::root, "$", 0, "$.foo[?@.bar, ?@.baz]"},
          {tt::name, "foo", 2, "$.foo[?@.bar, ?@.baz]"},
          {tt::lbracket, "[", 5, "$.foo[?@.bar, ?@.baz]"},
          {tt::filter, "?", 6, "$.foo[?@.bar, ?@.baz]"},
          {tt::current, "@", 7, "$.foo[?@.bar, ?@.baz]"},
          {tt::name, "bar", 9, "$.foo[?@.bar, ?@.baz]"},
          {tt::comma, ",", 12, "$.foo[?@.bar, ?@.baz]"},
          {tt::filter, "?", 14, "$.foo[?@.bar, ?@.baz]"},
          {tt::current, "@", 15, "$.foo[?@.bar, ?@.baz]"},
          {tt::name, "baz", 17, "$.foo[?@.bar, ?@.baz]"},
          {tt::rbracket, "]", 20, "$.foo[?@.bar, ?@.baz]"},
          {tt::eof_, "", 21, "$.foo[?@.bar, ?@.baz]"},
      });
}

TEST_F(LexerTest, FilterFunction) {
  expect_tokens(
      "$[?count(@.foo)>2]", {
                                {tt::root, "$", 0, "$[?count(@.foo)>2]"},
                                {tt::lbracket, "[", 1, "$[?count(@.foo)>2]"},
                                {tt::filter, "?", 2, "$[?count(@.foo)>2]"},
                                {tt::func_, "count", 3, "$[?count(@.foo)>2]"},
                                {tt::current, "@", 9, "$[?count(@.foo)>2]"},
                                {tt::name, "foo", 11, "$[?count(@.foo)>2]"},
                                {tt::rparen, ")", 14, "$[?count(@.foo)>2]"},
                                {tt::gt, ">", 15, "$[?count(@.foo)>2]"},
                                {tt::int_, "2", 16, "$[?count(@.foo)>2]"},
                                {tt::rbracket, "]", 17, "$[?count(@.foo)>2]"},
                                {tt::eof_, "", 18, "$[?count(@.foo)>2]"},
                            });
}

TEST_F(LexerTest, FilterFunctionWithTwoArgs) {
  expect_tokens("$[?count(@.foo, 1)>2]",
      {
          {tt::root, "$", 0, "$[?count(@.foo, 1)>2]"},
          {tt::lbracket, "[", 1, "$[?count(@.foo, 1)>2]"},
          {tt::filter, "?", 2, "$[?count(@.foo, 1)>2]"},
          {tt::func_, "count", 3, "$[?count(@.foo, 1)>2]"},
          {tt::current, "@", 9, "$[?count(@.foo, 1)>2]"},
          {tt::name, "foo", 11, "$[?count(@.foo, 1)>2]"},
          {tt::comma, ",", 14, "$[?count(@.foo, 1)>2]"},
          {tt::int_, "1", 16, "$[?count(@.foo, 1)>2]"},
          {tt::rparen, ")", 17, "$[?count(@.foo, 1)>2]"},
          {tt::gt, ">", 18, "$[?count(@.foo, 1)>2]"},
          {tt::int_, "2", 19, "$[?count(@.foo, 1)>2]"},
          {tt::rbracket, "]", 20, "$[?count(@.foo, 1)>2]"},
          {tt::eof_, "", 21, "$[?count(@.foo, 1)>2]"},
      });
}

TEST_F(LexerTest, FilterParenthesizedFunction) {
  expect_tokens("$[?(count(@.foo)>2)]",
      {
          {tt::root, "$", 0, "$[?(count(@.foo)>2)]"},
          {tt::lbracket, "[", 1, "$[?(count(@.foo)>2)]"},
          {tt::filter, "?", 2, "$[?(count(@.foo)>2)]"},
          {tt::lparen, "(", 3, "$[?(count(@.foo)>2)]"},
          {tt::func_, "count", 4, "$[?(count(@.foo)>2)]"},
          {tt::current, "@", 10, "$[?(count(@.foo)>2)]"},
          {tt::name, "foo", 12, "$[?(count(@.foo)>2)]"},
          {tt::rparen, ")", 15, "$[?(count(@.foo)>2)]"},
          {tt::gt, ">", 16, "$[?(count(@.foo)>2)]"},
          {tt::int_, "2", 17, "$[?(count(@.foo)>2)]"},
          {tt::rparen, ")", 18, "$[?(count(@.foo)>2)]"},
          {tt::rbracket, "]", 19, "$[?(count(@.foo)>2)]"},
          {tt::eof_, "", 20, "$[?(count(@.foo)>2)]"},
      });
}
TEST_F(LexerTest, FilterParenthesizedFunctionArgument) {
  expect_tokens("$[?(count((@.foo),1)>2)]",
      {
          {tt::root, "$", 0, "$[?(count((@.foo),1)>2)]"},
          {tt::lbracket, "[", 1, "$[?(count((@.foo),1)>2)]"},
          {tt::filter, "?", 2, "$[?(count((@.foo),1)>2)]"},
          {tt::lparen, "(", 3, "$[?(count((@.foo),1)>2)]"},
          {tt::func_, "count", 4, "$[?(count((@.foo),1)>2)]"},
          {tt::lparen, "(", 10, "$[?(count((@.foo),1)>2)]"},
          {tt::current, "@", 11, "$[?(count((@.foo),1)>2)]"},
          {tt::name, "foo", 13, "$[?(count((@.foo),1)>2)]"},
          {tt::rparen, ")", 16, "$[?(count((@.foo),1)>2)]"},
          {tt::comma, ",", 17, "$[?(count((@.foo),1)>2)]"},
          {tt::int_, "1", 18, "$[?(count((@.foo),1)>2)]"},
          {tt::rparen, ")", 19, "$[?(count((@.foo),1)>2)]"},
          {tt::gt, ">", 20, "$[?(count((@.foo),1)>2)]"},
          {tt::int_, "2", 21, "$[?(count((@.foo),1)>2)]"},
          {tt::rparen, ")", 22, "$[?(count((@.foo),1)>2)]"},
          {tt::rbracket, "]", 23, "$[?(count((@.foo),1)>2)]"},
          {tt::eof_, "", 24, "$[?(count((@.foo),1)>2)]"},
      });
}

TEST_F(LexerTest, FilterNested) {
  expect_tokens("$[?@[?@>1]]", {
                                   {tt::root, "$", 0, "$[?@[?@>1]]"},
                                   {tt::lbracket, "[", 1, "$[?@[?@>1]]"},
                                   {tt::filter, "?", 2, "$[?@[?@>1]]"},
                                   {tt::current, "@", 3, "$[?@[?@>1]]"},
                                   {tt::lbracket, "[", 4, "$[?@[?@>1]]"},
                                   {tt::filter, "?", 5, "$[?@[?@>1]]"},
                                   {tt::current, "@", 6, "$[?@[?@>1]]"},
                                   {tt::gt, ">", 7, "$[?@[?@>1]]"},
                                   {tt::int_, "1", 8, "$[?@[?@>1]]"},
                                   {tt::rbracket, "]", 9, "$[?@[?@>1]]"},
                                   {tt::rbracket, "]", 10, "$[?@[?@>1]]"},
                                   {tt::eof_, "", 11, "$[?@[?@>1]]"},
                               });
}

TEST_F(LexerTest, FilterNestedBrackets) {
  expect_tokens("$[?@[?@[1]>1]]", {
                                      {tt::root, "$", 0, "$[?@[?@[1]>1]]"},
                                      {tt::lbracket, "[", 1, "$[?@[?@[1]>1]]"},
                                      {tt::filter, "?", 2, "$[?@[?@[1]>1]]"},
                                      {tt::current, "@", 3, "$[?@[?@[1]>1]]"},
                                      {tt::lbracket, "[", 4, "$[?@[?@[1]>1]]"},
                                      {tt::filter, "?", 5, "$[?@[?@[1]>1]]"},
                                      {tt::current, "@", 6, "$[?@[?@[1]>1]]"},
                                      {tt::lbracket, "[", 7, "$[?@[?@[1]>1]]"},
                                      {tt::index, "1", 8, "$[?@[?@[1]>1]]"},
                                      {tt::rbracket, "]", 9, "$[?@[?@[1]>1]]"},
                                      {tt::gt, ">", 10, "$[?@[?@[1]>1]]"},
                                      {tt::int_, "1", 11, "$[?@[?@[1]>1]]"},
                                      {tt::rbracket, "]", 12, "$[?@[?@[1]>1]]"},
                                      {tt::rbracket, "]", 13, "$[?@[?@[1]>1]]"},
                                      {tt::eof_, "", 14, "$[?@[?@[1]>1]]"},
                                  });
}

TEST_F(LexerTest, Function) {
  expect_tokens("$[?foo()]", {
                                 {tt::root, "$", 0, "$[?foo()]"},
                                 {tt::lbracket, "[", 1, "$[?foo()]"},
                                 {tt::filter, "?", 2, "$[?foo()]"},
                                 {tt::func_, "foo", 3, "$[?foo()]"},
                                 {tt::rparen, ")", 7, "$[?foo()]"},
                                 {tt::rbracket, "]", 8, "$[?foo()]"},
                                 {tt::eof_, "", 9, "$[?foo()]"},
                             });
}

TEST_F(LexerTest, FunctionIntLiteral) {
  expect_tokens("$[?foo(42)]", {
                                   {tt::root, "$", 0, "$[?foo(42)]"},
                                   {tt::lbracket, "[", 1, "$[?foo(42)]"},
                                   {tt::filter, "?", 2, "$[?foo(42)]"},
                                   {tt::func_, "foo", 3, "$[?foo(42)]"},
                                   {tt::int_, "42", 7, "$[?foo(42)]"},
                                   {tt::rparen, ")", 9, "$[?foo(42)]"},
                                   {tt::rbracket, "]", 10, "$[?foo(42)]"},
                                   {tt::eof_, "", 11, "$[?foo(42)]"},
                               });
}

TEST_F(LexerTest, FunctionTwoIntArgs) {
  expect_tokens(
      "$[?foo(42, -7)]", {
                             {tt::root, "$", 0, "$[?foo(42, -7)]"},
                             {tt::lbracket, "[", 1, "$[?foo(42, -7)]"},
                             {tt::filter, "?", 2, "$[?foo(42, -7)]"},
                             {tt::func_, "foo", 3, "$[?foo(42, -7)]"},
                             {tt::int_, "42", 7, "$[?foo(42, -7)]"},
                             {tt::comma, ",", 9, "$[?foo(42, -7)]"},
                             {tt::int_, "-7", 11, "$[?foo(42, -7)]"},
                             {tt::rparen, ")", 13, "$[?foo(42, -7)]"},
                             {tt::rbracket, "]", 14, "$[?foo(42, -7)]"},
                             {tt::eof_, "", 15, "$[?foo(42, -7)]"},
                         });
}

TEST_F(LexerTest, BooleanLiterals) {
  expect_tokens(
      "$[?true==false]", {
                             {tt::root, "$", 0, "$[?true==false]"},
                             {tt::lbracket, "[", 1, "$[?true==false]"},
                             {tt::filter, "?", 2, "$[?true==false]"},
                             {tt::true_, "true", 3, "$[?true==false]"},
                             {tt::eq, "==", 7, "$[?true==false]"},
                             {tt::false_, "false", 9, "$[?true==false]"},
                             {tt::rbracket, "]", 14, "$[?true==false]"},
                             {tt::eof_, "", 15, "$[?true==false]"},
                         });
}

TEST_F(LexerTest, NullLiteral) {
  expect_tokens(
      "$[?@.foo == null]", {
                               {tt::root, "$", 0, "$[?@.foo == null]"},
                               {tt::lbracket, "[", 1, "$[?@.foo == null]"},
                               {tt::filter, "?", 2, "$[?@.foo == null]"},
                               {tt::current, "@", 3, "$[?@.foo == null]"},
                               {tt::name, "foo", 5, "$[?@.foo == null]"},
                               {tt::eq, "==", 9, "$[?@.foo == null]"},
                               {tt::null_, "null", 12, "$[?@.foo == null]"},
                               {tt::rbracket, "]", 16, "$[?@.foo == null]"},
                               {tt::eof_, "", 17, "$[?@.foo == null]"},
                           });
}

TEST_F(LexerTest, LogicalAnd) {
  expect_tokens(
      "$[?true && false]", {
                               {tt::root, "$", 0, "$[?true && false]"},
                               {tt::lbracket, "[", 1, "$[?true && false]"},
                               {tt::filter, "?", 2, "$[?true && false]"},
                               {tt::true_, "true", 3, "$[?true && false]"},
                               {tt::and_, "&&", 8, "$[?true && false]"},
                               {tt::false_, "false", 11, "$[?true && false]"},
                               {tt::rbracket, "]", 16, "$[?true && false]"},
                               {tt::eof_, "", 17, "$[?true && false]"},
                           });
}

TEST_F(LexerTest, FloatLiteral) {
  expect_tokens(
      "$[?@.foo > 42.7]", {
                              {tt::root, "$", 0, "$[?@.foo > 42.7]"},
                              {tt::lbracket, "[", 1, "$[?@.foo > 42.7]"},
                              {tt::filter, "?", 2, "$[?@.foo > 42.7]"},
                              {tt::current, "@", 3, "$[?@.foo > 42.7]"},
                              {tt::name, "foo", 5, "$[?@.foo > 42.7]"},
                              {tt::gt, ">", 9, "$[?@.foo > 42.7]"},
                              {tt::float_, "42.7", 11, "$[?@.foo > 42.7]"},
                              {tt::rbracket, "]", 15, "$[?@.foo > 42.7]"},
                              {tt::eof_, "", 16, "$[?@.foo > 42.7]"},
                          });
}

// TODO: test escape sequences inside string literals