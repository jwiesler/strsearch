#include "DllEntry.h"
#include "stringsearch/SuffixSort.hpp"
#include "stringsearch/Search.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <algorithm>

using namespace stringsearch;

Logger::~Logger() noexcept {
	try {
		const auto str = stream_.str();
		if(log_)
			log_(str.data());
		else
			std::cout << str << std::endl;
	} catch(...) {}
}

SearchInstance& SearchInstance::fromHandle(const InstanceHandle ptr) {
	return *reinterpret_cast<SearchInstance *>(ptr);
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

InstanceHandle CreateSearchInstanceFromText(const char16_t *charactersBegin, const size_t count, const LogCallback callback) {
	Logger(callback) << "Creating instance";
	const auto text = std::u16string_view(charactersBegin, count);
	ClockDuration createTime;
	const auto ptr = Time(createTime, [&]() {
		return new SearchInstance(text, callback);
	});
	ptr->log() << "Create took " << std::chrono::duration_cast<std::chrono::milliseconds>(createTime).count() << "ms";
	return ptr;
}

void DestroySearchInstance(const InstanceHandle instance) {
	if(instance == nullptr)
		return;
	auto &search = SearchInstance::fromHandle(instance);
	search.log() << "Destroying instance";
	delete &search;
}

Result CountOccurences(const InstanceHandle instance, const char16_t *patternBegin, const size_t count, int *occurrences) {
	if(instance == nullptr)
		return Result::InvalidInstance;
	if(patternBegin == nullptr && occurrences == nullptr)
		return Result::NullPointer;

	const auto &sa = SearchInstance::fromHandle(instance);
	const auto result = sa.search().find(std::u16string_view(patternBegin, count));
	*occurrences = int(std::distance(result.begin(), result.end()));
	return Result::Ok;
}

FindUniqueResult MakeUniqueAndGetItems(const Search &search, const FindResult &searchResult, const Span<Index> outputIndices, const unsigned int offset) {
	const auto res = search.findUnique(searchResult, outputIndices, offset);
	for(auto &index : outputIndices.subspan(0, res.Count))
		index = search.itemsLookup().getItem(index);
	return res;
}

Result FindUniqueItems(const InstanceHandle instance, const char16_t *patternBegin, const size_t count, Index *output, const size_t outputCount, FindUniqueItemsResult *result, const unsigned int offset) {
	if(instance == nullptr)
		return Result::InvalidInstance;
	if(output == nullptr && patternBegin == nullptr)
		return Result::NullPointer;
	
	const auto &search = SearchInstance::fromHandle(instance);
	const auto pattern = std::u16string_view(patternBegin, count);

	const auto outputIndices = Span<Index>(output, outputCount);

	ClockDuration searchTime;
	const auto searchResult = Time(searchTime, [&]() {
		return search.search().find(pattern);
	});
	
	search.log() << "Search took " << searchTime.count() << "ns";
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cv;
	search.log() << "Found " << searchResult.size() << " occurrences for " << std::quoted(cv.to_bytes(pattern.data(), pattern.data() + pattern.size())) << ", skipping " << offset;
	if(searchResult.size() < size_t(offset))
		return Result::OffsetOutOfBounds;
	
	ClockDuration uniqueTime;
	const auto uniqueResult = Time(uniqueTime, [&]() {
		return MakeUniqueAndGetItems(search.search(), searchResult, outputIndices, offset);
	});
	
	search.log() << "Unique took " << uniqueTime.count() << "ns";
	
	if(result)
		*result = FindUniqueItemsResult{searchResult.size(), uniqueResult.Count, uniqueResult.Consumed};
	return Result::Ok;
}

std::vector<std::u16string_view> ParseKeywords(const std::u16string_view pattern) {
	std::vector<std::u16string_view> keywords;
	auto it = pattern.begin();
	while(it != pattern.end()) {
		if(*it == u' ')
			continue;
		const auto endIt = std::find(it, pattern.end(), u' ');
		keywords.emplace_back(pattern.substr(std::distance(pattern.begin(), it), std::distance(it, endIt)));
		it = endIt;
		if(endIt == pattern.end())
			break;
		++it;
	}
	return keywords;
}

Result FindUniqueItemsKeywords(const InstanceHandle instance, const char16_t *patternBegin, const size_t count,
										Index * const output, const size_t outputCount, FindUniqueItemsResult * const result) {
	if(instance == nullptr)
		return Result::InvalidInstance;
	if(output == nullptr && patternBegin == nullptr)
		return Result::NullPointer;

	const auto &search = SearchInstance::fromHandle(instance);
	const auto pattern = std::u16string_view(patternBegin, count);
	const auto outputIndices = Span<Index>(output, outputCount);

	ClockDuration fullDuration;
	ClockDuration parseDuration;
	ClockDuration findDuration;
	ClockDuration uniqueDuration;
	const auto searchResult = Time(fullDuration, [&]() {
		const auto keywords = Time(parseDuration, [&]() {
			return ParseKeywords(pattern);
		});

		const auto findResults = Time(findDuration, [&]() {
			std::vector<FindResult> results;
			for(const auto &k : keywords)
				results.emplace_back(search.search().find(k));
			return results;
		});

		if(findResults.size() == 1) {
			const auto &findResult = findResults[0];
			const auto res = MakeUniqueAndGetItems(search.search(), findResult, outputIndices, 0);
			return FindUniqueInAllResult(findResult.size(), res.Count);
		}
		
		const auto res = Time(uniqueDuration, [&]() {
			return search.search().findUniqueInAllOf(findResults, outputIndices);
		});
		return res;
	});

	search.log() << "Keyword search took " << std::chrono::duration_cast<std::chrono::microseconds>(fullDuration).
		count() << "us "
		"(parse keywords: " << parseDuration.count() << "ns"
		", find keywords: " << findDuration.count() << "ns"
		", unique: " << uniqueDuration.count() << "ns)";

	if(result)
		*result = FindUniqueItemsResult{searchResult.TotalCount, searchResult.Count, 0};
	return Result::Ok;
}
