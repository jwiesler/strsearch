#define CATCH_CONFIG_MAIN

#pragma warning(push)
#pragma warning(disable: 26812)
#pragma warning(disable: 6319)

#include <catch2/catch.hpp>

#include "stringsearch/Search.hpp"
#include "stringsearch/SuffixSort.hpp"
#include "stringsearch/Utf16Le.hpp"

#include <numeric>

using namespace std::literals;

constexpr char16_t ToLittleEndian(const char16_t ch) noexcept {
	return ((ch << 8) & 0xFF00) | (ch >> 8);
}

template<size_t Size, typename T, typename Sequence>
constexpr std::array<T, Size> ToLittleEndianSequence(const Sequence &seq) noexcept {
	std::array<T, Size> res{};
	for(size_t i = 0; i < Size; ++i)
		res[i] = ToLittleEndian(seq[i]);

	return res;
}

template<typename T, typename... Tys>
constexpr std::array<T, sizeof...(Tys)> MakeArray(Tys &&... tys) {
	return {{ static_cast<T>(std::forward<Tys>(tys))... }};
}

template<typename T, size_t Size>
constexpr std::basic_string_view<T> ToStringView(const std::array<T, Size> &seq) {
	return std::u16string_view(seq.data(), Size);
}

constexpr auto TestCharArray = MakeArray<char16_t>(u'A', u'\0', u'B', u'B', u'\0', u'C', u'C', u'C', u'\0', u'D', u'D', u'\0', u'E', u'\0');
constexpr auto TestString = ToStringView(TestCharArray);

using namespace stringsearch;

//       0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| 11| 12| 13
// SA = 13 | 1 | 4 | 8 | 11| 0 | 3 | 2 | 7 | 6 | 5 | 10| 9 | 12
constexpr auto TestSuffixArray = MakeArray<Index>(13, 1, 4, 8, 11, 0, 3, 2, 7, 6, 5, 10, 9, 12);

TEST_CASE("ItemsLookup getters work", "[ItemsLookup]") {
	const ItemsLookup array(TestString);

	SECTION("getItem") {
		REQUIRE(array.getItem(0) == 0);
		REQUIRE(array.getItem(1) == 0);
		REQUIRE(array.getItem(2) == 1);
		REQUIRE(array.getItem(3) == 1);
		REQUIRE(array.getItem(4) == 1);
		REQUIRE(array.getItem(5) == 2);
		REQUIRE(array.getItem(6) == 2);
		REQUIRE(array.getItem(7) == 2);
		REQUIRE(array.getItem(8) == 2);
		REQUIRE(array.getItem(9) == 3);
		REQUIRE(array.getItem(10) == 3);
		REQUIRE(array.getItem(11) == 3);
		REQUIRE(array.getItem(12) == 4);
		REQUIRE(array.getItem(13) == 4);
	}

	SECTION("itemCount") {
		REQUIRE(array.itemCount() == 5);
	}
}

TEST_CASE("OldUniqueSearchLookup getters work", "[OldUniqueSearchLookup]") {
	const OldUniqueSearchLookup array(TestString);

	SECTION("itemCount") {
		REQUIRE(array.itemCount() == 5);
	}
	
	SECTION("getItemEnd") {	
		REQUIRE(array.getItemEnd(0) == 1);
		REQUIRE(array.getItemEnd(1) == 4);
		REQUIRE(array.getItemEnd(2) == 8);
		REQUIRE(array.getItemEnd(3) == 11);
		REQUIRE(array.getItemEnd(4) == 13);
	}
}

TEST_CASE("makeUnique", "[OldUniqueSearchLookup]") {
	const OldUniqueSearchLookup array(TestString);

	SECTION("makeUniqueFull") {
		std::array<Index, 9> indicesArray {
			0, 2, 3, 5, 6, 7, 9, 10, 12
		};
		Span<Index> indices = indicesArray;
		
		auto it = array.makeUniqueFull(indices.begin(), indices.end());
		REQUIRE(std::distance(indices.begin(), it) == 5);
		REQUIRE(indices[0] == 0);
		REQUIRE(indices[1] == 2);
		REQUIRE(indices[2] == 5);
		REQUIRE(indices[3] == 9);
		REQUIRE(indices[4] == 12);
	}

	SECTION("makeUniqueMerge") {
		std::array<Index, 11> indicesArray {
			0, 2, 7, 9, 12, 3, 6, 7, 9, 10, 12
		};
		std::array<Index, 11> outputArray{};
		Span<const Index> indices = indicesArray;

		Span<Index> output = outputArray;
		auto it = array.makeUniqueMerge(indices.begin(), indices.begin() + 5, indices.end(), output);
		REQUIRE(std::distance(output.begin(), it) == 5);
		REQUIRE(output[0] == 0);
		REQUIRE(output[1] == 2);
		REQUIRE(output[2] == 6);
		REQUIRE(output[3] == 9);
		REQUIRE(output[4] == 12);
	}
}

template<typename IteratorABegin, typename IteratorAEnd, typename IteratorBBegin, typename IteratorBEnd>
void CollectionsEqual(IteratorABegin &&abegin, IteratorAEnd &&aend, IteratorBBegin &&bbegin, IteratorBEnd &&bend) {
	size_t offset = 0;
	for(; abegin != aend; ++abegin, ++bbegin, ++offset) {
		INFO("At offset " << offset);
		CHECK(bbegin != bend);
		CHECK(*abegin == *bbegin);
	}
	CHECK(bbegin == bend);
}

template<typename IteratorABegin, typename IteratorAEnd>
void CollectionsEqual(IteratorABegin &&abegin, IteratorAEnd &&aend, Utf16LETextIterator bbegin, Utf16LETextIterator bend, const unsigned char *start) {
	size_t offset = 0;
	for(; abegin != aend; ++abegin, ++bbegin, ++offset) {
		INFO("At offset " << offset << ", Utf16LETextIterator is at offset " << std::distance(start, bbegin.get()));
		CHECK(bbegin != bend);
		CHECK(*abegin == *bbegin);
	}
	CHECK(bbegin == bend);
}

TEST_CASE("utf16le is compared correctly", "[LessThanUtf16Le]") {
	CHECK(LessThanUtf16Le(0x0001, 0x0002));
	CHECK_FALSE(LessThanUtf16Le(0x0002, 0x0001));
	
	CHECK(LessThanUtf16Le(0x0100, 0x0200));
	CHECK_FALSE(LessThanUtf16Le(0x0200, 0x0100));
	
	CHECK(LessThanUtf16Le(0x0100, 0x0001));
	CHECK_FALSE(LessThanUtf16Le(0x0001, 0x0100));
}

TEST_CASE("utf16le is iterated correctly", "[Utf16LETextIterator]") {
	const char16_t str[] = { 0x0054, 0x0950 };
	const auto *cstr = reinterpret_cast<const unsigned char *>(str);

	SECTION("basic assumptions") {
		CHECK(str[0] == 0x0054);
		CHECK(str[1] == 0x0950);
		
		CHECK(cstr[0] == 0x54);
		CHECK(cstr[1] == 0x00);
		CHECK(cstr[2] == 0x50);
		CHECK(cstr[3] == 0x09);
	}
	
	SECTION("small sample from the start") {
		Utf16LETextIterator it(str);
		Utf16LETextIterator end(str + 2);
		const auto result = MakeArray<char>(0, 'T', 0x9, 0x50);
		CollectionsEqual(result.begin(), result.end(), it, end, cstr);
	}

	SECTION("small sample from offset position") {
		Utf16LETextIterator it(reinterpret_cast<const unsigned char *>(str));
		Utf16LETextIterator end(str + 2);
		const auto result = MakeArray<char>('T', 0x9, 0x50);
		CollectionsEqual(result.begin(), result.end(), it, end, cstr);
	}

	SECTION("small sample from offset position to offset end") {
		Utf16LETextIterator it(reinterpret_cast<const unsigned char *>(str));
		Utf16LETextIterator end(reinterpret_cast<const unsigned char *>(str + 1));
		const auto result = MakeArray<char>('T', 0x9);
		CollectionsEqual(result.begin(), result.end(), it, end, cstr);
	}
}

TEST_CASE("ordering strings", "[Utf16LETextIterator]") {
	constexpr auto compare = [](const auto &a, const auto &b) {
		const auto av = ToStringView(a);
		const auto bv = ToStringView(b);
		return std::lexicographical_compare(
			Utf16LETextIterator(BeginPtr(av)), Utf16LETextIterator(EndPtr(av)),
			Utf16LETextIterator(BeginPtr(bv)), Utf16LETextIterator(EndPtr(bv))
		);
	};

	SECTION("empty") {
		constexpr auto e = MakeArray<char16_t>();
		constexpr auto a = MakeArray<char16_t>(u'A');
		CHECK(compare(e, a));
		CHECK_FALSE(compare(a, e));
	}
	
	SECTION("simple ascii") {
		constexpr auto aa = MakeArray<char16_t>(u'A', u'A');
		constexpr auto ab = MakeArray<char16_t>(u'A', u'B');
		constexpr auto ba = MakeArray<char16_t>(u'B', u'A');
		constexpr auto bb = MakeArray<char16_t>(u'B', u'B');
		// aa < ab < ba < bb
		CHECK(compare(aa, ab));
		CHECK(compare(aa, ba));
		CHECK(compare(aa, bb));
		CHECK(compare(ab, ba));
		CHECK(compare(ab, bb));
		CHECK(compare(ba, bb));

		CHECK_FALSE(compare(ab, aa));
		CHECK_FALSE(compare(ba, aa));
		CHECK_FALSE(compare(bb, aa));
		CHECK_FALSE(compare(ba, ab));
		CHECK_FALSE(compare(bb, ab));
		CHECK_FALSE(compare(bb, ba));
	}

	SECTION("utf16") {
		constexpr auto aa = MakeArray<char16_t>(0x0001);
		constexpr auto ab = MakeArray<char16_t>(u'A');
		// aa < ab
		CHECK(compare(aa, ab));
		CHECK_FALSE(compare(ab, aa));
	}

	SECTION("suffixarray") {
		CHECK(std::is_sorted(TestSuffixArray.begin(), TestSuffixArray.end(), [](const auto a, const auto b) {
			return LessThan(GetSuffix(TestString, a, TestString.size() - a), GetSuffix(TestString, b, TestString.size() - b));
		}));
	}
}

TEST_CASE("sort works", "[SuffixSort]") {
	std::array<Index, TestString.size()> indices{};
	std::iota(indices.begin(), indices.end(), Index(0));

	SECTION("SuffixSortOwnBuffer") {
		SuffixSortOwnBuffer(TestString, indices);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortSharedBuffer") {
		SuffixSortSharedBuffer(TestString, indices);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortInPlace") {
		SuffixSortInPlace(TestString, indices);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}
	
	SECTION("SuffixSortStd") {
		SuffixSortStd(TestString, indices);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortOwnBufferMax") {
		SuffixSortOwnBufferMax(TestString, indices, 2);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortSharedBufferMax") {
		SuffixSortSharedBufferMax(TestString, indices, 2);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortInPlaceMax") {
		SuffixSortInPlaceMax(TestString, indices, 2);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortOwnBufferMax with SuffixSortStd") {
		SuffixSortOwnBufferMax(TestString, indices, 4);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortSharedBufferMax with SuffixSortStd") {
		SuffixSortSharedBufferMax(TestString, indices, 4);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortInPlaceMax with SuffixSortStd") {
		SuffixSortInPlaceMax(TestString, indices, 4);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}
}

constexpr auto Av = u"A"sv;
constexpr auto Bv = u"B"sv;
constexpr auto Cv = u"C"sv;
constexpr auto Dv = u"D"sv;
constexpr auto Ev = u"E"sv;

constexpr auto Ar = std::make_pair(5, 6);
constexpr auto Br = std::make_pair(6, 8);
constexpr auto Cr = std::make_pair(8, 11);
constexpr auto Dr = std::make_pair(11, 13);
constexpr auto Er = std::make_pair(13, 14);

TEST_CASE("bound", "[SuffixArray]") {
	//       0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| 11| 12| 13
	// SA = 13 | 1 | 4 | 8 | 11| 0 | 3 | 2 | 7 | 6 | 5 | 10| 9 | 12
	const auto array = SuffixArray(TestSuffixArray);

	constexpr auto p = Cv;
	auto b = LessThan(GetSuffix(TestString, 13, p.size()), GetSuffix(TestString, 1, p.size()));
	auto c = LessThan(p, GetSuffix(TestString, 10, p.size()));
	auto d = LessThan(GetSuffix(TestString, 11, p.size()), p);
	auto e = LessThan(p, GetSuffix(TestString, 11, p.size()));
	
	const auto [pattern, expected] = GENERATE(Catch::Generators::table<std::u16string_view, std::pair<Index, Index>>({
		std::make_tuple(Av, Ar),
		std::make_tuple(Bv, Br),
		std::make_tuple(Cv, Cr),
		std::make_tuple(Dv, Dr),
		std::make_tuple(Ev, Er)
	}));

	SECTION("lowerBound") {
		auto ptr = SuffixArray::lowerBound(array.begin(), array.end(), TestString, pattern);
		REQUIRE(expected.first == std::distance(array.begin(), ptr));
	}

	SECTION("upperBound") {
		auto ptr = SuffixArray::upperBound(array.begin(), array.end(), TestString, pattern);
		REQUIRE(expected.second == std::distance(array.begin(), ptr));
	}
}

TEST_CASE("OldUniqueSearchLookup findUnique", "[OldUniqueSearchLookup]") {
	const auto array = SuffixArray(TestSuffixArray);
	const OldUniqueSearchLookup oldSearch(TestString);

	SECTION("full") {
		const auto [result, consumed, foundCount, resultIndex] = GENERATE(Catch::Generators::table<std::pair<Index, Index>, size_t, size_t, Index>({
			std::make_tuple(Ar, 1, 1, 0),
			std::make_tuple(Br, 2, 1, 2),
			std::make_tuple(Cr, 3, 1, 5),
			std::make_tuple(Dr, 2, 1, 9),
			std::make_tuple(Er, 1, 1, 12)
		}));
		
		std::array<Index, 10> output{};
		const auto findResult = FindResult(array.begin() + result.first, array.begin() + result.second);
		INFO("Found " << findResult.size() << " items");
		const auto res = oldSearch.findUnique(findResult, output);
		REQUIRE(res.Consumed == consumed);
		REQUIRE(res.Count == foundCount);
		REQUIRE(resultIndex == output[0]);
	}

	SECTION("partial") {
		const auto [result, partialConsumed, partialFound] = GENERATE(Catch::Generators::table<std::pair<Index, Index>, size_t, size_t>({
			std::make_tuple(Ar, 1, 1),
			std::make_tuple(Br, 1, 1),
			std::make_tuple(Cr, 1, 1),
			std::make_tuple(Dr, 1, 1),
			std::make_tuple(Er, 1, 1)
		}));
		
		std::array<Index, 1> output{};
		const auto findResult = FindResult(array.begin() + result.first, array.begin() + result.second);
		INFO("Found " << findResult.size() << " items");
		const auto res = oldSearch.findUnique(findResult, output);
		REQUIRE(res.Consumed == partialConsumed);
		REQUIRE(res.Count == partialFound);
	}

	SECTION("partial-repeated") {
		const auto [result, partialConsumed, partialFound] = std::make_tuple(Cr, 3, 1);
		
		std::array<Index, 2> output{};
		const auto findResult = FindResult(array.begin() + result.first, array.begin() + result.second);
		INFO("Found " << findResult.size() << " items");
		const auto res = oldSearch.findUnique(findResult, output);
		REQUIRE(res.Consumed == partialConsumed);
		REQUIRE(res.Count == partialFound);
	}
}

TEST_CASE("UniqueSearchLookup findUnique", "[UniqueSearchLookup]") {
	const auto array = SuffixArray(TestSuffixArray);
	const UniqueSearchLookup search(TestString, array);

	SECTION("full") {
		const auto [result, consumed, foundCount, resultIndex] = GENERATE(Catch::Generators::table<std::pair<Index, Index>, size_t, size_t, Index>({
			std::make_tuple(Ar, 1, 1, 0),
			std::make_tuple(Br, 2, 1, 3),
			std::make_tuple(Cr, 3, 1, 7),
			std::make_tuple(Dr, 2, 1, 10),
			std::make_tuple(Er, 1, 1, 12)
		}));
		
		std::array<Index, 10> output{};
		const auto findResult = FindResult(array.begin() + result.first, array.begin() + result.second);
		INFO("Found " << findResult.size() << " items");
		const auto res = search.findUnique(findResult, output);
		REQUIRE(res.Consumed == consumed);
		REQUIRE(res.Count == foundCount);
		REQUIRE(resultIndex == output[0]);
	}

	SECTION("partial") {
		const auto [result, partialConsumed, partialFound] = GENERATE(Catch::Generators::table<std::pair<Index, Index>, size_t, size_t>({
			std::make_tuple(Ar, 1, 1),
			std::make_tuple(Br, 2, 1),
			std::make_tuple(Cr, 3, 1),
			std::make_tuple(Dr, 2, 1),
			std::make_tuple(Er, 1, 1)
		}));
		
		std::array<Index, 1> output{};
		const auto findResult = FindResult(array.begin() + result.first, array.begin() + result.second);
		INFO("Found " << findResult.size() << " items");
		const auto res = search.findUnique(findResult, output);
		REQUIRE(res.Consumed == partialConsumed);
		REQUIRE(res.Count == partialFound);
	}

	SECTION("partial-repeated") {
		const auto [result, partialConsumed, partialFound] = std::make_tuple(Cr, 3, 1);
		
		std::array<Index, 2> output{};
		const auto findResult = FindResult(array.begin() + result.first, array.begin() + result.second);
		INFO("Found " << findResult.size() << " items");
		const auto res = search.findUnique(findResult, output);
		REQUIRE(res.Consumed == partialConsumed);
		REQUIRE(res.Count == partialFound);
	}
}

TEST_CASE("iterate", "[UniqueItemsIterator]") {
	const auto array = SuffixArray(TestSuffixArray);
	const UniqueSearchLookup lookup(TestString, array);
	const UniqueItemsIteratorEnd end;

	const auto [beginOffset, initialOffset, endOffset, indices] = GENERATE(values({
		std::make_tuple(0, 0, TestString.size(), std::vector<Index>{{ 13, 1, 4, 8, 11 }}),
		std::make_tuple(5, 5, TestString.size() - 3, std::vector<Index>{{ 0, 3, 7 }}), // (0), (3, 2), (7, 6, 5)
		std::make_tuple(5, 7, TestString.size() - 3, std::vector<Index>{{ 7 }}), // (0), (3, >2), (7, 6, 5)
	}));
	
	UniqueItemsIterator it(FindResult(array.begin() + beginOffset, array.begin() + endOffset), array.begin() + initialOffset, lookup);
	for(auto iit = indices.begin(); iit != indices.end(); ++iit, ++it) {
		INFO("Iterator is at offset " << beginOffset + it.offsetFromResultBegin());
		REQUIRE(!it.isDuplicate());
		REQUIRE(it != end);
		REQUIRE(*it == *iit);
	}
}

#pragma warning(pop)