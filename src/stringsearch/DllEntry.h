#pragma once
#include "dllexport.h"
#include "stringsearch/Definitions.hpp"
#include "stringsearch/Search.hpp"
#include <sstream>

enum class Result {
	Ok = 0,
	InvalidInstance = 1,
	NullPointer = 2,
	OffsetOutOfBounds
};

struct FindUniqueItemsResult {
	size_t TotalResults;
	size_t Count;
	size_t Consumed;
};

#define strsearchdll_CALLING_CONVENCTION __cdecl

using LogCallback = void(strsearchdll_CALLING_CONVENCTION *) (const char *message);
using InstanceHandle = void *;

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

class SearchInstance {
	stringsearch::Search search_;
	LogCallback log_;

public:
	SearchInstance(const std::u16string_view text, const LogCallback callback)
		: search_(text),
			log_(callback) {}
	
	DISABLE_COPY(SearchInstance);
	DISABLE_MOVE(SearchInstance);
	
	[[nodiscard]] const stringsearch::Search& search() const { return search_; }

	[[nodiscard]] Logger log() const { return Logger(log_); }

	static SearchInstance &fromHandle(InstanceHandle ptr);
};

extern "C" {
	strsearchdll_EXPORT void strsearchdll_CALLING_CONVENCTION SuffixSortSharedBuffer(const char16_t *characters, stringsearch::Index *saBegin, stringsearch::Index *saEnd);

	strsearchdll_EXPORT void strsearchdll_CALLING_CONVENCTION SuffixSortInPlace(const char16_t *characters, stringsearch::Index *saBegin, stringsearch::Index *saEnd);

	strsearchdll_EXPORT InstanceHandle strsearchdll_CALLING_CONVENCTION CreateSearchInstanceFromText(const char16_t *charactersBegin, size_t count, LogCallback callback);
	
	strsearchdll_EXPORT void strsearchdll_CALLING_CONVENCTION DestroySearchInstance(InstanceHandle instance);

	strsearchdll_EXPORT Result strsearchdll_CALLING_CONVENCTION CountOccurences(InstanceHandle instance, const char16_t *patternBegin, size_t count, int *occurrences);
	
	strsearchdll_EXPORT Result strsearchdll_CALLING_CONVENCTION FindUniqueItems(InstanceHandle instance, const char16_t *patternBegin, size_t count, stringsearch::Index *output, size_t outputCount, FindUniqueItemsResult *result, unsigned int offset);
	
	strsearchdll_EXPORT Result strsearchdll_CALLING_CONVENCTION FindUniqueItemsKeywords(InstanceHandle instance, const char16_t *patternBegin, size_t count,
																													stringsearch::Index *output, size_t outputCount, FindUniqueItemsResult *result);
}
