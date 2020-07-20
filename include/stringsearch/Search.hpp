#pragma once
#include "Definitions.hpp"

#include <string_view>

namespace stringsearch {
	template<typename T>
	Span<T> MakeSubSpan(const Span<T> span, const typename Span<T>::iterator begin, const typename Span<T>::iterator end) {
		const auto offset = std::distance(span.begin(), begin);
		const auto count = std::distance(begin, end);
		return span.subspan(offset, count);
	}
	
	class Search {
		std::wstring_view text_;
		Span<const Index> sa_;
		
	public:
		Search(std::wstring_view text, Span<const Index> sa)
			: text_(std::move(text)),
				sa_(std::move(sa)) {}

		[[nodiscard]] Span<const Index> find(std::wstring_view pattern) const;

		[[nodiscard]] Span<const Index>::iterator lowerBound(std::wstring_view pattern) const;
		[[nodiscard]] Span<const Index>::iterator upperBound(std::wstring_view pattern) const;

		[[nodiscard]] std::wstring_view getSuffix(Index index, size_t length) const;
	};
}
