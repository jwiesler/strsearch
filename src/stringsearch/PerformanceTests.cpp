#include <benchmark/benchmark.h>
#include <numeric>
#include <fstream>
#include <span>
#include "stringsearch/SuffixSort.hpp"
#include <array>

static std::wstring CharactersFromFile(const char *name, size_t count) {
	std::wifstream stream(name);
	std::wstring str;
	std::wstring res;
	while(std::getline(stream, str) && count--) {
		res += str;
		res += wchar_t(0);
	}
	return res;
}

static std::wstring CharactersFromStrings(const std::span<const std::wstring_view> strs) {
	std::wstring res;
	for (auto& str : strs) {
		res += str;
		res += wchar_t(0);
	}
	return res;
}

template<typename Function>
static void TestWithCharacters(benchmark::State &state, const std::wstring &characters, Function &&function) {
	std::vector<int> sa(characters.size());
	std::iota(sa.begin(), sa.end(), 0);

	for(auto _ : state) {
		function(std::as_const(characters).data(), sa);
		state.PauseTiming();
		benchmark::ClobberMemory();
		state.ResumeTiming();
	}
}

#define BM_SAMPLE(samplefn, fn) static void BM_##fn##samplefn(benchmark::State &state) {\
	samplefn(state, [](auto &&... vals) {\
		stringsearch::fn(std::forward<decltype(vals)>(vals)...);\
	});\
}\
BENCHMARK(BM_##fn##samplefn)

constexpr std::array<std::wstring_view, 3> StrsSample {{L"Hallo", L"schmallo", L"test"}};

template<typename Function>
static void TestWithTinySample(benchmark::State &state, Function &&function) {
	TestWithCharacters(state, CharactersFromStrings(StrsSample), std::forward<Function>(function));
}

#define BM_SMALL_TINY(name) BM_SAMPLE(TestWithTinySample, name)

BM_SMALL_TINY(SuffixSortStd);
BM_SMALL_TINY(SuffixSortInPlace);
BM_SMALL_TINY(SuffixSortOwnBuffer);
BM_SMALL_TINY(SuffixSortSharedBuffer);

#undef BM_SMALL_TINY

template<typename Function>
static void TestWithSmallSample(benchmark::State &state, Function &&function) {
	TestWithCharacters(state, CharactersFromFile("strings", 20), std::forward<Function>(function));
}

#define BM_SMALL_SAMPLE(name) BM_SAMPLE(TestWithSmallSample, name)

BM_SMALL_SAMPLE(SuffixSortStd);
BM_SMALL_SAMPLE(SuffixSortInPlace);
BM_SMALL_SAMPLE(SuffixSortOwnBuffer);
BM_SMALL_SAMPLE(SuffixSortSharedBuffer);

#undef BM_SMALL_SAMPLE

template<typename Function>
static void TestWithBigSample(benchmark::State &state, Function &&function) {
	TestWithCharacters(state, CharactersFromFile("strings", std::numeric_limits<size_t>::max()), std::forward<Function>(function));
}

#define BM_BIG_SAMPLE(name) BM_SAMPLE(TestWithBigSample, name)

BM_BIG_SAMPLE(SuffixSortStd);
BM_BIG_SAMPLE(SuffixSortInPlace);
BM_BIG_SAMPLE(SuffixSortOwnBuffer);
BM_BIG_SAMPLE(SuffixSortSharedBuffer);

#undef BM_BIG_SAMPLE

template<typename Function>
static void TestWithBigSampleMaxSizes(benchmark::State &state, Function &&function) {
	const auto max = state.range(0);
	TestWithBigSample(state, [&](auto &&... vals) {
		function(std::forward<decltype(vals)>(vals)..., max);
	});
}

BM_SAMPLE(TestWithBigSampleMaxSizes, SuffixSortOwnBufferMax)->DenseRange(10, 150, 10);
BM_SAMPLE(TestWithBigSampleMaxSizes, SuffixSortSharedBufferMax)->DenseRange(10, 150, 10);
BM_SAMPLE(TestWithBigSampleMaxSizes, SuffixSortInPlaceMax)->DenseRange(10, 150, 10);

// Run the benchmark
BENCHMARK_MAIN();
