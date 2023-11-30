#include "libjsonpath/jsonpath.hpp"
#include "libjsonpath/lex.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <path>" << std::endl;
    std::exit(1);
  }

  // libjsonpath::Lexer lexer{argv[1]};
  // lexer.run();
  // for (const auto& token : lexer.tokens()) {
  //   std::cout << token << std::endl;
  // }

  auto path{libjsonpath::parse(argv[1])};
  // std::cout << "Path length: " << path.size() << std::endl;
  std::cout << libjsonpath::to_string(path) << std::endl;
}

// #include "libjsonpath/lex.hpp"
// #include <iostream>

// int main(int, char**) {
//   libjsonpath::Lexer lexer{"$.foo.bar[1]"};
//   lexer.run();

//   for (const auto& token : lexer.tokens()) {
//     std::cout << token << "\n";
//   }
// }