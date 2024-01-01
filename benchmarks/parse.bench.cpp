#include "libjsonpath/parse.hpp"
#include "benchmark/benchmark.h"
#include "libjsonpath/jsonpath.hpp"

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

static void BM_ParseFilter(benchmark::State& state) {
  libjsonpath::Parser parser{};
  for (auto _ : state) {
    parser.parse("$[?@.a > 2]");
  }
}

static void BM_ParseFunction(benchmark::State& state) {
  libjsonpath::Parser parser{};
  for (auto _ : state) {
    parser.parse("$[?count(@..*)>2]");
  }
}

BENCHMARK(BM_ConstructParser);
BENCHMARK(BM_ConstructParserWithFuncs);
BENCHMARK(BM_ParseShorthand);
BENCHMARK(BM_ParseBracketed);
BENCHMARK(BM_ParseFilter);
BENCHMARK(BM_ParseFunction);

BENCHMARK_MAIN();
