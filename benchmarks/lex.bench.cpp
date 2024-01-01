#include "libjsonpath/lex.hpp"
#include "benchmark/benchmark.h"

static void BM_LexShorthand(benchmark::State& state) {
  for (auto _ : state) {
    libjsonpath::Lexer lexer{"$.foo.bar"};
    lexer.run();
  }
}
// Register the function as a benchmark
BENCHMARK(BM_LexShorthand);

static void BM_LexBracketed(benchmark::State& state) {
  for (auto _ : state) {
    libjsonpath::Lexer lexer{"$['foo']['bar']"};
    lexer.run();
  }
}
// Register the function as a benchmark
BENCHMARK(BM_LexBracketed);

BENCHMARK_MAIN();
