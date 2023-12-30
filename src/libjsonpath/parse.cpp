#include "libjsonpath/parse.hpp"
#include "libjsonpath/exceptions.hpp"
#include "libjsonpath/lex.hpp"
#include "libjsonpath/utils.hpp" // libjsonpath::singular_query
#include <cassert>
#include <cstdint>      // std::int32_t std::int64_t
#include <cstdlib>      // std::strtod
#include <limits>       // std::numeric_limits
#include <sstream>      // std::istringstream
#include <string>       // std::string
#include <system_error> // std::errc
#include <utility>      // std::move
#include <variant>      // std::holds_alternative std::get

namespace libjsonpath {

using namespace std::string_literals;

segments_t Parser::parse(const Tokens& tokens) const {
  TokenIterator it = tokens.cbegin();

  if (it->type == TokenType::root) {
    it++;
  }

  auto segments{parse_path(it)};

  if (it->type != TokenType::eof_) {
    throw SyntaxError(
        "expected end of query, found '{"s + std::string(it->value) + "'"s,
        *it);
  }

  return segments;
}

segments_t Parser::parse(std::string_view s) const {
  Lexer lexer{s};
  lexer.run();
  auto tokens{lexer.tokens()};
  if (tokens.size() && tokens.back().type == TokenType::error) {
    throw SyntaxError(tokens.back().value, tokens.back());
  }

  TokenIterator it = tokens.cbegin();

  if (it->type == TokenType::root) {
    it++;
  }

  auto segments{parse_path(it)};

  if (it->type != TokenType::eof_) {
    throw SyntaxError(
        "expected end of query, found '{"s + std::string(it->value) + "'"s,
        *it);
  }

  return segments;
}

segments_t Parser::parse_path(TokenIterator& tokens) const {
  segments_t segments{};
  segment_t maybe_segment;

  while (true) {
    maybe_segment = parse_segment(tokens);
    if (std::holds_alternative<std::monostate>(maybe_segment)) {
      break;
    }

    if (std::holds_alternative<Segment>(maybe_segment)) {
      segments.push_back(std::move(std::get<Segment>(maybe_segment)));
    } else if (std::holds_alternative<RecursiveSegment>(maybe_segment)) {
      segments.push_back(std::move(std::get<RecursiveSegment>(maybe_segment)));
    }

    tokens++;
  }

  return segments;
}

segments_t Parser::parse_filter_path(TokenIterator& tokens) const {
  segments_t segments{};
  segment_t maybe_segment;

  while (true) {
    maybe_segment = parse_segment(tokens);
    if (std::holds_alternative<std::monostate>(maybe_segment)) {
      tokens--;
      break;
    }

    if (std::holds_alternative<Segment>(maybe_segment)) {
      segments.push_back(std::move(std::get<Segment>(maybe_segment)));
    } else if (std::holds_alternative<RecursiveSegment>(maybe_segment)) {
      segments.push_back(std::move(std::get<RecursiveSegment>(maybe_segment)));
    }

    tokens++;
  }

  return segments;
}

segment_t Parser::parse_segment(TokenIterator& tokens) const {
  Token segment_token{*tokens};
  segment_t recursive_segment{};
  std::vector<selector_t> selectors{};

  switch (tokens->type) {
  case TokenType::name_:
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
    return RecursiveSegment{
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
    case TokenType::filter_:
      filter_token = current;
      items.push_back(parse_filter_selector(tokens));
      break;
    case TokenType::index:
      if (std::next(tokens)->type == TokenType::colon) {
        items.push_back(parse_slice_selector(tokens));
      } else {
        items.push_back(IndexSelector{current, token_to_int(*tokens)});
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
      throw SyntaxError("unexpected token in bracketed selection '"s +
                            std::string(current.value) + "'"s,
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
}

SliceSelector Parser::parse_slice_selector(TokenIterator& tokens) const {
  SliceSelector selector{*tokens, std::nullopt, std::nullopt, std::nullopt};

  if (tokens->type == TokenType::index) {
    selector.start = std::optional<std::int64_t>{token_to_int(*tokens)};
    tokens++;
    expect(tokens, TokenType::colon);
    tokens++;
  } else {
    expect(tokens, TokenType::colon);
    tokens++;
  }

  if (tokens->type == TokenType::index) {
    selector.stop = std::optional<std::int64_t>{token_to_int(*tokens)};
    tokens++;
  }

  if (tokens->type == TokenType::colon) {
    tokens++;
  }

  if (tokens->type == TokenType::index) {
    selector.step = std::optional<std::int64_t>{token_to_int(*tokens)};
    tokens++;
  }

  tokens--;
  return selector;
}

FilterSelector Parser::parse_filter_selector(TokenIterator& tokens) const {
  const auto filter_token{*tokens};
  tokens++;
  auto expr{parse_filter_expression(tokens, PRECEDENCE_LOWEST)};

  if (std::holds_alternative<Box<FunctionCall>>(expr)) {
    auto func{std::get<Box<FunctionCall>>(expr)};
    auto name{std::string{func->name}};
    if (function_result_type(name, func->token) == ExpressionType::value) {
      throw TypeError(
          "result of "s + name + "() must be compared", func->token);
    }
  }

  return FilterSelector{
      filter_token,
      std::move(expr),
  };
}

NullLiteral Parser::parse_null_literal(TokenIterator& tokens) const {
  return NullLiteral{*tokens};
}

BooleanLiteral Parser::parse_boolean_literal(TokenIterator& tokens) const {
  if (tokens->type == TokenType::false_) {
    return BooleanLiteral{*tokens, false};
  }
  return BooleanLiteral{*tokens, true};
}

StringLiteral Parser::parse_string_literal(TokenIterator& tokens) const {
  return StringLiteral{*tokens, decode_string_token(*tokens)};
}

IntegerLiteral Parser::parse_integer_literal(TokenIterator& tokens) const {
  return IntegerLiteral{*tokens, token_to_int(*tokens)};
}

FloatLiteral Parser::parse_float_literal(TokenIterator& tokens) const {
  return FloatLiteral{*tokens, token_to_double(*tokens)};
}

expression_t Parser::parse_logical_not(TokenIterator& tokens) const {
  const auto token{*tokens};
  tokens++;
  return Box(LogicalNotExpression{
      token,
      parse_filter_expression(tokens, PRECEDENCE_PREFIX),
  });
}

expression_t Parser::parse_infix(
    TokenIterator& tokens, expression_t left) const {
  auto token{*tokens};
  tokens++;
  auto precedence{get_precedence(token.type)};
  auto op{get_binary_operator(token)};
  auto right{parse_filter_expression(tokens, precedence)};

  // Non-singular queries are not allowed to be compared.
  throw_for_non_singular_query(left);
  throw_for_non_singular_query(right);

  // Use precedence to determine if the operator is a comparison operator.
  if (precedence == PRECEDENCE_COMPARISON) {
    thrown_for_non_comparable_function(left);
    thrown_for_non_comparable_function(right);
  }

  return Box(InfixExpression{
      token,
      std::move(left),  // pointer to left-hand expression
      op,               // binary operator
      std::move(right), // pointer to right-hand expression
  });
}

expression_t Parser::parse_grouped_expression(TokenIterator& tokens) const {
  tokens++;
  auto expr{parse_filter_expression(tokens, PRECEDENCE_LOWEST)};
  tokens++;

  while (tokens->type != TokenType::rparen) {
    if (tokens->type == TokenType::eof_) {
      throw SyntaxError("unbalanced parentheses", *tokens);
    }
    expr = parse_infix(tokens, std::move(expr));
  }

  expect(tokens, TokenType::rparen);
  return expr;
}

expression_t Parser::parse_root_query(TokenIterator& tokens) const {
  const auto token{*tokens};
  tokens++;
  return Box(RootQuery{
      token,
      parse_filter_path(tokens),
  });
}

expression_t Parser::parse_relative_query(TokenIterator& tokens) const {
  const auto token{*tokens};
  tokens++;
  return Box(RelativeQuery{
      token,
      parse_filter_path(tokens),
  });
}

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
    throw SyntaxError(
        "unexpected end of filter expression, found rbracket", *tokens);
  case TokenType::eof_:
    throw SyntaxError(
        "unexpected end of filter expression, found eof", *tokens);
  default:
    throw SyntaxError("unexpected filter expression token " +
                          token_type_to_string(tokens->type),
        *tokens);
  }
}

expression_t Parser::parse_function_call(TokenIterator& tokens) const {
  const auto token{*tokens};
  tokens++;
  std::vector<expression_t> args{};

  while (tokens->type != TokenType::rparen) {
    expression_t node{parse_filter_token(tokens)};

    // Is this argument part of a comparison or logical expression?
    while (BINARY_OPERATORS.find(std::next(tokens)->type) !=
           BINARY_OPERATORS.end()) {
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

    tokens++;
  }

  expect(tokens, TokenType::rparen);
  throw_for_function_signature(token, args);

  return Box(FunctionCall{
      token,
      token.value,
      std::move(args),
  });
}

expression_t Parser::parse_filter_expression(
    TokenIterator& tokens, int precedence) const {
  expression_t node{parse_filter_token(tokens)};

  while (true) {
    auto peek_type{std::next(tokens)->type};
    if (peek_type == TokenType::eof_ || peek_type == TokenType::rbracket ||
        (get_precedence(peek_type) < precedence)) {
      break;
    }

    if (BINARY_OPERATORS.find(peek_type) == BINARY_OPERATORS.end()) {
      // BINARY_OPERATORS does not contain peek_type
      return node;
    }

    tokens++;
    node = parse_infix(tokens, std::move(node));
  }

  return node;
}

void Parser::expect(TokenIterator it, TokenType tt) const {
  if (it->type != tt) {
    throw SyntaxError("unexpected token, expected "s +
                          token_type_to_string(tt) + " found "s +
                          token_type_to_string(it->type),
        *it);
  }
}

void Parser::expect_peek(TokenIterator it, TokenType tt) const {
  if (std::next(it)->type != tt) {
    throw SyntaxError("unexpected token, expected "s +
                          token_type_to_string(tt) + " found "s +
                          token_type_to_string(std::next(it)->type),
        *(std::next(it)));
  }
}

int Parser::get_precedence(TokenType tt) const noexcept {
  auto it{PRECEDENCES.find(tt)};
  if (it == PRECEDENCES.end()) {
    return PRECEDENCE_LOWEST;
  }
  return it->second;
}

BinaryOperator Parser::get_binary_operator(const Token& t) const {
  auto it{BINARY_OPERATORS.find(t.type)};
  if (it == BINARY_OPERATORS.end()) {
    throw SyntaxError("unknown operator "s + std::string(t.value), t);
  }
  return it->second;
}

std::string Parser::decode_string_token(const Token& t) const {
  std::string s{t.value};
  if (t.type == TokenType::sq_string) {
    // replaceAll(s, "\"", "\\\"");
    replaceAll(s, "\\'", "'");
  }
  return unescape_json_string(s, t);
}

void Parser::throw_for_non_singular_query(const expression_t& expr) const {
  if (std::holds_alternative<Box<RootQuery>>(expr)) {
    const auto& root_query{std::get<Box<RootQuery>>(expr)};
    if (!singular_query(root_query->query)) {
      throw TypeError(
          "non-singular query is not comparable", root_query->token);
    }
  }

  if (std::holds_alternative<Box<RelativeQuery>>(expr)) {
    auto& relative_query{std::get<Box<RelativeQuery>>(expr)};
    if (!singular_query(relative_query->query)) {
      throw TypeError(
          "non-singular query is not comparable", relative_query->token);
    }
  }
}

void Parser::thrown_for_non_comparable_function(
    const expression_t& expr) const {
  if (std::holds_alternative<Box<FunctionCall>>(expr)) {
    auto func{std::get<Box<FunctionCall>>(expr)};
    auto name{std::string{func->name}};
    if (function_result_type(name, func->token) != ExpressionType::value) {
      throw TypeError(
          "result of "s + name + "() is not comparable", func->token);
    }
  }
}

std::int64_t Parser::token_to_int(const Token& t) const {
  if (t.value.size() > 1 && t.value.rfind("0", 0) == 0) {
    if (t.type == TokenType::index) {
      throw SyntaxError(
          "array indicies with a leading zero are not allowed", t);
    }
    throw SyntaxError("integers with a leading zero are not allowed", t);
  }

  if (t.value.rfind("-0", 0) == 0) {
    if (t.type == TokenType::index) {
      throw SyntaxError("negative zero array indicies are not allowed", t);
    }

    if (t.value.size() > 2) {
      throw SyntaxError("integers with a leading zero are not allowed", t);
    }
  }

  // Handle integer literals in scientific notation by converting to a double
  // then cast to an integer.
  std::string null_terminated_str(t.value);
  std::istringstream iss(null_terminated_str);
  std::int64_t result;
  double double_result;
  iss >> double_result;

  // Check if double_result is within the range of int
  if (double_result >= std::numeric_limits<std::int64_t>::min() &&
      double_result <=
          static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
    result = static_cast<int64_t>(double_result);
  } else {
    throw Exception(
        "integer conversion failed for '"s + null_terminated_str + "'"s, t);
  }

  return result;
}

double Parser::token_to_double(const Token& t) const {
  // NOTE: double std::from_chars is not yet implemented, sometimes.
  std::string null_terminated_str(t.value);
  char* endptr;
  double result = std::strtod(null_terminated_str.c_str(), &endptr);
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

std::string Parser::unescape_json_string(
    std::string_view sv, const Token& token) const {
  std::string rv{};
  unsigned char byte{};    // current byte
  char digit;              // escape sequence hex digit
  std::int32_t code_point; // decoded \uXXXX or \uXXXX\uXXXX escape sequence
  std::string::size_type index{0}; // current byte index in sv
  std::string::size_type end{0};   // index of the end of a 4 hex digit escape
  std::string::size_type length{sv.length()};

  while (index < length) {
    byte = sv[index++];

    if (byte == '\\') {
      if (index < length) {
        digit = sv[index++];
      } else {
        throw SyntaxError("invalid escape", token);
      }

      switch (digit) {
      case '"':
        rv.push_back('"');
        break;
      case '\\':
        rv.push_back('\\');
        break;
      case '/':
        rv.push_back('/');
        break;
      case 'b':
        rv.push_back('\b');
        break;
      case 'f':
        rv.push_back('\f');
        break;
      case 'n':
        rv.push_back('\n');
        break;
      case 'r':
        rv.push_back('\r');
        break;
      case 't':
        rv.push_back('\t');
        break;
      case 'u':
        // Decode 4 hex digits.
        code_point = 0;
        end = index + 4;
        if (end > length) {
          throw SyntaxError("invalid \\uXXXX escape", token);
        }

        for (; index < end; index++) {
          digit = sv[index];
          code_point <<= 4;
          switch (digit) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            code_point |= (digit - '0');
            break;
          case 'a':
          case 'b':
          case 'c':
          case 'd':
          case 'e':
          case 'f':
            code_point |= (digit - 'a' + 10);
            break;
          case 'A':
          case 'B':
          case 'C':
          case 'D':
          case 'E':
          case 'F':
            code_point |= (digit - 'A' + 10);
            break;
          default:
            throw SyntaxError("invalid \\uXXXX escape", token);
          }
        }

        // Is the code point a high surrogate followed by another 6 byte escape
        // sequence?
        if ((code_point >= 0xD800 && code_point <= 0xDBFF) &&
            index + 6 <= length && sv[index] == '\\' && sv[index + 1] == 'u') {
          index += 2;
          std::int32_t low_surrogate = 0;
          end += 6;

          for (; index < end; index++) {
            digit = sv[index];
            low_surrogate <<= 4;
            switch (digit) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
              low_surrogate |= (digit - '0');
              break;
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
              low_surrogate |= (digit - 'a' + 10);
              break;
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
              low_surrogate |= (digit - 'A' + 10);
              break;
            default:
              throw SyntaxError("invalid \\uXXXX escape", token);
            }
          }

          // Combine high and low surrogates into a Unicode code point.
          code_point = 0x10000 + (((code_point & 0x03FF) << 10) |
                                     (low_surrogate & 0x03FF));
        }

        rv.append(encode_utf8(code_point, token));
        break;
      default:
        throw SyntaxError("invalid escape", token);
      }

    } else {
      // Find invalid characters.
      // Bytes that are less than 0x1f and not a continuation byte.
      if ((byte & 0x80) == 0) {
        // Single-byte code point
        if (byte <= 0x1F) {
          throw SyntaxError("invalid character in string literal", token);
        }
        rv.push_back(byte);
      } else if ((byte & 0xE0) == 0xC0) {
        // Start of two-byte code point
        rv.push_back(byte);
        rv.push_back(sv[index++]);
      } else if ((byte & 0xF0) == 0xE0) {
        // Start of a three-byte code point
        rv.push_back(byte);
        rv.push_back(sv[index++]);
        rv.push_back(sv[index++]);
      } else if ((byte & 0xF8) == 0xF0) {
        // Start of a four-byte code point
        rv.push_back(byte);
        rv.push_back(sv[index++]);
        rv.push_back(sv[index++]);
        rv.push_back(sv[index++]);
      } else {
        throw EncodingError("invalid character for string literal", token);
      }
    }
  }

  return rv;
}

std::string Parser::encode_utf8(
    std::int32_t code_point, const Token& token) const {
  std::string rv;

  if (code_point <= 0x7F) {
    // Single-byte UTF-8 encoding for code points up to 7F(hex)
    rv += static_cast<char>(code_point & 0x7F);
  } else if (code_point <= 0x7FF) {
    // Two-byte UTF-8 encoding for code points up to 7FF(hex)
    rv += static_cast<char>(0xC0 | ((code_point >> 6) & 0x1F));
    rv += static_cast<char>(0x80 | (code_point & 0x3F));
  } else if (code_point <= 0xFFFF) {
    // Three-byte UTF-8 encoding for code points up to FFFF(hex)
    rv += static_cast<char>(0xE0 | ((code_point >> 12) & 0x0F));
    rv += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
    rv += static_cast<char>(0x80 | (code_point & 0x3F));
  } else if (code_point <= 0x10FFFF) {
    // Four-byte UTF-8 encoding for code points up to 10FFFF(hex)
    rv += static_cast<char>(0xF0 | ((code_point >> 18) & 0x07));
    rv += static_cast<char>(0x80 | ((code_point >> 12) & 0x3F));
    rv += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
    rv += static_cast<char>(0x80 | (code_point & 0x3F));
  } else {
    throw EncodingError("invalid code point", token);
  }

  return rv;
}

ExpressionType Parser::function_result_type(
    std::string name, const Token& t) const {
  auto it{m_function_extensions.find(name)};
  if (it == m_function_extensions.end()) {
    throw NameError("no such function '"s + name + "'"s, t);
  }
  return it->second.res;
}

struct ValueTypeVisitor {
  const Token& m_token;
  const size_t m_index;
  const std::unordered_map<std::string, FunctionExtensionTypes>&
      m_function_extensions;

  ValueTypeVisitor(const Token& t, size_t index,
      const std::unordered_map<std::string, FunctionExtensionTypes>&
          function_extensions)
      : m_token{t}, m_index{index}, m_function_extensions{function_extensions} {
  }

  void operator()(const NullLiteral&) const {};

  void operator()(const BooleanLiteral&) const {};
  void operator()(const IntegerLiteral&) const {};
  void operator()(const FloatLiteral&) const {};
  void operator()(const StringLiteral&) const {};

  void operator()(const Box<LogicalNotExpression>&) const {
    throw TypeError(std::string{m_token.value} + "() argument " +
                        std::to_string(m_index) + " must be of ValueType",
        m_token);
  };

  void operator()(const Box<InfixExpression>&) const {
    throw TypeError(std::string{m_token.value} + "() argument " +
                        std::to_string(m_index) + " must be of ValueType",
        m_token);
  };

  void operator()(const Box<RelativeQuery>& expression) const {
    if (!singular_query(expression->query)) {
      throw TypeError(std::string{m_token.value} + "() argument " +
                          std::to_string(m_index) + " must be of ValueType",
          m_token);
    }
  };

  void operator()(const Box<RootQuery>& expression) const {
    if (!singular_query(expression->query)) {
      throw TypeError(std::string{m_token.value} + "() argument " +
                          std::to_string(m_index) + " must be of ValueType",
          m_token);
    }
  };

  void operator()(const Box<FunctionCall>& expression) const {
    auto name{std::string{expression->name}};
    auto it{m_function_extensions.find(name)};

    if (it == m_function_extensions.end()) {
      throw NameError("no such function '"s + name + "'"s, m_token);
    }

    FunctionExtensionTypes ext{it->second};

    if (ext.res != ExpressionType::value) {
      throw TypeError(std::string{m_token.value} + "() argument " +
                          std::to_string(m_index) + " must be of ValueType",
          m_token);
    }
  };
};

void Parser::throw_for_function_signature(
    const Token& t, const std::vector<expression_t>& args) const {
  auto name{std::string{t.value}};
  auto it{m_function_extensions.find(name)};

  if (it == m_function_extensions.end()) {
    throw NameError("no such function '"s + name + "'"s, t);
  }

  FunctionExtensionTypes ext{it->second};

  // Correct number of arguments
  if (args.size() != ext.args.size()) {
    throw TypeError(name + "() takes "s + std::to_string(ext.args.size()) +
                        " argument"s + (ext.args.size() == 1 ? ""s : "s") +
                        ", " + std::to_string(args.size()) + " given",
        t);
  }

  // Argument types
  for (size_t i = 0; i < ext.args.size(); i++) {
    auto typ{ext.args[i]};
    auto arg{args[i]};

    switch (typ) {
    case ExpressionType::value:
      std::visit(ValueTypeVisitor(t, i, m_function_extensions), arg);
      break;
    case ExpressionType::logical:
      if (!(std::holds_alternative<Box<RelativeQuery>>(arg) ||
              std::holds_alternative<Box<RootQuery>>(arg) ||
              std::holds_alternative<Box<InfixExpression>>(arg) ||
              std::holds_alternative<Box<LogicalNotExpression>>(arg))) {
        throw TypeError(name + "() argument " + std::to_string(i) +
                            " must be of LogicalType",
            t);
      }
      break;
    case ExpressionType::nodes:
      if (!(std::holds_alternative<Box<RelativeQuery>>(arg) ||
              std::holds_alternative<Box<RootQuery>>(arg) ||
              (std::holds_alternative<Box<FunctionCall>>(arg) &&
                  function_result_type(
                      std::string{std::get<Box<FunctionCall>>(arg)->name}, t) ==
                      ExpressionType::nodes))) {
        throw TypeError(
            name + "() argument " + std::to_string(i) + " must be of NodesType",
            t);
      }

    default:
      break;
    }
  }
}

} // namespace libjsonpath
