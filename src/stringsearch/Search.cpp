#include "stringsearch/Search.hpp"

#include <algorithm>

namespace stringsearch {
	Span<const Index> Search::find(const std::wstring_view pattern) const {
		const auto lower = lowerBound(pattern);
		const auto upper = upperBound(pattern);
		return MakeSubSpan(sa_, lower, upper);
	}

	Span<const Index>::iterator Search::lowerBound(const std::wstring_view pattern) const {
		return std::lower_bound(sa_.begin(), sa_.end(), Index(0), [&](const Index &index, auto) {
			const auto suffix = getSuffix(index, pattern.size());
			return std::lexicographical_compare(suffix.begin(), suffix.end(), pattern.begin(), pattern.end());
		});
	}

	Span<const Index>::iterator Search::upperBound(const std::wstring_view pattern) const {
		return std::upper_bound(sa_.begin(), sa_.end(), Index(0), [&](auto, const Index &index) {
			const auto suffix = getSuffix(index, pattern.size());
			return std::lexicographical_compare(pattern.begin(), pattern.end(), suffix.begin(), suffix.end());
		});
	}

	std::wstring_view Search::getSuffix(const Index index, const size_t length) const {
		return text_.substr(index, length);
	}
}
