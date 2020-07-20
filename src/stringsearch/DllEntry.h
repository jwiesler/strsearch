#pragma once
#include "dllexport.h"

extern "C" {
	strsearchdll_EXPORT void SuffixSortSharedBuffer(const wchar_t *characters, int *saBegin, int *saEnd);

	strsearchdll_EXPORT void SuffixSortInPlace(const wchar_t *characters, int *saBegin, int *saEnd);

	strsearchdll_EXPORT int Find(const wchar_t *charactersBegin, const wchar_t *charactersEnd, const wchar_t *patternBegin, const wchar_t *patternEnd, const int *saBegin, const int *saEnd);
}
