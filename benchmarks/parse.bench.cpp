#include "libjsonpath/parse.hpp"
#include "benchmark/benchmark.h"
#include "libjsonpath/jsonpath.hpp"

static void BM_ParseShorthand(benchmark::State& state) {
  libjsonpath::Parser parser{};
  for (auto _ : state) {
    parser.parse("$.foo.bar");
  }
}

static void BM_ParseBracketed(benchmark::State& state) {
  libjsonpath::Parser parser{};
  for (auto _ : state) {
    parser.parse("$['foo']['bar']");
  }
}

static void BM_ConstructParser(benchmark::State& state) {
  for (auto _ : state) {
    libjsonpath::Parser parser{};
    parser.parse("$['foo']['bar']");
  }
}

static void BM_ConstructParserWithFuncs(benchmark::State& state) {
  for (auto _ : state) {
    libjsonpath::Parser parser{libjsonpath::DEFAULT_FUNCTION_EXTENSIONS};
    parser.parse("$['foo']['bar']");
  }
}

BENCHMARK(BM_ConstructParser);
BENCHMARK(BM_ConstructParserWithFuncs);
BENCHMARK(BM_ParseShorthand);
BENCHMARK(BM_ParseBracketed);

BENCHMARK_MAIN();
