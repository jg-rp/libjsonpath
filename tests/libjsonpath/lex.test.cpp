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

TEST_F(LexerTest, RootPropertySelectorWithoutDot) {
  expect_tokens("$foo",
      {
          {tt::root, "$", 0},
          {tt::error, "expected '.', '..' or a bracketed selection, found 'f'",
              1},
      });
}

TEST_F(LexerTest, WhitespaceAfterRoot) {
  expect_tokens("$ .foo.bar", {
                                  {tt::root, "$", 0},
                                  {tt::name, "foo", 3},
                                  {tt::name, "bar", 7},
                                  {tt::eof_, "", 10},
                              });
}

TEST_F(LexerTest, WhitespaceBeforeDotProperty) {
  expect_tokens(
      "$. foo.bar", {
                        {tt::root, "$", 0},
                        {tt::error, "unexpected whitespace after dot", 3},
                    });
}

TEST_F(LexerTest, whitespaceAfterDotProperty) {
  expect_tokens("$.foo .bar", {
                                  {tt::root, "$", 0},
                                  {tt::name, "foo", 2},
                                  {tt::name, "bar", 7},
                                  {tt::eof_, "", 10},
                              });
}

TEST_F(LexerTest, BasicDotWild) {
  expect_tokens("$.foo.*", {
                               {tt::root, "$", 0},
                               {tt::name, "foo", 2},
                               {tt::wild, "*", 6},
                               {tt::eof_, "", 7},
                           });
}

TEST_F(LexerTest, BasicRecurse) {
  expect_tokens("$..foo", {
                              {tt::root, "$", 0},
                              {tt::ddot, "..", 1},
                              {tt::name, "foo", 3},
                              {tt::eof_, "", 6},
                          });
}

TEST_F(LexerTest, BasicRecurseWithTrailingDot) {
  expect_tokens("$...foo",
      {{tt::root, "$", 0}, {tt::ddot, "..", 1},
          {tt::error, "unexpected descendant selection token '.'", 3}});
}

TEST_F(LexerTest, ErroneousDoubleRecurse) {
  expect_tokens("$....foo",
      {
          {tt::root, "$", 0},
          {tt::ddot, "..", 1},
          {tt::error, "unexpected descendant selection token '.'", 3},
      });
}

TEST_F(LexerTest, BracketedNameSelectorDoubleQuotes) {
  expect_tokens("$.foo[\"bar\"]", {
                                      {tt::root, "$", 0},
                                      {tt::name, "foo", 2},
                                      {tt::lbracket, "[", 5},
                                      {tt::dq_string, "bar", 7},
                                      {tt::rbracket, "]", 11},
                                      {tt::eof_, "", 12},
                                  });
}

TEST_F(LexerTest, BracketedNameSelectorSingleQuotes) {
  expect_tokens("$.foo['bar']", {
                                    {tt::root, "$", 0},
                                    {tt::name, "foo", 2},
                                    {tt::lbracket, "[", 5},
                                    {tt::sq_string, "bar", 7},
                                    {tt::rbracket, "]", 11},
                                    {tt::eof_, "", 12},
                                });
}

TEST_F(LexerTest, MultipleSelectors) {
  expect_tokens("$.foo['bar', 123, *]", {
                                            {tt::root, "$", 0},
                                            {tt::name, "foo", 2},
                                            {tt::lbracket, "[", 5},
                                            {tt::sq_string, "bar", 7},
                                            {tt::comma, ",", 11},
                                            {tt::index, "123", 13},
                                            {tt::comma, ",", 16},
                                            {tt::wild, "*", 18},
                                            {tt::rbracket, "]", 19},
                                            {tt::eof_, "", 20},
                                        });
}

TEST_F(LexerTest, Slice) {
  expect_tokens("$.foo[1:3]", {
                                  {tt::root, "$", 0},
                                  {tt::name, "foo", 2},
                                  {tt::lbracket, "[", 5},
                                  {tt::index, "1", 6},
                                  {tt::colon, ":", 7},
                                  {tt::index, "3", 8},
                                  {tt::rbracket, "]", 9},
                                  {tt::eof_, "", 10},
                              });
}

TEST_F(LexerTest, Filter) {
  expect_tokens("$.foo[?@.bar]", {
                                     {tt::root, "$", 0},
                                     {tt::name, "foo", 2},
                                     {tt::lbracket, "[", 5},
                                     {tt::filter, "?", 6},
                                     {tt::current, "@", 7},
                                     {tt::name, "bar", 9},
                                     {tt::rbracket, "]", 12},
                                     {tt::eof_, "", 13},
                                 });
}

TEST_F(LexerTest, FilterParenthesizedExpression) {
  expect_tokens("$.foo[?(@.bar)]", {
                                       {tt::root, "$", 0},
                                       {tt::name, "foo", 2},
                                       {tt::lbracket, "[", 5},
                                       {tt::filter, "?", 6},
                                       {tt::lparen, "(", 7},
                                       {tt::current, "@", 8},
                                       {tt::name, "bar", 10},
                                       {tt::rparen, ")", 13},
                                       {tt::rbracket, "]", 14},
                                       {tt::eof_, "", 15},
                                   });
}

TEST_F(LexerTest, TwoFilters) {
  expect_tokens("$.foo[?@.bar, ?@.baz]", {
                                             {tt::root, "$", 0},
                                             {tt::name, "foo", 2},
                                             {tt::lbracket, "[", 5},
                                             {tt::filter, "?", 6},
                                             {tt::current, "@", 7},
                                             {tt::name, "bar", 9},
                                             {tt::comma, ",", 12},
                                             {tt::filter, "?", 14},
                                             {tt::current, "@", 15},
                                             {tt::name, "baz", 17},
                                             {tt::rbracket, "]", 20},
                                             {tt::eof_, "", 21},
                                         });
}

TEST_F(LexerTest, FilterFunction) {
  expect_tokens("$[?count(@.foo)>2]", {
                                          {tt::root, "$", 0},
                                          {tt::lbracket, "[", 1},
                                          {tt::filter, "?", 2},
                                          {tt::func_, "count", 3},
                                          {tt::current, "@", 9},
                                          {tt::name, "foo", 11},
                                          {tt::rparen, ")", 14},
                                          {tt::gt, ">", 15},
                                          {tt::int_, "2", 16},
                                          {tt::rbracket, "]", 17},
                                          {tt::eof_, "", 18},
                                      });
}

TEST_F(LexerTest, FilterFunctionWithTwoArgs) {
  expect_tokens("$[?count(@.foo, 1)>2]", {
                                             {tt::root, "$", 0},
                                             {tt::lbracket, "[", 1},
                                             {tt::filter, "?", 2},
                                             {tt::func_, "count", 3},
                                             {tt::current, "@", 9},
                                             {tt::name, "foo", 11},
                                             {tt::comma, ",", 14},
                                             {tt::int_, "1", 16},
                                             {tt::rparen, ")", 17},
                                             {tt::gt, ">", 18},
                                             {tt::int_, "2", 19},
                                             {tt::rbracket, "]", 20},
                                             {tt::eof_, "", 21},
                                         });
}

TEST_F(LexerTest, FilterParenthesizedFunction) {
  expect_tokens("$[?(count(@.foo)>2)]", {
                                            {tt::root, "$", 0},
                                            {tt::lbracket, "[", 1},
                                            {tt::filter, "?", 2},
                                            {tt::lparen, "(", 3},
                                            {tt::func_, "count", 4},
                                            {tt::current, "@", 10},
                                            {tt::name, "foo", 12},
                                            {tt::rparen, ")", 15},
                                            {tt::gt, ">", 16},
                                            {tt::int_, "2", 17},
                                            {tt::rparen, ")", 18},
                                            {tt::rbracket, "]", 19},
                                            {tt::eof_, "", 20},
                                        });
}
TEST_F(LexerTest, FilterParenthesizedFunctionArgument) {
  expect_tokens("$[?(count((@.foo),1)>2)]", {
                                                {tt::root, "$", 0},
                                                {tt::lbracket, "[", 1},
                                                {tt::filter, "?", 2},
                                                {tt::lparen, "(", 3},
                                                {tt::func_, "count", 4},
                                                {tt::lparen, "(", 10},
                                                {tt::current, "@", 11},
                                                {tt::name, "foo", 13},
                                                {tt::rparen, ")", 16},
                                                {tt::comma, ",", 17},
                                                {tt::int_, "1", 18},
                                                {tt::rparen, ")", 19},
                                                {tt::gt, ">", 20},
                                                {tt::int_, "2", 21},
                                                {tt::rparen, ")", 22},
                                                {tt::rbracket, "]", 23},
                                                {tt::eof_, "", 24},
                                            });
}

TEST_F(LexerTest, FilterNested) {
  expect_tokens("$[?@[?@>1]]", {
                                   {tt::root, "$", 0},
                                   {tt::lbracket, "[", 1},
                                   {tt::filter, "?", 2},
                                   {tt::current, "@", 3},
                                   {tt::lbracket, "[", 4},
                                   {tt::filter, "?", 5},
                                   {tt::current, "@", 6},
                                   {tt::gt, ">", 7},
                                   {tt::int_, "1", 8},
                                   {tt::rbracket, "]", 9},
                                   {tt::rbracket, "]", 10},
                                   {tt::eof_, "", 11},
                               });
}

TEST_F(LexerTest, FilterNestedBrackets) {
  expect_tokens("$[?@[?@[1]>1]]", {
                                      {tt::root, "$", 0},
                                      {tt::lbracket, "[", 1},
                                      {tt::filter, "?", 2},
                                      {tt::current, "@", 3},
                                      {tt::lbracket, "[", 4},
                                      {tt::filter, "?", 5},
                                      {tt::current, "@", 6},
                                      {tt::lbracket, "[", 7},
                                      {tt::index, "1", 8},
                                      {tt::rbracket, "]", 9},
                                      {tt::gt, ">", 10},
                                      {tt::int_, "1", 11},
                                      {tt::rbracket, "]", 12},
                                      {tt::rbracket, "]", 13},
                                      {tt::eof_, "", 14},
                                  });
}

TEST_F(LexerTest, Function) {
  expect_tokens("$[?foo()]", {
                                 {tt::root, "$", 0},
                                 {tt::lbracket, "[", 1},
                                 {tt::filter, "?", 2},
                                 {tt::func_, "foo", 3},
                                 {tt::rparen, ")", 7},
                                 {tt::rbracket, "]", 8},
                                 {tt::eof_, "", 9},
                             });
}

TEST_F(LexerTest, FunctionIntLiteral) {
  expect_tokens("$[?foo(42)]", {
                                   {tt::root, "$", 0},
                                   {tt::lbracket, "[", 1},
                                   {tt::filter, "?", 2},
                                   {tt::func_, "foo", 3},
                                   {tt::int_, "42", 7},
                                   {tt::rparen, ")", 9},
                                   {tt::rbracket, "]", 10},
                                   {tt::eof_, "", 11},
                               });
}

TEST_F(LexerTest, FunctionTwoIntArgs) {
  expect_tokens("$[?foo(42, -7)]", {
                                       {tt::root, "$", 0},
                                       {tt::lbracket, "[", 1},
                                       {tt::filter, "?", 2},
                                       {tt::func_, "foo", 3},
                                       {tt::int_, "42", 7},
                                       {tt::comma, ",", 9},
                                       {tt::int_, "-7", 11},
                                       {tt::rparen, ")", 13},
                                       {tt::rbracket, "]", 14},
                                       {tt::eof_, "", 15},
                                   });
}

TEST_F(LexerTest, BooleanLiterals) {
  expect_tokens("$[?true==false]", {
                                       {tt::root, "$", 0},
                                       {tt::lbracket, "[", 1},
                                       {tt::filter, "?", 2},
                                       {tt::true_, "true", 3},
                                       {tt::eq, "==", 7},
                                       {tt::false_, "false", 9},
                                       {tt::rbracket, "]", 14},
                                       {tt::eof_, "", 15},
                                   });
}

TEST_F(LexerTest, LogicalAnd) {
  expect_tokens("$[?true && false]", {
                                         {tt::root, "$", 0},
                                         {tt::lbracket, "[", 1},
                                         {tt::filter, "?", 2},
                                         {tt::true_, "true", 3},
                                         {tt::and_, "&&", 8},
                                         {tt::false_, "false", 11},
                                         {tt::rbracket, "]", 16},
                                         {tt::eof_, "", 17},
                                     });
}

TEST_F(LexerTest, FloatLiteral) {
  expect_tokens("$[?@.foo > 42.7]", {
                                        {tt::root, "$", 0},
                                        {tt::lbracket, "[", 1},
                                        {tt::filter, "?", 2},
                                        {tt::current, "@", 3},
                                        {tt::name, "foo", 5},
                                        {tt::gt, ">", 9},
                                        {tt::float_, "42.7", 11},
                                        {tt::rbracket, "]", 15},
                                        {tt::eof_, "", 16},
                                    });
}
