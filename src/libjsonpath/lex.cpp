#include "libjsonpath/lex.h"
#include <cassert>
#include <format>
#include <iostream>

namespace libjsonpath {

State Lexer::lexRoot() {
  const auto c{next()};
  if (c && c.value() != '$') {
    backup();
    error(std::format("expected '$', found '{}'", c.value_or(' ')));
    return State::error;
  }
  emit(TokenType::root);
  return State::lexSegment;
};

State Lexer::lexSegment() {
  if (ignore_whitespace() && !(peek())) {
    error("trailing whitespace");
    return State::error;
  }

  const auto maybe_c(next());
  if (!maybe_c) {
    emit(TokenType::eof_);
    return State::none;
  }

  const auto c{maybe_c.value()};
  switch (c) {
  case '.':
    if (peek().value_or('\0') == '.') {
      emit(TokenType::ddot);
      return State::lexDescendantSelection;
    }
    return State::lexDotSelector;
  case '[':
    emit(TokenType::lbracket);
    return State::lexInsideBracketedSelection;
  default:
    backup();
    if (filter_nesting_level_) {
      return State::lexInsideFilter;
    }
    error(std::format(
        "expected '.', '..' or a bracketed selection, found '{}'", c));
    return State::error;
  }
}

State Lexer::lexDescendantSelection() {
  const auto maybe_c(next());
  if (!maybe_c) {
    error("bald descendant segment");
    return State::error;
  }

  const auto c{maybe_c.value()};
  switch (c) {
  case '*':
    emit(TokenType::wild);
    return State::lexSegment;
  case '[':
    emit(TokenType::lbracket);
    return State::lexInsideBracketedSelection;
  default:
    backup();
    if (accept_name()) {
      emit(TokenType::name);
      return State::lexSegment;
    } else {
      error(std::format("unexpected descendant selection token '{}'", c));
      return State::error;
    }
  }
}

State Lexer::lexDotSelector() {
  ignore(); // Ignore the dot.

  if (ignore_whitespace()) {
    error("unexpected whitespace after dot");
    return State::error;
  }

  const auto c{next()};
  if (c && c.value() == '*') {
    emit(TokenType::wild);
    return State::lexSegment;
  }

  backup();
  if (accept_name()) {
    emit(TokenType::name);
    return State::lexSegment;
  } else {
    error("unexpected shorthand selection");
    return State::error;
  }
}

State Lexer::lexInsideBracketedSelection() {
  std::optional<char> c;
  while (true) {
    ignore_whitespace();
    c = next();

    if (!c) {
      error("unclosed bracketed selection"); // string view literals?
      return State::error;
    }

    switch (c.value()) {
    case ']':
      emit(TokenType::rbracket);
      return filter_nesting_level_ ? State::lexInsideFilter : State::lexSegment;
    case '*':
      emit(TokenType::wild);
      continue;
    case '?':
      emit(TokenType::filter);
      filter_nesting_level_++;
      return State::lexInsideFilter;
    case ',':
      emit(TokenType::comma);
      continue;
    case ':':
      emit(TokenType::colon);
      continue;
    case '\'':
      return State::lexInsideSingleQuotedString;
    case '"':
      return State::lexInsideDoubleQuotedString;
    case '-':
      if (!(accept_run(digits))) {
        error("expected at least one digit after a minus sign");
        return State::error;
      }
      // A negative index.
      emit(TokenType::index);
      continue;
    default:
      backup();

      if (accept_run(digits)) {
        emit(TokenType::index);
        continue;
      } else {
        error("unexpected token in bracketed selection");
        return State::error;
      }
    }
  }
}

State Lexer::lexInsideFilter() {
  std::optional<char> c;
  std::string_view v;

  while (true) {
    ignore_whitespace();
    c = next();

    if (!c) {
      error("unbalanced parentheses");
      return State::error;
    }

    switch (c.value()) {
    case ']':
      filter_nesting_level_--;
      if (paren_stack_.size() == 1) {
        error("unbalanced parentheses");
        return State::error;
      }
      backup();
      return State::lexInsideBracketedSelection;
    case ',':
      emit(TokenType::comma);
      // If we have unbalanced parens, we are inside a function call and a
      // comma separates arguments. Otherwise a comma separates selectors.
      if (paren_stack_.size()) {
        continue;
      }
      filter_nesting_level_++;
      return State::lexInsideBracketedSelection;
    case '\'':
      return State::lexInsideSingleQuotedFilterString;
    case '"':
      return State::lexInsideDoubleQuotedFilterString;
    case '(':
      emit(TokenType::lparen);
      // Are we in a function call? If so, a function argument contains parens.
      if (paren_stack_.size()) {
        int paren_count{paren_stack_.top()};
        paren_stack_.pop();
        paren_stack_.push(paren_count + 1);
        continue;
      }
    case ')':
      emit(TokenType::rparen);
      // Are we closing a function call or a parenthesized expression?
      if (paren_stack_.size()) {
        if (paren_stack_.top() == 1) {
          paren_stack_.pop();
        } else {
          int paren_count{paren_stack_.top()};
          paren_stack_.pop();
          paren_stack_.push(paren_count + 1);
        }
      }
      continue;
    case '$':
      emit(TokenType::root);
      return State::lexSegment;
    case '@':
      emit(TokenType::current);
      return State::lexSegment;
    case '.':
      backup();
      return State::lexSegment;
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
      } else {
        backup();
        error("unexpected filter selector token '='");
        return State::error;
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
      if (!(accept_run(digits))) {
        error("at least one digit is required after a minus sign");
        return State::error;
      }

      // A float?
      if (peek().value_or(' ') == '.') {
        next();
        if (!(accept_run(digits))) {
          error("a fractional digit is required a decimal point");
          return State::error;
        }

        // Exponent?
        if (accept('e')) {
          accept(sign);
          if (!(accept(digits))) {
            error("at least one exponent digit is required");
            return State::error;
          }
        }

        emit(TokenType::float_);
        continue;
      }

      // Exponent?
      if (accept('e')) {
        accept(sign);
        if (!(accept(digits))) {
          error("at least one exponent digit is required");
          return State::error;
        }
      }

      emit(TokenType::int_);
      continue;

    default:
      backup();

      // Non-negative int or float?
      if (accept_run(digits)) {
        if (peek().value_or(' ') == '.') {
          next();
          if (!(accept_run(digits))) {
            error("a fractional digit is required a decimal point");
            return State::error;
          }

          // Exponent?
          if (accept('e')) {
            accept(sign);
            if (!(accept(digits))) {
              error("at least one exponent digit is required");
              return State::error;
            }
          }

          emit(TokenType::float_);
          continue;
        }

        // Exponent?
        if (accept('e')) {
          accept(sign);
          if (!(accept(digits))) {
            error("at least one exponent digit is required");
            return State::error;
          }
        }

        emit(TokenType::int_);
        continue;
      }

      v = view();

      if (v.starts_with("&&")) {
        emit(TokenType::and_);
        pos_ += 2;
        continue;
        ;
      }

      if (v.starts_with("||")) {
        emit(TokenType::or_);
        pos_ += 2;
        continue;
        ;
      }

      if (v.starts_with("true")) {
        emit(TokenType::true_);
        pos_ += 4;
        continue;
        ;
      }

      if (v.starts_with("false")) {
        emit(TokenType::false_);
        pos_ += 5;
        continue;
        ;
      }

      if (v.starts_with("null")) {
        emit(TokenType::null_);
        pos_ += 2;
        continue;
        ;
      }

      // Function call?
      if (accept(function_name_first)) {
        accept_run(function_name_char);

        if (peek().value_or(' ') != '(') {
          error("expected a function call");
          return State::error;
        }

        paren_stack_.push(1);
        emit(TokenType::func_);
        next(); // Discard the left paren.
        ignore();
        continue;
      }
    }

    error(std::format("unexpected filter selection token '{}'", c.value()));
    return State::error;
  }
}

State Lexer::lexInsideSingleQuotedString() {
  ignore(); // Discard the opening quote.

  // Empty string?
  if (peek().value_or(' ') == '\'') {
    emit(TokenType::sq_string);
    next(); // Discard the closing quote.
    ignore();
    return State::lexInsideBracketedSelection;
  }

  std::string_view v;
  std::optional<char> c;

  while (true) {
    v = view();
    c = next();

    if (v.starts_with("\\\\") || v.starts_with("\\\'")) {
      next();
      continue;
    }

    if (c.value_or(' ') == '\\' && !(escapes.contains(peek().value_or(' ')))) {
      error(
          std::format("invalid escape sequence '\\{}'", peek().value_or(' ')));
      return State::error;
    }

    if (!c) {
      error(std::format("unclosed string starting at index {}", start_));
      return State::error;
    }

    if (c.value_or(' ') == '\'') {
      backup();
      emit(TokenType::sq_string);
      next(); // Discard the closing quote.
      ignore();
      return State::lexInsideBracketedSelection;
    }
  }
}

State Lexer::lexInsideDoubleQuotedString() {
  ignore(); // Discard the opening quote.

  // Empty string?
  if (peek().value_or(' ') == '"') {
    emit(TokenType::dq_string);
    next(); // Discard the closing quote.
    ignore();
    return State::lexInsideBracketedSelection;
  }

  std::string_view v;
  std::optional<char> c;

  while (true) {
    v = view();
    c = next();

    if (v.starts_with("\\\\") || v.starts_with("\\\"")) {
      next();
      continue;
    }

    if (c.value_or(' ') == '\\' && !(escapes.contains(peek().value_or(' ')))) {
      error(
          std::format("invalid escape sequence '\\{}'", peek().value_or(' ')));
      return State::error;
    }

    if (!c) {
      error(std::format("unclosed string starting at index {}", start_));
      return State::error;
    }

    if (c.value_or(' ') == '"') {
      backup();
      emit(TokenType::dq_string);
      next(); // Discard the closing quote.
      ignore();
      return State::lexInsideBracketedSelection;
    }
  }
}

State Lexer::lexInsideSingleQuotedFilterString() {
  ignore(); // Discard the opening quote.

  // Empty string?
  if (peek().value_or(' ') == '\'') {
    emit(TokenType::sq_string);
    next(); // Discard the closing quote.
    ignore();
    return State::lexInsideFilter;
  }

  std::string_view v;
  std::optional<char> c;

  while (true) {
    v = view();
    c = next();

    if (v.starts_with("\\\\") || v.starts_with("\\\'")) {
      next();
      continue;
    }

    if (c.value_or(' ') == '\\' && !(escapes.contains(peek().value_or(' ')))) {
      error(
          std::format("invalid escape sequence '\\{}'", peek().value_or(' ')));
      return State::error;
    }

    if (!c) {
      error(std::format("unclosed string starting at index {}", start_));
      return State::error;
    }

    if (c.value_or(' ') == '\'') {
      backup();
      emit(TokenType::sq_string);
      next(); // Discard the closing quote.
      ignore();
      return State::lexInsideFilter;
    }
  }
}

State Lexer::lexInsideDoubleQuotedFilterString() {
  ignore(); // Discard the opening quote.

  // Empty string?
  if (peek().value_or(' ') == '"') {
    emit(TokenType::dq_string);
    next(); // Discard the closing quote.
    ignore();
    return State::lexInsideFilter;
  }

  std::string_view v;
  std::optional<char> c;

  while (true) {
    v = view();
    c = next();

    if (v.starts_with("\\\\") || v.starts_with("\\\"")) {
      next();
      continue;
    }

    if (c.value_or(' ') == '\\' && !(escapes.contains(peek().value_or(' ')))) {
      error(
          std::format("invalid escape sequence '\\{}'", peek().value_or(' ')));
      return State::error;
    }

    if (!c) {
      error(std::format("unclosed string starting at index {}", start_));
      return State::error;
    }

    if (c.value_or(' ') == '"') {
      backup();
      emit(TokenType::dq_string);
      next(); // Discard the closing quote.
      ignore();
      return State::lexInsideFilter;
    }
  }
}

Lexer::Lexer(std::string_view query)
    : query_{query}, length_{query.length()} {};

void Lexer::run() {
  auto current_state{State::lexRoot};

  while (true) {
    switch (current_state) {
    case State::error:
    case State::none:
      return;
    case State::lexRoot:
      current_state = lexRoot();
      break;
    case State::lexSegment:
      current_state = lexSegment();
      break;
    case State::lexDescendantSelection:
      current_state = lexDescendantSelection();
      break;
    case State::lexDotSelector:
      current_state = lexDotSelector();
      break;
    case State::lexInsideBracketedSelection:
      current_state = lexInsideBracketedSelection();
      break;
    case State::lexInsideFilter:
      current_state = lexInsideFilter();
      break;
    case State::lexInsideSingleQuotedString:
      current_state = lexInsideSingleQuotedString();
      break;
    case State::lexInsideDoubleQuotedString:
      current_state = lexInsideDoubleQuotedString();
      break;
    case State::lexInsideSingleQuotedFilterString:
      current_state = lexInsideSingleQuotedFilterString();
      break;
    case State::lexInsideDoubleQuotedFilterString:
      current_state = lexInsideDoubleQuotedFilterString();
      break;
    default:
      error("unknown lexer state {}");
      return;
    }
  }
};

void Lexer::emit(TokenType t) {
  std::string_view view{query_};
  view.remove_prefix(start_);
  view.remove_suffix(length_ - pos_);
  tokens_.push_back(Token{t, view, start_});
  start_ = pos_;
};

std::optional<char> Lexer::next() {
  if (pos_ >= length_) {
    return std::nullopt;
  }

  return std::optional<char>(std::in_place, query_[pos_++]);
};

std::string_view Lexer::view() const {
  std::string_view view_{query_};
  view_.remove_prefix(pos_);
  return view_;
}

void Lexer::ignore() { start_ = pos_; }

void Lexer::backup() {
  assert(pos_ > start_ && "can't backup beyond start");
  --pos_;
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
  if (c && valid.contains(c.value())) {
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
  while (c && valid.contains(c.value())) {
    c = next();
    found = true;
  }

  if (c) {
    backup();
  }

  return found;
}

bool Lexer::accept_name() {
  assert(pos_ == start_ && "must emit or ignore before consuming whitespace");
  std::optional<char> c = next();

  // XXX: Just ASCII for now.
  if (c && name_first.contains(c.value())) {
    c = next();
  } else if (c) {
    backup();
    return false;
  }

  while (c && name_char.contains(c.value())) {
    c = next();
  }

  if (c) {
    backup();
  }

  return true;
}

bool Lexer::ignore_whitespace() {

  if (accept_run(Lexer::whitespace)) {
    ignore();
    return true;
  }
  return false;
}

void Lexer::error(std::string_view message) {
  error_ = message;
  tokens_.push_back(Token{TokenType::error, error_, pos_});
}

} // namespace libjsonpath
