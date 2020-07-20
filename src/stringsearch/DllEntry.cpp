#include "DllEntry.h"
#include "stringsearch/SuffixSort.hpp"
#include "stringsearch/Search.hpp"

void SuffixSortSharedBuffer(const wchar_t *characters, int *saBegin, int *saEnd) {
	stringsearch::SuffixSortSharedBufferMax(characters, std::span<int>(saBegin, saEnd));
}

void SuffixSortInPlace(const wchar_t *characters, int *saBegin, int *saEnd) {
	stringsearch::SuffixSortInPlaceMax(characters, std::span<int>(saBegin, saEnd));
}

constexpr std::wstring_view MakeView(const wchar_t *begin, const wchar_t *end) {
	return std::wstring_view(begin, std::distance(begin, end));
}

int Find(const wchar_t *charactersBegin, const wchar_t *charactersEnd, const wchar_t *patternBegin,
	const wchar_t *patternEnd, const int *saBegin, const int *saEnd) {
	const auto text = MakeView(charactersBegin, charactersEnd);
	const auto pattern = MakeView(patternBegin, patternEnd);
	const auto sa = std::span(saBegin, saEnd);
	const stringsearch::Search search(text, sa);
	return search.find(pattern).size();
}
