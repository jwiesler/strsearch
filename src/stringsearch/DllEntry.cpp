#include "DllEntry.h"
#include "stringsearch/SuffixSort.hpp"
#include "stringsearch/Search.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace stringsearch;

Search &Get(const InstanceHandle ptr) {
	return *reinterpret_cast<Search *>(ptr);
}

void SuffixSortSharedBuffer(const wchar_t *characters, int *saBegin, int *saEnd) {
	SuffixSortSharedBufferMax(characters, MakeSpan(saBegin, saEnd));
}

void SuffixSortInPlace(const wchar_t *characters, int *saBegin, int *saEnd) {
	SuffixSortInPlaceMax(characters, MakeSpan(saBegin, saEnd));
}

InstanceHandle CreateSearchInstanceFromText(const wchar_t *charactersBegin, const size_t count) {
	std::wcout << "Creating instance\n";
	const auto text = std::wstring_view(charactersBegin, count);
	return new Search(text);
}

void DestroySearchInstance(const InstanceHandle instance) {
	if(instance == nullptr)
		return;
	std::wcout << "Destroying instance\n";
	delete &Get(instance);
}

Result CountOccurences(const InstanceHandle instance, const wchar_t *patternBegin, const size_t count, int *occurrences) {
	if(instance == nullptr)
		return Result::InvalidInstance;
	if(patternBegin == nullptr && occurrences == nullptr)
		return Result::NullPointer;

	const auto &sa = Get(instance);
	const auto result = sa.find(std::wstring_view(patternBegin, count));
	*occurrences = int(std::distance(result.begin(), result.end()));
	return Result::Ok;
}

Result FindUniqueItems(const InstanceHandle instance, const wchar_t *patternBegin, const size_t count, Index *output, const size_t outputCount, FindUniqueItemsResult *result, const unsigned int offset) {
	if(instance == nullptr)
		return Result::InvalidInstance;
	if(output == nullptr && patternBegin == nullptr)
		return Result::NullPointer;
	
	const auto &sa = Get(instance);
	const auto pattern = std::wstring_view(patternBegin, count);
	
	const auto beforeSearch = std::chrono::high_resolution_clock::now();
	const auto searchResult = sa.find(pattern);
	const auto afterSearch = std::chrono::high_resolution_clock::now();
	
	std::wcout << "Search took " << (afterSearch - beforeSearch).count() << "ns\n";
	std::wcout << "Found " << searchResult.size() << " occurrences for " << std::quoted(pattern) << ", skipping " << offset << '\n';

	if(searchResult.size() < size_t(offset))
		return Result::OffsetOutOfBounds;
	
	const auto beforeUnique = std::chrono::high_resolution_clock::now();
	const auto res = sa.findUnique(searchResult, Span<Index>(output, outputCount), offset);
	for(auto &index : Span<Index>(output, res.Count))
		index = sa.itemsLookup().getItem(index);
	const auto afterUnique = std::chrono::high_resolution_clock::now();
	std::wcout << "Unique took " << (afterUnique - beforeUnique).count() << "ns\n";
	
	if(result)
		*result = FindUniqueItemsResult{searchResult.size(), res.Count, res.Consumed};
	return Result::Ok;
}
