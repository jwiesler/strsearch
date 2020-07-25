#pragma once
#include "dllexport.h"
#include "stringsearch/Definitions.hpp"

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

extern "C" {
	using InstanceHandle = void *;
	
	strsearchdll_EXPORT void strsearchdll_CALLING_CONVENCTION SuffixSortSharedBuffer(const char16_t *characters, stringsearch::Index *saBegin, stringsearch::Index *saEnd);

	strsearchdll_EXPORT void strsearchdll_CALLING_CONVENCTION SuffixSortInPlace(const char16_t *characters, stringsearch::Index *saBegin, stringsearch::Index *saEnd);

	strsearchdll_EXPORT InstanceHandle strsearchdll_CALLING_CONVENCTION CreateSearchInstanceFromText(const char16_t *charactersBegin, size_t count);
	
	strsearchdll_EXPORT void strsearchdll_CALLING_CONVENCTION DestroySearchInstance(InstanceHandle instance);

	strsearchdll_EXPORT Result strsearchdll_CALLING_CONVENCTION CountOccurences(InstanceHandle instance, const char16_t *patternBegin, size_t count, int *occurrences);
	
	strsearchdll_EXPORT Result strsearchdll_CALLING_CONVENCTION FindUniqueItems(InstanceHandle instance, const char16_t *patternBegin, size_t count, stringsearch::Index *output, size_t outputCount, FindUniqueItemsResult *result, unsigned int offset);
}
