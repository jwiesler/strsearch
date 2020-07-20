#pragma once
#include "Definitions.hpp"

#include <string_view>
#include <span>

namespace stringsearch {
	template<typename T>
	std::span<T> MakeSubSpan(const std::span<T> span, const typename std::span<T>::const_iterator begin, const typename std::span<T>::const_iterator end) {
		const auto offset = std::distance(span.begin(), begin);
		const auto count = std::distance(begin, end);
		return span.subspan(offset, count);
	}
	
	class Search {
		std::wstring_view text_;
		std::span<const Index> sa_;
		
	public:
		Search(std::wstring_view text, std::span<const Index> sa)
			: text_(std::move(text)),
				sa_(std::move(sa)) {}

		[[nodiscard]] std::span<const Index> find(std::wstring_view pattern) const;

		[[nodiscard]] std::span<const Index>::const_iterator lowerBound(std::wstring_view pattern) const;
		[[nodiscard]] std::span<const Index>::const_iterator upperBound(std::wstring_view pattern) const;

		[[nodiscard]] std::wstring_view getSuffix(Index index, size_t length) const;
	};
}
