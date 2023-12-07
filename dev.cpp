#include "libjsonpath/config.hpp"
#include "libjsonpath/jsonpath.hpp"
#include "libjsonpath/lex.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cout << argv[0] << " Version " << LIBJSONPATH_VERSION_MAJOR << "."
              << LIBJSONPATH_VERSION_MINOR << "." << LIBJSONPATH_VERSION_PATCH
              << std::endl;
    std::cout << "Usage: " << argv[0] << " <path>" << std::endl;
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

  std::cout << libjsonpath::VERSION << std::endl;
}

// #include "libjsonpath/lex.hpp"
// #include <iostream>

// int main(int, char**) {
//   libjsonpath::Lexer lexer{"$[\"\\uD834\\uDD1E\"]"};
//   lexer.run();

//   for (const auto& token : lexer.tokens()) {
//     std::cout << token << std::endl;
//   }

//   auto segments{libjsonpath::parse("$[\"\\uD834\\uDD1E\"]")};
//   std::cout << libjsonpath::to_string(segments) << std::endl;
// }