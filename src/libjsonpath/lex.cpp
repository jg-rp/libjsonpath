#include "libjsonpath/lex.hpp"
#include "libjsonpath/exceptions.hpp"
#include <cassert>

namespace libjsonpath {

using namespace std::string_literals;

Lexer::Lexer(std::string_view query)
    : m_query{query}, m_length{query.length()} {}

Lexer::State Lexer::lex_root() {
  const auto c{next()};
  if (c && c.value() != '$') {
    backup();
    error("expected '$', found '"s + c.value_or(' ') + "'"s);
    return ERROR;
  }
  emit(TokenType::root);
  return LEX_SEGMENT;
}

Lexer::State Lexer::lex_segment() {
  if (ignore_whitespace() && !(peek())) {
    error("trailing whitespace");
    return ERROR;
  }

  const auto maybe_c{next()};
  if (!maybe_c) {
    emit(TokenType::eof_);
    return NONE;
  }

  const auto c{maybe_c.value()};
  switch (c) {
  case '.':
    if (peek().value_or('\0') == '.') {
      next();
      emit(TokenType::ddot);
      return LEX_DESCENDANT_SELECTION;
    }
    return LEX_DOT_SELECTOR;
  case '[':
    emit(TokenType::lbracket);
    return LEX_INSIDE_BRACKETED_SELECTION;
  default:
    backup();
    if (m_filter_nesting_level) {
      return LEX_INSIDE_FILTER;
    }
    error("expected '.', '..' or a bracketed selection, found '"s + c + "'"s);
    return ERROR;
  }
}

Lexer::State Lexer::lex_descendant_selection() {
  const auto maybe_c(next());
  if (!maybe_c) {
    error("bald descendant segment");
    return ERROR;
  }

  const auto c{maybe_c.value()};
  switch (c) {
  case '*':
    emit(TokenType::wild);
    return LEX_SEGMENT;
  case '[':
    emit(TokenType::lbracket);
    return LEX_INSIDE_BRACKETED_SELECTION;
  default:
    backup();
    if (accept_name()) {
      emit(TokenType::name_);
      return LEX_SEGMENT;
    } else {
      error("unexpected descendant selection token '"s + c + "'"s);
      return ERROR;
    }
  }
}

Lexer::State Lexer::lex_dot_selector() {
  ignore(); // Ignore the dot.

  if (ignore_whitespace()) {
    error("unexpected whitespace after dot");
    return ERROR;
  }

  const auto c{next()};
  if (c && c.value() == '*') {
    emit(TokenType::wild);
    return LEX_SEGMENT;
  }

  backup();
  if (accept_name()) {
    emit(TokenType::name_);
    return LEX_SEGMENT;
  } else {
    error("unexpected shorthand selector '"s + c.value() + "'"s);
    return ERROR;
  }
}

Lexer::State Lexer::lex_inside_bracketed_selection() {
  std::optional<char> c;
  while (true) {
    ignore_whitespace();
    c = next();

    if (!c) {
      error("unclosed bracketed selection"); // string view literals?
      return ERROR;
    }

    switch (c.value()) {
    case ']':
      emit(TokenType::rbracket);
      return m_filter_nesting_level ? LEX_INSIDE_FILTER : LEX_SEGMENT;
    case '*':
      emit(TokenType::wild);
      continue;
    case '?':
      emit(TokenType::filter_);
      m_filter_nesting_level++;
      return LEX_INSIDE_FILTER;
    case ',':
      emit(TokenType::comma);
      continue;
    case ':':
      emit(TokenType::colon);
      continue;
    case '\'':
      return LEX_INSIDE_SINGLE_QUOTED_STRING;
    case '"':
      return LEX_INSIDE_DOUBLE_QUOTED_STRING;
    case '-':
      if (!(accept_run(s_digits))) {
        error("expected at least one digit after a minus sign");
        return ERROR;
      }
      // A negative index.
      emit(TokenType::index);
      continue;
    default:
      backup();

      if (accept_run(s_digits)) {
        emit(TokenType::index);
        continue;
      } else {
        error("unexpected token in bracketed selection");
        return ERROR;
      }
    }
  }
}

Lexer::State Lexer::lex_inside_filter() {
  std::optional<char> c;
  std::string_view v;

  while (true) {
    ignore_whitespace();
    c = next();

    if (!c) {
      m_filter_nesting_level--;
      if (m_paren_stack.size() == 1) {
        error("unbalanced parentheses");
        return ERROR;
      }
      backup();
      return LEX_INSIDE_BRACKETED_SELECTION;
    }

    switch (c.value()) {
    case ']':
      m_filter_nesting_level--;
      if (m_paren_stack.size() == 1) {
        error("unbalanced parentheses");
        return ERROR;
      }
      backup();
      return LEX_INSIDE_BRACKETED_SELECTION;
    case ',':
      emit(TokenType::comma);
      // If we have unbalanced parens, we are inside a function call and a
      // comma separates arguments. Otherwise a comma separates selectors.
      if (m_paren_stack.size()) {
        continue;
      }
      m_filter_nesting_level--;
      return LEX_INSIDE_BRACKETED_SELECTION;
    case '\'':
      return LEX_INSIDE_SINGLE_QUOTED_FILTER_STRING;
    case '"':
      return LEX_INSIDE_DOUBLE_QUOTED_FILTER_STRING;
    case '(':
      emit(TokenType::lparen);
      // Are we in a function call? If so, a function argument contains parens.
      if (m_paren_stack.size()) {
        int paren_count{m_paren_stack.top()};
        m_paren_stack.pop();
        m_paren_stack.push(paren_count + 1);
      }
      continue;
    case ')':
      emit(TokenType::rparen);
      // Are we closing a function call or a parenthesized expression?
      if (m_paren_stack.size()) {
        if (m_paren_stack.top() == 1) {
          m_paren_stack.pop();
        } else {
          int paren_count{m_paren_stack.top()};
          m_paren_stack.pop();
          m_paren_stack.push(paren_count - 1);
        }
      }
      continue;
    case '$':
      emit(TokenType::root);
      return LEX_SEGMENT;
    case '@':
      emit(TokenType::current);
      return LEX_SEGMENT;
    case '.':
      backup();
      return LEX_SEGMENT;
    case '!':
      if (peek().value_or(' ') == '=') {
        next();
        emit(TokenType::ne);
      } else {
        emit(TokenType::not_);
      }
      continue;
    case '=':
      if (peek().value_or(' ') == '=') {
        next();
        emit(TokenType::eq);
        continue;
      } else {
        backup();
        error("unexpected filter selector token '='");
        return ERROR;
      }
    case '<':
      if (peek().value_or(' ') == '=') {
        next();
        emit(TokenType::le);
      } else {
        emit(TokenType::lt);
      }
      continue;
    case '>':
      if (peek().value_or(' ') == '=') {
        next();
        emit(TokenType::ge);
      } else {
        emit(TokenType::gt);
      }
      continue;
    case '-':
      if (!(accept_run(s_digits))) {
        error("at least one digit is required after a minus sign");
        return ERROR;
      }

      // A float?
      if (peek().value_or(' ') == '.') {
        next();
        if (!(accept_run(s_digits))) {
          error("a fractional digit is required a decimal point");
          return ERROR;
        }

        // Exponent?
        if (accept('e')) {
          accept(s_sign);
          if (!(accept(s_digits))) {
            error("at least one exponent digit is required");
            return ERROR;
          }
        }

        emit(TokenType::float_);
        continue;
      }

      // Exponent?
      if (accept('e')) {
        if (accept('-')) {
          // Emit a float if we have a negative exponent.
          if (!(accept(s_digits))) {
            error("at least one exponent digit is required");
            return ERROR;
          }
          emit(TokenType::float_);
          continue;
        }

        accept('+');
        if (!(accept(s_digits))) {
          error("at least one exponent digit is required");
          return ERROR;
        }
      }

      emit(TokenType::int_);
      continue;

    default:
      backup();

      // Non-negative int or float?
      if (accept_run(s_digits)) {
        if (peek().value_or(' ') == '.') {
          next();
          if (!(accept_run(s_digits))) {
            error("a fractional digit is required a decimal point");
            return ERROR;
          }

          // Exponent?
          if (accept('e')) {
            accept(s_sign);
            if (!(accept(s_digits))) {
              error("at least one exponent digit is required");
              return ERROR;
            }
          }

          emit(TokenType::float_);
          continue;
        }

        // Exponent?
        if (accept('e')) {
          if (accept('-')) {
            // Emit a float if we have a negative exponent.
            if (!(accept(s_digits))) {
              error("at least one exponent digit is required");
              return ERROR;
            }
            emit(TokenType::float_);
            continue;
          }

          accept('+');
          if (!(accept(s_digits))) {
            error("at least one exponent digit is required");
            return ERROR;
          }
        }

        emit(TokenType::int_);
        continue;
      }

      v = view();

      if (v.rfind("&&", 0) == 0) {
        m_pos += 2;
        emit(TokenType::and_);
        continue;
      }

      if (v.rfind("||", 0) == 0) {
        m_pos += 2;
        emit(TokenType::or_);
        continue;
      }

      if (v.rfind("true", 0) == 0) {
        m_pos += 4;
        emit(TokenType::true_);
        continue;
        ;
      }

      if (v.rfind("false", 0) == 0) {
        m_pos += 5;
        emit(TokenType::false_);
        continue;
      }

      if (v.rfind("null", 0) == 0) {
        m_pos += 4;
        emit(TokenType::null_);
        continue;
      }

      // Function call?
      if (accept(s_function_name_first)) {
        accept_run(s_function_name_char);

        if (peek().value_or(' ') != '(') {
          error("expected a function call");
          return ERROR;
        }

        m_paren_stack.push(1);
        emit(TokenType::func_);
        next(); // Discard the left paren.
        ignore();
        continue;
      }
    }

    error("unexpected filter selection token '"s + c.value() + "'"s);
    return ERROR;
  }
}

void Lexer::run() {
  auto current_state{LEX_ROOT};

  while (true) {
    switch (current_state) {
    case ERROR:
    case NONE:
      return;
    case LEX_ROOT:
      current_state = lex_root();
      break;
    case LEX_SEGMENT:
      current_state = lex_segment();
      break;
    case LEX_DESCENDANT_SELECTION:
      current_state = lex_descendant_selection();
      break;
    case LEX_DOT_SELECTOR:
      current_state = lex_dot_selector();
      break;
    case LEX_INSIDE_BRACKETED_SELECTION:
      current_state = lex_inside_bracketed_selection();
      break;
    case LEX_INSIDE_FILTER:
      current_state = lex_inside_filter();
      break;
    case LEX_INSIDE_SINGLE_QUOTED_STRING:
      current_state = lex_inside_string<LEX_INSIDE_BRACKETED_SELECTION, '\'',
          TokenType::sq_string>();
      break;
    case LEX_INSIDE_DOUBLE_QUOTED_STRING:
      current_state = lex_inside_string<LEX_INSIDE_BRACKETED_SELECTION, '"',
          TokenType::dq_string>();
      break;
    case LEX_INSIDE_SINGLE_QUOTED_FILTER_STRING:
      current_state =
          lex_inside_string<LEX_INSIDE_FILTER, '\'', TokenType::sq_string>();
      break;
    case LEX_INSIDE_DOUBLE_QUOTED_FILTER_STRING:
      current_state =
          lex_inside_string<LEX_INSIDE_FILTER, '"', TokenType::dq_string>();
      break;
    default:
      error("unknown lexer state {}");
      return;
    }
  }
}

void Lexer::emit(TokenType t) {
  std::string_view view{m_query};
  view.remove_prefix(m_start);
  view.remove_suffix(m_length - m_pos);
  m_tokens.push_back(Token{t, view, m_start, m_query});
  m_start = m_pos;
}

std::optional<char> Lexer::next() {
  if (m_pos >= m_length) {
    return std::nullopt;
  }

  return std::optional<char>(std::in_place, m_query[m_pos++]);
}

std::string_view Lexer::view() const {
  std::string_view view_{m_query};
  view_.remove_prefix(m_pos);
  return view_;
}

void Lexer::ignore() { m_start = m_pos; }

void Lexer::backup() {
  // assert(m_pos > m_start && "can't backup beyond start");
  if (m_pos > m_start) {
    --m_pos;
  }
}

std::optional<char> Lexer::peek() {
  const auto c = next();
  if (c) {
    backup();
  }

  return c;
}

bool Lexer::accept(const char ch) {
  const auto c = next();
  if (c && c.value() == ch) {
    return true;
  }

  backup();
  return false;
}

bool Lexer::accept(const std::unordered_set<char>& valid) {
  const auto c = next();
  if (c && valid.find(c.value()) != valid.end()) {
    return true;
  }

  if (c) {
    backup();
  }

  return false;
}

bool Lexer::accept_run(const std::unordered_set<char>& valid) {
  auto found{false};
  auto c = next();
  while (c && valid.find(c.value()) != valid.end()) {
    c = next();
    found = true;
  }

  if (c) {
    backup();
  }

  return found;
}

bool Lexer::accept_name() {
  if (!accept_name_first()) {
    return false;
  }

  while (true) {
    if (!accept_name_char()) {
      break;
    }
  }

  return true;
}

bool Lexer::accept_name_first() {
  std::optional<char> maybe_c = next();
  if (!maybe_c) {
    return false;
  }

  char byte{maybe_c.value()};

  if ((byte & 0x80) == 0) {
    // Single byte character (0x00-0x74)
    if ((byte >= 0x41 && byte <= 0x5A) || byte == 0x5F ||
        (byte >= 0x61 && byte <= 0x7A)) {
      return true; // A-Z or a-z or _
    }
    backup();
    return false;
  } else if ((byte & 0xE0) == 0xC0) {
    // Two-byte character (0x80-0x07FF)
    accept_continuation_byte();
    return true;
  } else if ((byte & 0xF0) == 0xE0) {
    // Three-byte character (0x0800-0xFFFF)
    accept_continuation_byte();
    accept_continuation_byte();
    return true;
  } else if ((byte & 0xF8) == 0xF0) {
    // Four-byte character (0x010000-0x10FFFF)
    accept_continuation_byte();
    accept_continuation_byte();
    accept_continuation_byte();
    return true;
  }
  backup();
  return false;
}

bool Lexer::accept_name_char() {
  std::optional<char> maybe_c = next();
  if (!maybe_c) {
    return false;
  }

  char byte{maybe_c.value()};

  if ((byte & 0x80) == 0) {
    // Single byte character (0x00-0x74)
    if ((byte >= 0x30 && byte <= 0x39) || (byte >= 0x41 && byte <= 0x5A) ||
        byte == 0x5F || (byte >= 0x61 && byte <= 0x7A)) {
      return true; // 0-9 or A-Z or a-z or _
    }
    backup();
    return false;
  } else if ((byte & 0xE0) == 0xC0) {
    // Two-byte character (0x80-0x07FF)
    accept_continuation_byte();
    return true;
  } else if ((byte & 0xF0) == 0xE0) {
    // Three-byte character (0x0800-0xFFFF)
    accept_continuation_byte();
    accept_continuation_byte();
    return true;
  } else if ((byte & 0xF8) == 0xF0) {
    // Four-byte character (0x010000-0x10FFFF)
    accept_continuation_byte();
    accept_continuation_byte();
    accept_continuation_byte();
    return true;
  } else {
    error("invalid UTF-8");
    throw LexerError(m_error, Token{TokenType::error, m_error, m_pos, m_query});
  }
}

void Lexer::accept_continuation_byte() {
  std::optional<char> continuation_byte{next()};
  if (!(continuation_byte && (continuation_byte.value() & 0xC0) == 0x80)) {
    error("invalid UTF-8");
    throw LexerError(m_error, Token{TokenType::error, m_error, m_pos, m_query});
  }
}

bool Lexer::ignore_whitespace() {
  // This would be a bug, not an exception for programmers to catch.
  assert(m_pos == m_start && "must emit or ignore before consuming whitespace");
  if (accept_run(Lexer::s_whitespace)) {
    ignore();
    return true;
  }
  return false;
}

void Lexer::error(std::string_view message) {
  m_error = message;
  m_tokens.push_back(Token{TokenType::error, m_error, m_pos, m_query});
}

} // namespace libjsonpath
