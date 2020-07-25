#include "DllEntry.h"
#include "stringsearch/SuffixSort.hpp"
#include "stringsearch/Search.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <locale>
#include <codecvt>

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

Result FindUniqueItems(const InstanceHandle instance, const char16_t *patternBegin, const size_t count, Index *output, const size_t outputCount, FindUniqueItemsResult *result, const unsigned int offset) {
	if(instance == nullptr)
		return Result::InvalidInstance;
	if(output == nullptr && patternBegin == nullptr)
		return Result::NullPointer;
	
	const auto &search = SearchInstance::fromHandle(instance);
	const auto pattern = std::u16string_view(patternBegin, count);

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
		const auto res = search.search().findUnique(searchResult, Span<Index>(output, outputCount), offset);
		for(auto &index : Span<Index>(output, res.Count))
			index = search.search().itemsLookup().getItem(index);
		return res;
	});
	
	search.log() << "Unique took " << uniqueTime.count() << "ns";
	
	if(result)
		*result = FindUniqueItemsResult{searchResult.size(), uniqueResult.Count, uniqueResult.Consumed};
	return Result::Ok;
}
