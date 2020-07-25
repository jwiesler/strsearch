#include <benchmark/benchmark.h>
#include "stringsearch/SuffixSort.hpp"
#include "stringsearch/Search.hpp"
#include <numeric>
#include <fstream>
#include <random>

static_assert(sizeof(wchar_t) == sizeof(char16_t));

static std::u16string CharactersFromFile(const char *name, size_t count = std::numeric_limits<size_t>::max()) {
	std::wifstream stream(name);
	std::wstring str;
	std::wstring res;
	while(std::getline(stream, str) && count--) {
		res += str;
		res += wchar_t(0);
	}
	return {res.begin(), res.end()};
}

static std::u16string CharactersFromStrings(const stringsearch::Span<const std::u16string_view> strs) {
	std::u16string res;
	for (auto& str : strs) {
		res += str;
		res += char16_t(0);
	}
	return res;
}

#ifdef BM_RADIX_SORT
template<typename Function>
static void TestWithCharacters(benchmark::State &state, const std::u16string_view &characters, Function &&function) {
	std::vector<int> sa(characters.size());
	std::iota(sa.begin(), sa.end(), 0);

	for(auto _ : state) {
		function(std::as_const(characters), sa);
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

constexpr std::array<std::u16string_view, 3> StrsSample {{u"Hallo", u"schmallo", u"test"}};

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
	TestWithCharacters(state, CharactersFromFile("strings"), std::forward<Function>(function));
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
#endif

#ifdef BM_SUFFIX_ARRAY_FIND
static void BenchmarkSAFindWithCharacters(benchmark::State &state, const std::u16string_view &characters) {
	const stringsearch::SuffixArray sa(characters);
	
	std::mt19937 gen(42);  // NOLINT(cert-msc32-c)
	const std::uniform_int_distribution<stringsearch::Index> sizeDistribution(1, 6);
	const std::uniform_int_distribution<stringsearch::Index> offsetDistribution(0, characters.size() - 6);
	
	for(auto _ : state) {
		state.PauseTiming();
		const auto offset = offsetDistribution(gen);
		const auto size = sizeDistribution(gen);
		const auto pattern = characters.substr(offset, size);
		state.ResumeTiming();
		benchmark::DoNotOptimize(sa.find(characters, pattern));
	}
}

static void BenchmarkSAFind(benchmark::State &state) {
	BenchmarkSAFindWithCharacters(state, CharactersFromFile("strings"));
}

BENCHMARK(BenchmarkSAFind);
#endif

#ifdef BM_UNIQUE
template<typename Function>
static void BenchmarkUniqueWithCharacters(benchmark::State &state, const stringsearch::SuffixArray &sa, const std::u16string_view characters, Function && function) {
	std::mt19937 gen(42);  // NOLINT(cert-msc32-c)
	const auto upper = state.range(0);
	const std::uniform_int_distribution<stringsearch::Index> sizeDistribution(1, upper);
	const std::uniform_int_distribution<stringsearch::Index> offsetDistribution(0, characters.size() - 6);
	stringsearch::UniqueSearchLookup lookup(characters, sa);
	stringsearch::OldUniqueSearchLookup oldUniqueSearchLookup(characters);
	std::vector<stringsearch::Index> output(characters.size());
	for(auto _ : state) {
		state.PauseTiming();
		const auto offset = offsetDistribution(gen);
		const auto size = sizeDistribution(gen);
		const auto pattern = characters.substr(offset, size);
		const auto range = sa.find(characters, pattern);
		state.ResumeTiming();
		benchmark::DoNotOptimize(function(range, output));
	}
}

static void BenchmarkUnique(benchmark::State &state) {
	const auto characters = CharactersFromFile("strings");
	const stringsearch::SuffixArray sa(characters);
	stringsearch::UniqueSearchLookup lookup(characters, sa);
	BenchmarkUniqueWithCharacters(state, sa, characters, [&](auto && result, auto && output) {
		return lookup.findUnique(result, output);
	});
}

static void BenchmarkUniqueOld(benchmark::State &state) {
	const auto characters = CharactersFromFile("strings");
	const stringsearch::SuffixArray sa(characters);
	stringsearch::OldUniqueSearchLookup lookup(characters);
	std::vector<stringsearch::Index> buffer(characters.size());
	BenchmarkUniqueWithCharacters(state, sa, characters, [&](auto && result, auto && output) {
		return lookup.findUnique(result, output, buffer);
	});
}

BENCHMARK(BenchmarkUnique)->DenseRange(1, 6);
BENCHMARK(BenchmarkUniqueOld)->DenseRange(1, 6);
#endif

// Run the benchmark
BENCHMARK_MAIN();
