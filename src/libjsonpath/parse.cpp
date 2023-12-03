#include "libjsonpath/parse.hpp"
#include "libjsonpath/exceptions.hpp"
#include "libjsonpath/utils.hpp" // libjsonpath::singular_query
#include <cassert>
#include <charconv>     // std::from_chars
#include <cstdlib>      // std::strtod
#include <format>       // std::format
#include <system_error> // std::errc
#include <utility>      // std::move
#include <variant>      // std::holds_alternative std::get

namespace libjsonpath {

segments_t Parser::parse(const Tokens& tokens) const {
  TokenIterator it = tokens.cbegin();

  if (it->type == TokenType::root) {
    it++;
  }

  auto segments{parse_path(it, false)};

  if (it->type != TokenType::eof_) {
    throw SyntaxError(
        std::format("expected end of query, found '{}'", it->value), *it);
  }

  return segments;
};

segments_t Parser::parse_path(TokenIterator& tokens, bool in_filter) const {
  segments_t segments{};
  segment_t maybe_segment;

  while (true) {
    maybe_segment = parse_segment(tokens);
    if (std::holds_alternative<std::monostate>(maybe_segment)) {
      if (in_filter) {
        tokens--;
      }
      break;
    } else if (std::holds_alternative<Segment>(maybe_segment)) {
      segments.push_back(std::move(std::get<Segment>(maybe_segment)));
    } else if (std::holds_alternative<RecursiveSegment>(maybe_segment)) {
      segments.push_back(std::move(std::get<RecursiveSegment>(maybe_segment)));
    }

    tokens++;
  }

  return segments;
};

segment_t Parser::parse_segment(TokenIterator& tokens) const {
  Token segment_token{*tokens};
  segment_t recursive_segment{};
  std::vector<selector_t> selectors{};

  switch (tokens->type) {
  case TokenType::name:
    selectors.push_back(
        NameSelector{*tokens, decode_string_token(*tokens), true});
    break;
  case TokenType::wild:
    selectors.push_back(WildSelector{*tokens, true});
    break;
  case TokenType::lbracket:
    selectors = parse_bracketed_selection(tokens);
    break;
  case TokenType::ddot:
    tokens++;
    recursive_segment = parse_segment(tokens);
    // A missing selection after a recursive descent segment should
    // have been caught by the lexer.
    assert(std::holds_alternative<Segment>(recursive_segment) &&
           "unexpected recursive segment");
    return Segment{
        segment_token,
        std::move(std::get<Segment>(recursive_segment).selectors),
    };
  default:
    return segment_t{}; // monostate
  }

  return Segment{segment_token, std::move(selectors)};
}

std::vector<selector_t> Parser::parse_bracketed_selection(
    TokenIterator& tokens) const {
  std::vector<selector_t> items{};
  auto segment_token{*tokens};
  tokens++; // move past left bracket
  auto current{*tokens};
  Token filter_token;

  while (current.type != TokenType::rbracket) {
    switch (current.type) {
    case TokenType::dq_string:
    case TokenType::sq_string:
      items.push_back(
          NameSelector{current, decode_string_token(current), false});
      break;
    case TokenType::filter:
      filter_token = current;
      items.push_back(parse_filter_selector(tokens));
      break;
    case TokenType::index:
      if (std::next(tokens)->type == TokenType::colon) {
        items.push_back(parse_slice_selector(tokens));
      } else {
        items.push_back(IndexSelector{current, svtoi(tokens->value)});
      }
      break;
    case TokenType::colon:
      items.push_back(parse_slice_selector(tokens));
      break;
    case TokenType::wild:
      items.push_back(WildSelector{current, false});
      break;
    case TokenType::eof_:
      throw SyntaxError("unexpected end of query", current);
    default:
      throw SyntaxError(
          std::format(
              "unexpected token in bracketed selection '{}'", current.value),
          current);
    }

    if (std::next(tokens)->type != TokenType::rbracket) {
      expect_peek(tokens, TokenType::comma);
      tokens++; // move to comma
    }

    tokens++; // move past comma or right bracket
    current = *tokens;
  }

  if (!items.size()) {
    throw SyntaxError("empty bracketed segment", segment_token);
  }

  return items;
};

SliceSelector Parser::parse_slice_selector(TokenIterator& tokens) const {
  SliceSelector selector{*tokens, std::nullopt, std::nullopt, std::nullopt};

  if (tokens->type == TokenType::index) {
    selector.start = std::optional<int>{svtoi(tokens->value)};
    tokens++;
    expect(tokens, TokenType::colon);
    tokens++;
  } else {
    expect(tokens, TokenType::colon);
    tokens++;
  }

  if (tokens->type == TokenType::index) {
    selector.stop = std::optional<int>{svtoi(tokens->value)};
    tokens++;
  }

  if (tokens->type == TokenType::colon) {
    tokens++;
  }

  if (tokens->type == TokenType::index) {
    selector.step = std::optional<int>{svtoi(tokens->value)};
    tokens++;
  }

  tokens--;
  return selector;
};

std::unique_ptr<FilterSelector> Parser::parse_filter_selector(
    TokenIterator& tokens) const {
  const auto filter_token{*tokens};
  tokens++;
  return std::make_unique<FilterSelector>(FilterSelector{
      filter_token,
      parse_filter_expression(tokens, PRECEDENCE_LOWEST),
  });
};

NullLiteral Parser::parse_null_literal(TokenIterator& tokens) const {
  return NullLiteral{*tokens};
};

BooleanLiteral Parser::parse_boolean_literal(TokenIterator& tokens) const {
  if (tokens->type == TokenType::false_) {
    return BooleanLiteral{*tokens, false};
  }
  return BooleanLiteral{*tokens, true};
};

StringLiteral Parser::parse_string_literal(TokenIterator& tokens) const {
  return StringLiteral{*tokens, decode_string_token(*tokens)};
};

IntegerLiteral Parser::parse_integer_literal(TokenIterator& tokens) const {
  return IntegerLiteral{*tokens, svtoi(tokens->value)};
};

FloatLiteral Parser::parse_float_literal(TokenIterator& tokens) const {
  return FloatLiteral{*tokens, svtod(tokens->value)};
};

std::unique_ptr<LogicalNotExpression> Parser::parse_logical_not(
    TokenIterator& tokens) const {
  const auto token{*tokens};
  tokens++;
  return std::make_unique<LogicalNotExpression>(LogicalNotExpression{
      token,
      parse_filter_expression(tokens, PRECEDENCE_PREFIX),
  });
};

std::unique_ptr<InfixExpression> Parser::parse_infix(
    TokenIterator& tokens, expression_t left) const {
  auto token{*tokens};
  tokens++;
  auto right{parse_filter_expression(tokens, get_precedence(token.type))};

  // XXX: should this only be for expression with comparison operators?
  throw_for_non_singular_query(left);
  throw_for_non_singular_query(right);

  return std::make_unique<InfixExpression>(InfixExpression{
      token,
      std::move(left),                 // pointer to left-hand expression
      get_binary_operator(token.type), // binary operator
      std::move(right),
  });
};

expression_t Parser::parse_grouped_expression(TokenIterator& tokens) const {
  tokens++;
  auto expr{parse_filter_expression(tokens, PRECEDENCE_LOWEST)};
  tokens++;

  while (tokens->type != TokenType::rparen) {
    if (tokens->type == TokenType::eof_) {
      // TODO: raise an exception
      assert(false && "unbalanced parentheses");
    }
    expr = parse_infix(tokens, std::move(expr));
  }

  expect(tokens, TokenType::rparen);
  return expr;
}

std::unique_ptr<RootQuery> Parser::parse_root_query(
    TokenIterator& tokens) const {
  const auto token{*tokens};
  tokens++;
  return std::make_unique<RootQuery>(RootQuery{
      token,
      parse_path(tokens, true),
  });
};

std::unique_ptr<RelativeQuery> Parser::parse_relative_query(
    TokenIterator& tokens) const {
  const auto token{*tokens};
  tokens++;
  return std::make_unique<RelativeQuery>(RelativeQuery{
      token,
      parse_path(tokens, true),
  });
};

expression_t Parser::parse_filter_token(TokenIterator& tokens) const {
  switch (tokens->type) {
  case TokenType::false_:
  case TokenType::true_:
    return parse_boolean_literal(tokens);
  case TokenType::int_:
    return parse_integer_literal(tokens);
  case TokenType::float_:
    return parse_float_literal(tokens);
  case TokenType::lparen:
    return parse_grouped_expression(tokens);
  case TokenType::not_:
    return parse_logical_not(tokens);
  case TokenType::null_:
    return parse_null_literal(tokens);
  case TokenType::root:
    return parse_root_query(tokens);
  case TokenType::current:
    return parse_relative_query(tokens);
  case TokenType::dq_string:
  case TokenType::sq_string:
    return parse_string_literal(tokens);
  case TokenType::func_:
    return parse_function_call(tokens);
  case TokenType::rbracket:
    // TODO: raise an exception
    // unexpected end of expression (eof ir rbracket)?
    assert(false && "unexpected end of filter expression, found rbracket");
  case TokenType::eof_:
    // TODO: raise an exception
    // unexpected end of expression (eof ir rbracket)?
    assert(false && "unexpected end of filter expression, found eof");
    break;
  default:
    // TODO: raise an exception
    // unexpected end of expression (eof ir rbracket)?
    assert(false && "unexpected token for filter expression");
    break;
  }
}

std::unique_ptr<FunctionCall> Parser::parse_function_call(
    TokenIterator& tokens) const {
  const auto token{*tokens};
  tokens++;
  std::vector<expression_t> args{};

  while (tokens->type != TokenType::rparen) {
    expression_t node{parse_filter_token(tokens)};

    // Is this function call part of a comparison or logical expression?
    while (BINARY_OPERATORS.contains(std::next(tokens)->type)) {
      tokens++;
      node = parse_infix(tokens, std::move(node));
    }

    args.push_back(std::move(node));

    if (std::next(tokens)->type != TokenType::rparen) {
      if (std::next(tokens)->type == TokenType::rbracket) {
        break;
      }
      expect_peek(tokens, TokenType::comma);
      tokens++; // move past comma
    }
  }

  expect(tokens, TokenType::rparen);

  return std::make_unique<FunctionCall>(FunctionCall{
      token,
      token.value,
      std::move(args),
  });
};

expression_t Parser::parse_filter_expression(
    TokenIterator& tokens, int precedence) const {
  expression_t node{parse_filter_token(tokens)};

  while (true) {
    auto peek_type{std::next(tokens)->type};
    if (peek_type == TokenType::eof_ || peek_type == TokenType::rbracket ||
        (get_precedence(peek_type) < precedence)) {
      break;
    }

    if (!BINARY_OPERATORS.contains(peek_type)) {
      return node;
    }

    tokens++;
    node = parse_infix(tokens, std::move(node));
  }

  return node;
};

void Parser::expect(TokenIterator it, TokenType tt) const {
  // TODO: raise an exception
  // TODO: token type to string
  assert(it->type == tt && "unexpected token");
}

void Parser::expect_peek(TokenIterator it, TokenType tt) const {
  // TODO: raise an exception
  // TODO: token type to string
  assert(std::next(it)->type == tt && "unexpected token");
}

int Parser::get_precedence(TokenType tt) const noexcept {
  auto it{PRECEDENCES.find(tt)};
  if (it == PRECEDENCES.end()) {
    return PRECEDENCE_LOWEST;
  }
  return it->second;
}

BinaryOperator Parser::get_binary_operator(TokenType tt) const {
  auto it{BINARY_OPERATORS.find(tt)};
  if (it == BINARY_OPERATORS.end()) {
    // TODO: raise an exception
    assert(false && "unknown operator");
  }
  return it->second;
}

std::string Parser::decode_string_token(const Token& t) const {
  std::string s{t.value};
  if (t.type == TokenType::sq_string) {
    replaceAll(s, "\"", "\\\"");
    replaceAll(s, "\\'", "'");
  }

  // TODO: replace UTF-16 escape sequences with unicode chars
  return s;
}

void Parser::throw_for_non_singular_query(const expression_t& expr) const {
  if (std::holds_alternative<std::unique_ptr<RootQuery>>(expr)) {
    const auto& root_query{std::get<std::unique_ptr<RootQuery>>(expr)};
    if (!singular_query(root_query->query)) {
      throw SyntaxError(
          "non-singular query is not comparable", root_query->token);
    }
  }

  if (std::holds_alternative<std::unique_ptr<RelativeQuery>>(expr)) {
    auto& relative_query{std::get<std::unique_ptr<RelativeQuery>>(expr)};
    if (!singular_query(relative_query->query)) {
      throw SyntaxError(
          "non-singular query is not comparable", relative_query->token);
    }
  }
}

std::int64_t Parser::svtoi(std::string_view sv) const {
  if (sv.size() > 1 && sv.starts_with("0")) {
    // TODO: raise an exception
    assert(false && "leading zero in array index");
  }

  if (sv.starts_with("-0")) {
    // TODO: raise an exception
    assert(false && "negative zero");
  }

  std::int64_t number;
  auto [_, err] = std::from_chars(sv.data(), sv.data() + sv.size(), number);
  assert(err == std::errc{});
  return number;
}

double Parser::svtod(std::string_view sv) const {
  if (sv.size() > 1 && sv.starts_with("0")) {
    // TODO: raise an exception
    assert(false && "leading zero in array index");
  }

  if (sv.starts_with("-0")) {
    // TODO: raise an exception
    assert(false && "negative zero");
  }

  // NOTE: double from_chars is not yet implemented, sometimes.
  std::string null_terminated_str(sv);
  char* endptr;
  double result = std::strtod(null_terminated_str.c_str(), &endptr);

  // TODO: raise an exception
  assert(endptr != null_terminated_str.c_str() && "can't convert to double");
  assert(*endptr == '\0' && "double conversion stopped early");
  return result;
}

void Parser::replaceAll(std::string& original, std::string_view substring,
    std::string_view replacement) const {
  size_t pos = 0;
  const size_t substring_length = substring.length();
  const size_t replacement_length = replacement.length();

  while ((pos = original.find(substring, pos)) != std::string::npos) {
    original.replace(pos, substring_length, replacement);
    pos += replacement_length;
  }
}

} // namespace libjsonpath
