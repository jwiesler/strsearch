#pragma once
#include <span>
#include "Definitions.hpp"

namespace stringsearch {
	void SuffixSortStd(const wchar_t *characters, std::span<Index> sa);
	
	void SuffixSortSharedBuffer(const wchar_t *characters, std::span<Index> sa);

	void SuffixSortOwnBuffer(const wchar_t *characters, std::span<Index> sa);

	void SuffixSortInPlace(const wchar_t *characters, std::span<Index> sa);
	
	void SuffixSortSharedBufferMax(const wchar_t *characters, std::span<Index> sa, size_t max = 80);

	void SuffixSortOwnBufferMax(const wchar_t *characters, std::span<Index> sa, size_t max);

	void SuffixSortInPlaceMax(const wchar_t *characters, std::span<Index> sa, size_t max = 80);
}
