#include "libjsonpath/lex.h"
#include <iostream>

int main(int, char**) {
  libjsonpath::Lexer lexer{"$[?true==false]"};
  lexer.run();

  for (const auto& token : lexer.tokens()) {
    std::cout << token << "\n";
  }
}