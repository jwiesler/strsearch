#pragma once
#include "Definitions.hpp"

namespace stringsearch {
	template<typename T>
	const T *BeginPtr(const std::basic_string_view<T> view) {
		return view.data();
	}

	template<typename T>
	const T *EndPtr(const std::basic_string_view<T> view) {
		return view.data() + view.size();
	}
	
	void SuffixSortStd(std::u16string_view characters, Span<Index> sa);
	
	void SuffixSortSharedBuffer(std::u16string_view characters, Span<Index> sa);

	void SuffixSortOwnBuffer(std::u16string_view characters, Span<Index> sa);

	void SuffixSortInPlace(std::u16string_view characters, Span<Index> sa);
	
	void SuffixSortSharedBufferMax(std::u16string_view characters, Span<Index> sa, size_t max = 80);

	void SuffixSortOwnBufferMax(std::u16string_view characters, Span<Index> sa, size_t max = 80);

	void SuffixSortInPlaceMax(std::u16string_view characters, Span<Index> sa, size_t max = 80);
}
