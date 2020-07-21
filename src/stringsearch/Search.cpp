#include "stringsearch/Search.hpp"

#include <algorithm>
#include <numeric>
#include <vector>

#include "stringsearch/SuffixSort.hpp"

namespace stringsearch {
	ItemsArray::ItemsArray(const std::wstring_view text) {
		items_.reserve(text.size());
		
		auto it = text.begin();
		auto index = Index(0);
		for(; it != text.end(); ++it) {
			items_.emplace_back(index);
			if(*it != 0)
				continue;

			itemEnds_.emplace_back(Index(std::distance(text.begin(), it)));
			++index;
		}
	}

	Index ItemsArray::getItem(const Index suffix) const {
		return items_[suffix];
	}

	Index ItemsArray::getItemEnd(const Index item) const {
		return itemEnds_[item];
	}

	SuffixArray::SuffixArray(const std::wstring_view text) : sa_(text.size()) {
		std::iota(sa_.begin(), sa_.end(), Index(0));
		SuffixSortInPlace(text.data(), sa_);
	}

	Span<const Index> SuffixArray::find(const std::wstring_view text, const std::wstring_view pattern) const {
		const auto lower = lowerBound(sa_.begin(), sa_.end(), text, pattern);
		const auto upper = upperBound(lower, sa_.end(), text, pattern);

		const auto offset = std::distance(sa_.begin(), lower);
		const auto count = std::distance(lower, upper);
		return Span<const Index>(sa_).subspan(offset, count);
	}

	SuffixArray::IndexPtr SuffixArray::lowerBound(const IndexPtr begin, const IndexPtr end,
																const std::wstring_view text, const std::wstring_view pattern) {
		return std::lower_bound(begin, end, Index(0), [&](const Index &index, auto) {
			const auto suffix = GetSuffix(text, index, pattern.size());
			return std::lexicographical_compare(suffix.begin(), suffix.end(), pattern.begin(), pattern.end());
		});
	}

	SuffixArray::IndexPtr SuffixArray::upperBound(const IndexPtr begin, const IndexPtr end,
																const std::wstring_view text, const std::wstring_view pattern) {
		return std::upper_bound(begin, end, Index(0), [&](auto, const Index &index) {
			const auto suffix = GetSuffix(text, index, pattern.size());
			return std::lexicographical_compare(pattern.begin(), pattern.end(), suffix.begin(), suffix.end());
		});
	}

	Search::Search(const std::wstring_view text) : suffixArray_(text),
																	items_(text),
																	text_(text) {}

	Span<const Index> Search::find(const std::wstring_view pattern) const {
		return suffixArray_.find(text_, pattern);
	}

	Search::FindUniqueResult Search::findUnique(const std::wstring_view pattern, const Span<Index> outputIndices) const {
		const auto range = find(pattern);
		return findUnique(range.begin(), range.end(), outputIndices);
	}

	Search::FindUniqueResult Search::findUnique(const Span<const Index>::iterator begin,
																const Span<const Index>::iterator end,
																const Span<Index> outputIndices) const {
		std::vector<Index> buffer(outputIndices.size());
		return findUnique(begin, end, outputIndices, buffer);
	}

	Search::FindUniqueResult Search::findUnique(const Span<const Index>::iterator begin,
																const Span<const Index>::iterator end, const Span<Index> outputIndices,
																const Span<Index> buffer) const {
		auto rangeA = outputIndices;
		auto rangeB = buffer;

		auto indicesIt = begin;
		
		// always points into the current rangeA
		auto goodItemsEnd = outputIndices.begin();
		size_t iteration = 0;

		while(true) {
			const auto itemsThisIt = std::min(std::distance(goodItemsEnd, rangeA.end()), std::distance(indicesIt, end));
			const auto rangeAEnd = goodItemsEnd + itemsThisIt;
			
			std::copy(indicesIt, indicesIt + itemsThisIt, goodItemsEnd);
			indicesIt += itemsThisIt;
			std::sort(goodItemsEnd, rangeAEnd);

			if(goodItemsEnd == rangeA.begin()) {
				goodItemsEnd = items_.makeUniqueFull(goodItemsEnd, rangeAEnd);
			} else {
				goodItemsEnd = items_.makeUniqueMerge(rangeA.begin(), goodItemsEnd, rangeAEnd, rangeB);
				std::swap(rangeA, rangeB);
			}

			const auto remainingItems = std::distance(goodItemsEnd, rangeA.end());
			++iteration;
			if(remainingItems == 0 || indicesIt + remainingItems > end)
				break;
		}

		if(rangeA.data() != outputIndices.data())
			std::copy(rangeA.begin(), goodItemsEnd, outputIndices.begin());

		return FindUniqueResult {
			size_t(std::distance(rangeA.begin(), goodItemsEnd)),
			size_t(std::distance(begin, indicesIt))
		};
	}

	std::wstring_view GetSuffix(const std::wstring_view text, const Index index, const size_t length) {
		return text.substr(index, length);
	}

	Span<Index>::iterator ItemsArray::makeUniqueFull(const Span<Index>::iterator begin, const Span<Index>::iterator end) const {
		Index lastWordEnd = 0;
		auto writeIt = begin;
		for(auto it = begin; it != end; ++it) {
			const auto index = *it;
			if(index < lastWordEnd)
				continue;

			const auto offset = getItem(index);
			lastWordEnd = getItemEnd(offset);

			*writeIt++ = index;
		}

		return writeIt;
	}

	Span<Index>::iterator ItemsArray::makeUniqueMerge(const Span<const Index>::iterator begin,
																		const Span<const Index>::iterator secondHalfStart,
																		const Span<const Index>::iterator end,
																		const Span<Index> target) const {
		auto i1 = begin;
		auto i2 = secondHalfStart;

		Index lastWordEnd = 0;
		auto write = target.begin();
		for(; i1 != secondHalfStart;) {
			Index min;
			if(i2 == end) {
				min = *i1++;
			} else {
				const auto v1 = *i1;
				const auto v2 = *i2;

				if(v1 < v2)
					min = *i1++;
				else
					min = *i2++;
			}

			if(min < lastWordEnd)
				continue;

			lastWordEnd = getItemEnd(getItem(min));
			*write++ = min;
		}

		return write;
	}

}
