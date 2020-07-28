#include "Api.h"
#include "ApiFunction.h"
#include "stringsearch/SuffixSort.hpp"
#include "stringsearch/Search.hpp"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <sstream>
#include "MappingIterator.h"
#include "ApiDefinitions.h"

using namespace stringsearch;
using namespace api;

class Logger {
	const LogCallback log_;
	std::stringstream stream_;

public:
	explicit Logger(const LogCallback log) noexcept : log_(log) {}
	~Logger() noexcept;

	DISABLE_COPY(Logger);
	DISABLE_MOVE(Logger);

	template<typename T>
	Logger& operator<<(const T &t) {
		stream_ << t;
		return *this;
	}
};

Logger::~Logger() noexcept {
	try {
		const auto str = stream_.str();
		if(log_)
			log_(str.data());
		else
			std::cout << str << std::endl;
	} catch(...) {}
}

class SearchInstance {
	Search search_;
	LogCallback log_;

public:
	SearchInstance(const std::u16string_view text, const LogCallback callback)
		: search_(text),
			log_(callback) {}
	
	DISABLE_COPY(SearchInstance);
	DISABLE_MOVE(SearchInstance);
	
	[[nodiscard]] const Search& search() const { return search_; }

	[[nodiscard]] Logger log() const { return Logger(log_); }

	static SearchInstance &fromHandle(const InstanceHandle ptr) {
		return *reinterpret_cast<SearchInstance *>(ptr);
	}
};

namespace stringsearch::api {
	template<>
	struct APIArg<SearchInstance> {
		static constexpr size_t argc = 1;
		static Result validate(const InstanceHandle instance) noexcept {
			return instance != nullptr ? Result::Ok : Result::InvalidInstance;
		}

		static SearchInstance& convert(const InstanceHandle instance) noexcept {
			return SearchInstance::fromHandle(instance);
		}
	};
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

template<typename F>
decltype(auto) Time(TimeDuration &duration, F && f) {
	const auto before = Clock::now();
	decltype(auto) res = f();
	const auto after = Clock::now();
	duration = (after - before).count();
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

#define FORWARD_EVERYTHING_LAMBDA(func) [](auto &&... args) -> decltype(auto) { return func(std::forward<decltype(args)>(args)...); }

void DestroyInstanceImpl(const SearchInstance &search) {
	search.log() << "Destroying instance";
	delete &search;
}

void DestroySearchInstance(const InstanceHandle instance) {
	CallApiFunctionImplementation<decltype(DestroyInstanceImpl)>(FORWARD_EVERYTHING_LAMBDA(DestroyInstanceImpl), std::forward_as_tuple(instance));
}

Result CountOccurencesImpl(const SearchInstance &search, const std::u16string_view pattern, int *occurrences) {
	const auto result = search.search().find(pattern);
	*occurrences = int(std::distance(result.begin(), result.end()));
	return Result::Ok;
}

Result CountOccurences(const InstanceHandle instance, const char16_t *patternBegin, const size_t count, int *occurrences) {
	return CallApiFunctionImplementation<decltype(CountOccurencesImpl)>(
		FORWARD_EVERYTHING_LAMBDA(CountOccurencesImpl), 
		std::forward_as_tuple(instance, patternBegin, count, occurrences)
	);
}

FindUniqueResult MakeUniqueAndGetItems(const Search &search, const FindResult &searchResult, const Span<Index> outputIndices, const unsigned int offset) {
	const auto res = search.itemsLookup().findUnique(searchResult, outputIndices, offset);
	for(auto &index : outputIndices.subspan(0, res.Count))
		index = search.itemsLookup().getItem(index);
	return res;
}

Result FindUniqueItemsInternal(const SearchInstance &search, const std::u16string_view pattern, Span<Index> outputIndices, FindUniqueItemsResult &result, const unsigned int offset, FindUniqueItemsTimings &timings) {
	const auto searchResult = Time(timings.Find, [&]() {
		return search.search().find(pattern);
	});
	
	if(searchResult.size() < size_t(offset))
		return Result::OffsetOutOfBounds;
	
	const auto uniqueResult = Time(timings.Unique, [&]() {
		return MakeUniqueAndGetItems(search.search(), searchResult, outputIndices, offset);
	});
	result = FindUniqueItemsResult{searchResult.size(), uniqueResult.Count, uniqueResult.Consumed};

	return Result::Ok;
}

Result FindUniqueItemsImpl(const SearchInstance &search, const std::u16string_view pattern, Span<Index> outputIndices, FindUniqueItemsResult *resultOut, const unsigned int offset, FindUniqueItemsTimings *timingsOut) {
	FindUniqueItemsTimings timings{};
	FindUniqueItemsResult result{};
	const auto res = FindUniqueItemsInternal(search, pattern, outputIndices, result, offset, timings);
	
	if(resultOut)
		*resultOut = result;
	if(timingsOut)
		*timingsOut = timings;
	return res;
}

Result FindUniqueItems(const InstanceHandle instance, const char16_t *patternBegin, const size_t count, Index *output, const size_t outputCount, FindUniqueItemsResult *result, const unsigned int offset, FindUniqueItemsTimings *timings) {
	return CallApiFunctionImplementation<decltype(FindUniqueItemsImpl)>(
		FORWARD_EVERYTHING_LAMBDA(FindUniqueItemsImpl), 
		std::forward_as_tuple(instance, patternBegin, count, output, outputCount, result, offset, timings)
	);
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



Result FindUniqueItemsKeywordsStrategy(const SearchInstance &search, const std::u16string_view pattern, const Span<Index> outputIndices, KeywordsMatch matchingStrategy, unsigned int offset, FindUniqueItemsResult *resultOut, FindUniqueItemsKeywordsTimings *timingsOut) {
	FindUniqueItemsKeywordsTimings timings{};
	FindUniqueItemsResult result{};
	
	const auto keywords = Time(timings.Parse, [&]() {
		return ParseKeywords(pattern);
	});

	Result r;
	if(keywords.size() == 1) {
		r = FindUniqueItemsInternal(search, keywords[0], outputIndices, result, offset, timings);
	} else {
		const auto findResults = Time(timings.Find, [&]() {
			std::vector<FindResult> results;
			for(const auto &k : keywords)
				results.emplace_back(search.search().find(k));
			return results;
		});

		r = Time(timings.Unique, [&]() {
			if(matchingStrategy == KeywordsMatch::All) {
				auto searchResult = search.search().itemsLookup().findUniqueInAllPatterns(findResults);
				if(searchResult.size() < offset)
					return Result::OffsetOutOfBounds;
				const auto skippedResults = Span<Index>(searchResult).subspan(offset);
				const auto count = std::min(outputIndices.size(), skippedResults.size());
				std::copy_n(searchResult.begin(), count, outputIndices.begin());
				result = FindUniqueItemsResult{searchResult.size(), count, count};
			} else if(matchingStrategy == KeywordsMatch::AtLeastOne) {
				auto searchResult = search.search().itemsLookup().findUniquePatterns(findResults);
				if(searchResult.size() < offset)
					return Result::OffsetOutOfBounds;
				SortCountDescendingFirstContainedAscending(searchResult);
				const auto skippedResults = Span<std::pair<Index, ContainedInfo>>(searchResult).subspan(offset);
				const auto count = std::min(outputIndices.size(), skippedResults.size());
				std::copy_n(skippedResults.begin(), count, Map(outputIndices.begin(), [](const std::pair<Index, ContainedInfo> p) {
					return p.first;
				}));
				result = FindUniqueItemsResult{searchResult.size(), count, count};
			}

			return Result::Ok;
		});
	}
		
	if(resultOut)
		*resultOut = result;
	if(timingsOut)
		*timingsOut = timings;
	return r;
}

Result FindUniqueItemsKeywords(const InstanceHandle instance, const char16_t *patternBegin, const size_t count,
										Index * const output, const size_t outputCount, KeywordsMatch matching, unsigned int offset, FindUniqueItemsResult * const result, FindUniqueItemsKeywordsTimings *timings) {
	return CallApiFunctionImplementation<decltype(FindUniqueItemsKeywordsStrategy)>(
		FORWARD_EVERYTHING_LAMBDA(FindUniqueItemsKeywordsStrategy), 
		std::forward_as_tuple(instance, patternBegin, count, output, outputCount, matching, offset, result, timings)
	);
}
