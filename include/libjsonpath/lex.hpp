#ifndef LIBJSONPATH_LEX_H
#define LIBJSONPATH_LEX_H

#include "libjsonpath/tokens.hpp"
#include <deque>    // std::deque
#include <optional> // std::optional
#include <stack>    // std::stack
#include <string> // std::string std::string::size_type std::string_literals std::to_string
#include <unordered_set> // std::unordered_set

namespace libjsonpath {

class Lexer {
public:
  Lexer(std::string_view query);

  // Start the state machine.
  void run();

  // Tokens generated by the lexer after calling _run()_.
  const std::deque<Token>& tokens() const noexcept { return m_tokens; };

  // The error message produced by _run()_, or an empty string if there
  // was no error.
  const std::string& error_message() const noexcept { return m_error; };

private:
  enum State {
    ERROR,
    NONE,
    LEX_ROOT,
    LEX_SEGMENT,
    LEX_DESCENDANT_SELECTION,
    LEX_DOT_SELECTOR,
    LEX_INSIDE_BRACKETED_SELECTION,
    LEX_INSIDE_FILTER,
    LEX_INSIDE_SINGLE_QUOTED_STRING,
    LEX_INSIDE_DOUBLE_QUOTED_STRING,
    LEX_INSIDE_SINGLE_QUOTED_FILTER_STRING,
    LEX_INSIDE_DOUBLE_QUOTED_FILTER_STRING,
  };

  const std::string_view m_query{};
  const std::string::size_type m_length{};
  std::string m_error{};
  std::deque<Token> m_tokens{};

  // A JSONPath filter expression can contain _filter queries_, which
  // are fully-formed JSONPath queries relative to the current JSON
  // node or the document root. So, considering that JSONPath queries
  // can be arbitrarily nested in this way, we must keep track of the
  // number of nested filter selectors in order to yield control back
  // to the appropriate lexer state function.
  int m_filter_nesting_level{0};

  // A running count of parentheses for each, possibly nested, filter
  // function call. If the stack is empty, we are not in a filter
  // function call. Remember that function arguments can use arbitrarily
  // nested parentheses.
  std::stack<int> m_paren_stack{};

  // Index of the start of the current token being scanned.
  std::string::size_type m_start{0};

  // Index of the character currently being scanned.
  std::string::size_type m_pos{0};

  // The set of characters that are considered to be whitespace.
  static inline const std::unordered_set<char> s_whitespace{
      ' ', '\n', '\t', '\r'};

  // The set of characters that are allowed to follow a '\' to form
  // and escape sequence in a string literal.
  static inline const std::unordered_set<char> s_escapes{
      'b', 'f', 'n', 'r', 't', 'u', '/'};

  static inline const std::unordered_set<char> s_digits{
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

  static inline const std::unordered_set<char> s_sign{'+', '-'};

  static inline const std::unordered_set<char> s_function_name_first{'a', 'b',
      'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
      'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

  static inline const std::unordered_set<char> s_function_name_char{'0', '1',
      '2', '3', '4', '5', '6', '7', '8', '9', '_', 'a', 'b', 'c', 'd', 'e', 'f',
      'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u',
      'v', 'w', 'x', 'y', 'z'};

  // Return the next character from the query string, wrapped in an
  // optional object, and advance the current position. If the optional
  // object is empty, we have reached the end of the query string.
  std::optional<char> next();

  // Return the next character from the query string - wrapped in an
  // optional object - without advancing _pos_. If the optional object
  // is empty, we have reached the end of the query string.
  std::optional<char> peek();

  // Push a new token of type _t_ and value between _start_ and _pos_
  // to the token list.
  void emit(TokenType t);

  bool accept(const char ch);
  bool accept(const std::unordered_set<char>& valid);
  bool accept_run(const std::unordered_set<char>& valid);

  // Advance the lexer if the next run of characters is a valid name.
  bool accept_name();

  // Advance the lexer if the next character is valid for the first character
  // in a JSONPath name.
  bool accept_name_first();

  // Advance the lexer if the next character is valid for a JSONPath name.
  bool accept_name_char();

  // Advance the lexer if the next character is a valid UTF-8 continuation
  // byte, or throw a LexerError if it's not.
  void accept_continuation_byte();

  // Return a string view of _query_ starting from the current position.
  std::string_view view() const;

  void backup();            // Go back one character, if _pos_ > _start_.
  void ignore();            // Consume characters between _start_ and _pos_.
  bool ignore_whitespace(); // Consume whitespace characters from _start_.

  void error(std::string_view message); // Emit an error token.

  // Lexer state functions, each of which emit tokens and return the next
  // state.
  State lex_root();
  State lex_segment();
  State lex_descendant_selection();
  State lex_dot_selector();
  State lex_inside_bracketed_selection();
  State lex_inside_filter();

  // Scan for a string literal surrounded by _quote_, emitting a
  // _token_type_ token type and returning _next_state_.
  template <State next_state, char quote, TokenType tt>
  State lex_inside_string() {
    using namespace std::string_literals;
    ignore(); // Discard the opening quote.

    // Empty string?
    if (peek().value_or(' ') == quote) {
      emit(tt);
      next(); // Discard the closing quote.
      ignore();
      return next_state;
    }

    std::string_view v;
    std::optional<char> c;
    const std::string escaped_quote{'\\', quote};

    while (true) {
      v = view();
      c = next();

      if (v.rfind("\\\\", 0) == 0 || v.rfind(escaped_quote, 0) == 0) {
        next();
        continue;
      }

      if (c.value_or(' ') == '\\' &&
          !(s_escapes.find(peek().value_or(' ')) != s_escapes.end())) {
        error("invalid escape sequence '\\'"s + peek().value_or(' '));
        return ERROR;
      }

      if (!c) {
        error("unclosed string starting at index "s + std::to_string(m_start));
        return ERROR;
      }

      if (c.value_or(' ') == quote) {
        backup();
        emit(tt);
        next(); // Discard the closing quote.
        ignore();
        return next_state;
      }
    }
  }
};

} // namespace libjsonpath

#endif