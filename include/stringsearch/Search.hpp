#pragma once
#include "Definitions.hpp"

#include <string_view>
#include <vector>

namespace stringsearch {
	template<typename T, typename Iterator>
	Span<T> MakeSubSpan(const Span<T> span, const Iterator begin, const Iterator end) {
		const auto offset = std::distance(span.begin(), begin);
		const auto count = std::distance(begin, end);
		return span.subspan(offset, count);
	}

	class ItemsArray {
		std::vector<Index> items_;
		std::vector<Index> itemEnds_;
		
	public:
		explicit ItemsArray(std::wstring_view text);

		[[nodiscard]] Index getItem(Index suffix) const;
		[[nodiscard]] Index getItemEnd(Index item) const;

		[[nodiscard]] Span<Index>::iterator makeUniqueMerge(const Span<const Index>::iterator begin,
																			const Span<const Index>::iterator secondHalfStart,
																			const Span<const Index>::iterator end,
																			Span<Index> target) const;

		[[nodiscard]] Span<Index>::iterator makeUniqueFull(Span<Index>::iterator begin, Span<Index>::iterator end) const;

		[[nodiscard]] size_t itemCount() const { return itemEnds_.size(); }
	};

	[[nodiscard]] std::wstring_view GetSuffix(std::wstring_view text, Index index, size_t length);
	
	class SuffixArray {
		std::vector<Index> sa_;

		using IndexPtr = std::vector<Index>::const_iterator;
		
	public:
		explicit SuffixArray(std::wstring_view text);

		[[nodiscard]] Span<const Index> find(std::wstring_view text, std::wstring_view pattern) const;

		[[nodiscard]] static IndexPtr lowerBound(IndexPtr begin, IndexPtr end, std::wstring_view text, std::wstring_view pattern);
		[[nodiscard]] static IndexPtr upperBound(IndexPtr begin, IndexPtr end, std::wstring_view text, std::wstring_view pattern);

		[[nodiscard]] std::vector<Index>::const_iterator begin() const noexcept { return sa_.begin(); }
		[[nodiscard]] std::vector<Index>::const_iterator end() const noexcept { return sa_.end(); }
	};

	class Search {
		SuffixArray suffixArray_;
		ItemsArray items_;
		std::wstring_view text_;

	public:
		explicit Search(std::wstring_view text);

		[[nodiscard]] Span<const Index> find(std::wstring_view pattern) const;

		struct FindUniqueResult {
			size_t Count;
			size_t Consumed;
		};

		[[nodiscard]] FindUniqueResult findUnique(std::wstring_view pattern, Span<Index> outputIndices) const;
		[[nodiscard]] FindUniqueResult findUnique(Span<const Index>::iterator begin, Span<const Index>::iterator end, Span<Index> outputIndices) const;
		[[nodiscard]] FindUniqueResult findUnique(Span<const Index>::iterator begin, Span<const Index>::iterator end, Span<Index> outputIndices, Span<Index> buffer) const;
	};
}
