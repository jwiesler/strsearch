#include "DllEntry.h"
#include "stringsearch/SuffixSort.hpp"
#include "stringsearch/Search.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <locale>
#include <codecvt>

using namespace stringsearch;

Search &Get(const InstanceHandle ptr) {
	return *reinterpret_cast<Search *>(ptr);
}

void SuffixSortSharedBuffer(const char16_t *characters, int *saBegin, int *saEnd) {
	SuffixSortSharedBufferMax(characters, MakeSpan(saBegin, saEnd));
}

void SuffixSortInPlace(const char16_t *characters, int *saBegin, int *saEnd) {
	SuffixSortInPlaceMax(characters, MakeSpan(saBegin, saEnd));
}

using Clock = std::chrono::high_resolution_clock;
using ClockDuration = std::chrono::high_resolution_clock::duration;

template<typename F>
decltype(auto) Time(ClockDuration &duration, F && f) {
	const auto before = Clock::now();
	decltype(auto) res = f();
	const auto after = Clock::now();
	duration = after - before;
	return res;
}

InstanceHandle CreateSearchInstanceFromText(const char16_t *charactersBegin, const size_t count) {
	std::cout << "Creating instance\n";
	const auto text = std::u16string_view(charactersBegin, count);
	ClockDuration createTime;
	const auto ptr = Time(createTime, [&]() {
		return new Search(text);
	});
	std::cout << "Create took " << createTime.count() << "ns\n";
	return ptr;
}

void DestroySearchInstance(const InstanceHandle instance) {
	if(instance == nullptr)
		return;
	std::cout << "Destroying instance\n";
	delete &Get(instance);
}

Result CountOccurences(const InstanceHandle instance, const char16_t *patternBegin, const size_t count, int *occurrences) {
	if(instance == nullptr)
		return Result::InvalidInstance;
	if(patternBegin == nullptr && occurrences == nullptr)
		return Result::NullPointer;

	const auto &sa = Get(instance);
	const auto result = sa.find(std::u16string_view(patternBegin, count));
	*occurrences = int(std::distance(result.begin(), result.end()));
	return Result::Ok;
}

Result FindUniqueItems(const InstanceHandle instance, const char16_t *patternBegin, const size_t count, Index *output, const size_t outputCount, FindUniqueItemsResult *result, const unsigned int offset) {
	if(instance == nullptr)
		return Result::InvalidInstance;
	if(output == nullptr && patternBegin == nullptr)
		return Result::NullPointer;
	
	const auto &sa = Get(instance);
	const auto pattern = std::u16string_view(patternBegin, count);

	std::cout << std::hex << std::showbase;
	for(const auto ch : pattern) {
		std::cout << unsigned(ch) << ' ';
	}
	std::cout << '\n';

	ClockDuration searchTime;
	const auto searchResult = Time(searchTime, [&]() {
		return sa.find(pattern);
	});
	
	std::cout << "Search took " << searchTime.count() << "ns\n";
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cv;
	std::cout << "Found " << searchResult.size() << " occurrences for " << std::quoted(cv.to_bytes(pattern.data(), pattern.data() + pattern.size())) << ", skipping " << offset << '\n';
	if(searchResult.size() < size_t(offset))
		return Result::OffsetOutOfBounds;
	
	ClockDuration uniqueTime;
	const auto uniqueResult = Time(uniqueTime, [&]() {
		const auto res = sa.findUnique(searchResult, Span<Index>(output, outputCount), offset);
		for(auto &index : Span<Index>(output, res.Count))
			index = sa.itemsLookup().getItem(index);
		return res;
	});
	
	std::cout << "Unique took " << uniqueTime.count() << "ns\n";
	
	if(result)
		*result = FindUniqueItemsResult{searchResult.size(), uniqueResult.Count, uniqueResult.Consumed};
	return Result::Ok;
}
