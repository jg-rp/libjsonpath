# libjsonpath

A JSONPath parser written in C++, targeting C++20.

This project is a work in progress. So far we only have a lexer, producing a `std::deque<Token>`, where `Token` is a struct.

```cpp
struct Token {
  TokenType type;
  std::string_view value;
  std::string::size_type index;
};
```

This example prints one token per line for the JSONPath query `$.foo.bar[1]`.

```cpp
#include "libjsonpath/lex.h"
#include <iostream>

int main(int, char**) {
  libjsonpath::Lexer lexer{"$.foo.bar[1]"};
  lexer.run();

  for (const auto& token : lexer.tokens()) {
    std::cout << token << "\n";
  }
}
```

```plain
Token{type=ROOT, value="$", index=0}
Token{type=NAME, value="foo", index=2}
Token{type=NAME, value="bar", index=6}
Token{type=[, value="[", index=9}
Token{type=INDEX, value="1", index=10}
Token{type=], value="]", index=11}
Token{type=EOF, value="", index=12}
```
