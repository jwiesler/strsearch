#define CATCH_CONFIG_MAIN

#include "stringsearch/Search.hpp"

#include <catch2/catch.hpp>
#include <numeric>
#include "stringsearch/SuffixSort.hpp"

using namespace std::literals;
constexpr char16_t TestCharArray[] = { u'A', u'\0', u'B', u'B', u'\0', u'C', u'C', u'C', u'\0', u'D', u'D', u'\0', u'E', u'\0'};
constexpr auto TestCharCount = std::size(TestCharArray);
constexpr auto TestString = std::u16string_view(TestCharArray, TestCharCount);

using namespace stringsearch;

//       0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| 11| 12| 13
// SA = 13 | 1 | 4 | 8 | 11| 0 | 3 | 2 | 7 | 6 | 5 | 10| 9 | 12
constexpr std::array<Index, TestCharCount> TestSuffixArray = { 13, 1, 4, 8, 11, 0, 3, 2, 7, 6, 5, 10, 9, 12 };

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
void CollectionsEqual(IteratorABegin &&abegin, IteratorAEnd &&aend, Utf16TextIterator bbegin, Utf16TextIterator bend, const char *start) {
	size_t offset = 0;
	for(; abegin != aend; ++abegin, ++bbegin, ++offset) {
		INFO("At offset " << offset << ", Utf16TextIterator is at offset " << std::distance(start, bbegin.get()));
		CHECK(bbegin != bend);
		CHECK(*abegin == *bbegin);
	}
	CHECK(bbegin == bend);
}

TEST_CASE("utf16 is iterated correctly", "[Utf16TextIterator]") {
	const char16_t str[] = { 0x0054, 0x0950 };
	const auto *cstr = reinterpret_cast<const char *>(str);
	
	SECTION("basic assumptions") {
		REQUIRE(str[0] == 0x0054);
		REQUIRE(str[1] == 0x0950);
		
		CHECK(cstr[0] == 0x54);
		CHECK(cstr[1] == 0x00);
		CHECK(cstr[2] == 0x50);
		CHECK(cstr[3] == 0x09);
	}
	
	SECTION("small sample from the start") {
		Utf16TextIterator it(str);
		Utf16TextIterator end(str + 2);
		std::array<char, 4> result {{ 0, 'T', 0x9, 0x50 }};
		CollectionsEqual(result.begin(), result.end(), it, end, cstr);
	}

	SECTION("small sample from offset position") {
		Utf16TextIterator it(reinterpret_cast<const char *>(str));
		Utf16TextIterator end(str + 2);
		std::array<char, 3> result {{ 'T', 0x9, 0x50 }};
		CollectionsEqual(result.begin(), result.end(), it, end, cstr);
	}
}

TEST_CASE("sort works", "[SuffixSort]") {
	std::array<Index, TestCharCount> indices{};
	std::iota(indices.begin(), indices.end(), Index(0));

	SECTION("SuffixSortOwnBuffer") {
		SuffixSortOwnBuffer(TestString.data(), indices);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortSharedBuffer") {
		SuffixSortSharedBuffer(TestString.data(), indices);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortInPlace") {
		SuffixSortInPlace(TestString.data(), indices);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}
	
	SECTION("SuffixSortStd") {
		SuffixSortStd(TestString.data(), indices);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortOwnBufferMax") {
		SuffixSortOwnBufferMax(TestString.data(), indices, 2);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortSharedBufferMax") {
		SuffixSortSharedBufferMax(TestString.data(), indices, 2);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}

	SECTION("SuffixSortInPlaceMax") {
		SuffixSortInPlaceMax(TestString.data(), indices, 2);
		CollectionsEqual(indices.begin(), indices.end(), TestSuffixArray.begin(), TestSuffixArray.end());
	}
}

TEST_CASE("bound", "[SuffixArray]") {
	//       0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| 11| 12| 13
	// SA = 13 | 1 | 4 | 8 | 11| 0 | 3 | 2 | 7 | 6 | 5 | 10| 9 | 12
	const auto array = SuffixArray(TestSuffixArray);

	SECTION("lowerBound") {
		const auto [pattern, expected] = GENERATE(Catch::Generators::table<std::u16string_view, Index>({
			std::make_tuple(u"A"sv, 5),
			std::make_tuple(u"B"sv, 6),
			std::make_tuple(u"C"sv, 8),
			std::make_tuple(u"D"sv, 11),
			std::make_tuple(u"E"sv, 13)
		}));
		
		auto ptr = SuffixArray::lowerBound(array.begin(), array.end(), TestString, pattern);
		REQUIRE(std::distance(array.begin(), ptr) == expected);
	}

	SECTION("upperBound") {
		const auto [pattern, expected] = GENERATE(Catch::Generators::table<std::u16string_view, Index>({
			std::make_tuple(u"A"sv, 6),
			std::make_tuple(u"B"sv, 8),
			std::make_tuple(u"C"sv, 11),
			std::make_tuple(u"D"sv, 13),
			std::make_tuple(u"E"sv, 14)
		}));
		
		auto ptr = SuffixArray::upperBound(array.begin(), array.end(), TestString, pattern);
		REQUIRE(std::distance(array.begin(), ptr) == expected);
	}
}

TEST_CASE("OldUniqueSearchLookup findUnique", "[OldUniqueSearchLookup]") {
	const auto array = SuffixArray(TestSuffixArray);
	const OldUniqueSearchLookup oldSearch(TestString);

	SECTION("full") {
		const auto [pattern, consumed, foundCount, resultIndex] = GENERATE(Catch::Generators::table<std::u16string_view, size_t, size_t, Index>({
			std::make_tuple(u"A"sv, 1, 1, 0),
			std::make_tuple(u"B"sv, 2, 1, 2),
			std::make_tuple(u"C"sv, 3, 1, 5),
			std::make_tuple(u"D"sv, 2, 1, 9),
			std::make_tuple(u"E"sv, 1, 1, 12)
		}));
		
		std::array<Index, 10> output{};
		const auto findResult = array.find(TestString, pattern);
		INFO("Found " << findResult.size() << " items");
		const auto res = oldSearch.findUnique(findResult, output);
		REQUIRE(res.Consumed == consumed);
		REQUIRE(res.Count == foundCount);
		REQUIRE(resultIndex == output[0]);
	}

	SECTION("partial") {
		const auto [pattern, partialConsumed, partialFound] = GENERATE(Catch::Generators::table<std::u16string_view, size_t, size_t>({
			std::make_tuple(u"A"sv, 1, 1),
			std::make_tuple(u"B"sv, 1, 1),
			std::make_tuple(u"C"sv, 1, 1),
			std::make_tuple(u"D"sv, 1, 1),
			std::make_tuple(u"E"sv, 1, 1)
		}));
		
		std::array<Index, 1> output{};
		const auto findResult = array.find(TestString, pattern);
		INFO("Found " << findResult.size() << " items");
		const auto res = oldSearch.findUnique(findResult, output);
		REQUIRE(res.Consumed == partialConsumed);
		REQUIRE(res.Count == partialFound);
	}

	SECTION("partial-repeated") {
		const auto [pattern, partialConsumed, partialFound] = std::make_tuple(u"C"sv, 3, 1);
		
		std::array<Index, 2> output{};
		const auto findResult = array.find(TestString, pattern);
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
		const auto [pattern, consumed, foundCount, resultIndex] = GENERATE(Catch::Generators::table<std::u16string_view, size_t, size_t, Index>({
			std::make_tuple(u"A"sv, 1, 1, 0),
			std::make_tuple(u"B"sv, 2, 1, 3),
			std::make_tuple(u"C"sv, 3, 1, 7),
			std::make_tuple(u"D"sv, 2, 1, 10),
			std::make_tuple(u"E"sv, 1, 1, 12)
		}));
		
		std::array<Index, 10> output{};
		const auto result = array.find(TestString, pattern);
		INFO("Found " << result.size() << " items");
		const auto res = search.findUnique(result, output);
		REQUIRE(res.Consumed == consumed);
		REQUIRE(res.Count == foundCount);
		REQUIRE(resultIndex == output[0]);
	}

	SECTION("partial") {
		const auto [pattern, partialConsumed, partialFound] = GENERATE(Catch::Generators::table<std::u16string_view, size_t, size_t>({
			std::make_tuple(u"A"sv, 1, 1),
			std::make_tuple(u"B"sv, 2, 1),
			std::make_tuple(u"C"sv, 3, 1),
			std::make_tuple(u"D"sv, 2, 1),
			std::make_tuple(u"E"sv, 1, 1)
		}));
		
		std::array<Index, 1> output{};
		const auto result = array.find(TestString, pattern);
		INFO("Found " << result.size() << " items");
		const auto res = search.findUnique(result, output);
		REQUIRE(res.Consumed == partialConsumed);
		REQUIRE(res.Count == partialFound);
	}

	SECTION("partial-repeated") {
		const auto [pattern, partialConsumed, partialFound] = std::make_tuple(u"C"sv, 3, 1);
		
		std::array<Index, 2> output{};
		const auto result = array.find(TestString, pattern);
		INFO("Found " << result.size() << " items");
		const auto res = search.findUnique(result, output);
		REQUIRE(res.Consumed == partialConsumed);
		REQUIRE(res.Count == partialFound);
	}
}

TEST_CASE("iterate", "[UniqueItemsIterator]") {
	const auto array = SuffixArray(TestSuffixArray);
	const UniqueSearchLookup lookup(TestString, array);
	const UniqueItemsIteratorEnd end;

	const auto [beginOffset, initialOffset, endOffset, indices] = GENERATE(values({
		std::make_tuple(0, 0, TestCharCount, std::vector<Index>{{ 13, 1, 4, 8, 11 }}),
		std::make_tuple(5, 5, TestCharCount - 3, std::vector<Index>{{ 0, 3, 7 }}), // (0), (3, 2), (7, 6, 5)
		std::make_tuple(5, 7, TestCharCount - 3, std::vector<Index>{{ 7 }}), // (0), (3, >2), (7, 6, 5)
	}));
	
	UniqueItemsIterator it(FindResult(array.begin() + beginOffset, array.begin() + endOffset), array.begin() + initialOffset, lookup);
	for(auto iit = indices.begin(); iit != indices.end(); ++iit, ++it) {
		INFO("Iterator is at offset " << beginOffset + it.offsetFromResultBegin());
		REQUIRE(!it.isDuplicate());
		REQUIRE(it != end);
		REQUIRE(*it == *iit);
	}
}