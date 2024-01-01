# libjsonpath

A JSONPath parser written in C++, targeting C++17.

This project is a work in progress. So far we have a lexer producing a `std::deque<Token>`, and a parser that parses those tokens into a `std::vector<std::variant<Segment, RecursiveSegment>>`. When a segment includes a filter selector, that filter selector's `expression` member is effectively the root of a parse tree for the filter expression. See `include/libjsonpath/selectors.hpp` for a description of segment, selector and filter expression nodes.

This example parses a JSONPath query string from the command line and prints a canonical representation of the resulting structure.

```cpp
#include "libjsonpath/jsonpath.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <path>" << std::endl;
    std::exit(1);
  }

  auto path{libjsonpath::parse(argv[1])};
  std::cout << libjsonpath::to_string(path) << std::endl;
  return 0;
}
```

Given the query `$.foo.bar[?@.some > $.thing]`, we get the following printed to the terminal.

```plain
$['foo']['bar'][?@['some'] > $['thing']]
```

## Build and run benchmarks

Benchmarks are excluded from the `ALL` target and should be built in "Release" mode.

```
$ mkdir build_benchmarks
$ cd build_benchmarks
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ cmake --build . --config Release --target lexer_benchmarks --target parser_benchmarks
$ ./lexer_benchmark
$ ./parser_benchmark
```
