#define CATCH_CONFIG_MAIN

#include "stringsearch/Search.hpp"

#include <catch2/catch.hpp>
#include <numeric>

using namespace std::literals;
constexpr wchar_t TestCharArray[] = { L'A', L'\0', L'B', L'B', L'\0', L'C', L'C', L'C', L'\0', L'D', L'D', L'\0', L'E', L'\0'};
constexpr auto TestString = std::wstring_view(TestCharArray, std::size(TestCharArray));

using namespace stringsearch;
TEST_CASE("getters", "[ItemsArray]") {
	const ItemsArray array(TestString);

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

	SECTION("getItemEnd") {
		REQUIRE(array.itemCount() == 5);
		REQUIRE(array.getItemEnd(0) == 1);
		REQUIRE(array.getItemEnd(1) == 4);
		REQUIRE(array.getItemEnd(2) == 8);
		REQUIRE(array.getItemEnd(3) == 11);
		REQUIRE(array.getItemEnd(4) == 13);
	}
}

TEST_CASE("makeUnique", "[ItemsArray]") {
	const ItemsArray array(TestString);

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

TEST_CASE("bound", "[SuffixArray]") {
	//       0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| 11| 12| 13
	// SA = 13 | 1 | 4 | 8 | 11| 0 | 3 | 2 | 7 | 6 | 5 | 10| 9 | 12
	const SuffixArray array(TestString);

	SECTION("lowerBound") {
		const auto [pattern, expected] = GENERATE(Catch::Generators::table<std::wstring_view, Index>({
			std::make_tuple(L"A"sv, 5),
			std::make_tuple(L"B"sv, 6),
			std::make_tuple(L"C"sv, 8),
			std::make_tuple(L"D"sv, 11),
			std::make_tuple(L"E"sv, 13)
		}));
		
		auto ptr = SuffixArray::lowerBound(array.begin(), array.end(), TestString, pattern);
		REQUIRE(std::distance(array.begin(), ptr) == expected);
	}

	SECTION("upperBound") {
		const auto [pattern, expected] = GENERATE(Catch::Generators::table<std::wstring_view, Index>({
			std::make_tuple(L"A"sv, 6),
			std::make_tuple(L"B"sv, 8),
			std::make_tuple(L"C"sv, 11),
			std::make_tuple(L"D"sv, 13),
			std::make_tuple(L"E"sv, 14)
		}));
		
		auto ptr = SuffixArray::upperBound(array.begin(), array.end(), TestString, pattern);
		REQUIRE(std::distance(array.begin(), ptr) == expected);
	}
}

TEST_CASE("findUnique", "[Search]") {
	const Search array(TestString);

	SECTION("full") {
		const auto [pattern, consumed, foundCount, resultIndex] = GENERATE(Catch::Generators::table<std::wstring_view, size_t, size_t, Index>({
			std::make_tuple(L"A"sv, 1, 1, 0),
			std::make_tuple(L"B"sv, 2, 1, 2),
			std::make_tuple(L"C"sv, 3, 1, 5),
			std::make_tuple(L"D"sv, 2, 1, 9),
			std::make_tuple(L"E"sv, 1, 1, 12)
		}));
		
		std::array<Index, 10> output{};
		auto res = array.findUnique(pattern, output);
		REQUIRE(res.Consumed == consumed);
		REQUIRE(res.Count == foundCount);
		REQUIRE(resultIndex == output[0]);
	}

	SECTION("partial") {
		const auto [pattern, partialConsumed, partialFound] = GENERATE(Catch::Generators::table<std::wstring_view, size_t, size_t>({
			std::make_tuple(L"A"sv, 1, 1),
			std::make_tuple(L"B"sv, 1, 1),
			std::make_tuple(L"C"sv, 1, 1),
			std::make_tuple(L"D"sv, 1, 1),
			std::make_tuple(L"E"sv, 1, 1)
		}));
		
		std::array<Index, 1> output{};
		auto res = array.findUnique(pattern, output);
		REQUIRE(res.Consumed == partialConsumed);
		REQUIRE(res.Count == partialFound);
	}

	SECTION("partial-repeated") {
		const auto [pattern, partialConsumed, partialFound] = std::make_tuple(L"C"sv, 3, 1);
		
		std::array<Index, 2> output{};
		auto res = array.findUnique(pattern, output);
		REQUIRE(res.Consumed == partialConsumed);
		REQUIRE(res.Count == partialFound);
	}
}