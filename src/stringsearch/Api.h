#pragma once
#include "ApiDefinitions.h"
#include "stringsearch/Definitions.hpp"

#include "dllexport.h"

extern "C" {
	strsearchdll_EXPORT void strsearchdll_CALLING_CONVENCTION SuffixSortSharedBuffer(
		const char16_t *characters, stringsearch::Index *saBegin, stringsearch::Index *saEnd);

	strsearchdll_EXPORT void strsearchdll_CALLING_CONVENCTION SuffixSortInPlace(
		const char16_t *characters, stringsearch::Index *saBegin, stringsearch::Index *saEnd);

	strsearchdll_EXPORT stringsearch::api::InstanceHandle strsearchdll_CALLING_CONVENCTION CreateSearchInstanceFromText(
		const char16_t *charactersBegin, size_t count, stringsearch::api::LogCallback callback);

	strsearchdll_EXPORT void strsearchdll_CALLING_CONVENCTION DestroySearchInstance(
		stringsearch::api::InstanceHandle instance);

	strsearchdll_EXPORT stringsearch::api::Result strsearchdll_CALLING_CONVENCTION CountOccurences(
		stringsearch::api::InstanceHandle instance, const char16_t *patternBegin, size_t count, int *occurrences);

	strsearchdll_EXPORT stringsearch::api::Result strsearchdll_CALLING_CONVENCTION FindUniqueItems(
		stringsearch::api::InstanceHandle instance, const char16_t *patternBegin, size_t count,
		stringsearch::Index *output, size_t outputCount, stringsearch::api::FindUniqueItemsResult *result,
		unsigned int offset, stringsearch::api::FindUniqueItemsTimings *timings);

	strsearchdll_EXPORT stringsearch::api::Result strsearchdll_CALLING_CONVENCTION FindUniqueItemsKeywords(
		stringsearch::api::InstanceHandle instance, const char16_t *patternBegin, size_t count,
		stringsearch::Index *output, size_t outputCount, stringsearch::api::KeywordsMatch matching, unsigned int offset, stringsearch::api::FindUniqueItemsResult *result, stringsearch::api::FindUniqueItemsKeywordsTimings *timings);
}
