#include "stringsearch/Search.hpp"

#include "stringsearch/SuffixSort.hpp"
#include "stringsearch/Utf16Le.hpp"

#include <algorithm>
#include <numeric>
#include <vector>
#include <unordered_map>

namespace stringsearch {
	ItemsLookup::ItemsLookup(const std::u16string_view text) {
		items_.reserve(text.size());

		auto index = Index(0);
		for(auto it : text) {
			items_.emplace_back(index);
			if(it != 0)
				continue;
			
			++index;
		}

		itemCount_ = index;
	}

	OldUniqueSearchLookup::OldUniqueSearchLookup(const std::u16string_view text) : ItemsLookup(text) {
		itemEnds_.reserve(itemCount());
		auto index = Index(0);
		for(auto it = text.begin(); it != text.end(); ++it) {
			if(*it != 0)
				continue;

			itemEnds_.emplace_back(Index(std::distance(text.begin(), it)));
			++index;
		}
	}
	
	FindUniqueResult OldUniqueSearchLookup::findUnique(const FindResult result,
																const Span<Index> outputIndices) const {
		std::vector<Index> buffer(outputIndices.size());
		return findUnique(result, outputIndices, buffer);
	}

	FindUniqueResult OldUniqueSearchLookup::findUnique(const FindResult result, const Span<Index> outputIndices,
																const Span<Index> buffer) const {
		auto rangeA = outputIndices;
		auto rangeB = buffer;

		auto indicesIt = result.begin();
		
		// always points into the current rangeA
		auto goodItemsEnd = outputIndices.begin();
		size_t iteration = 0;

		while(true) {
			const auto itemsThisIt = std::min(std::distance(goodItemsEnd, rangeA.end()),
														std::distance(indicesIt, result.end()));
			const auto rangeAEnd = goodItemsEnd + itemsThisIt;

			std::copy(indicesIt, indicesIt + itemsThisIt, goodItemsEnd);
			indicesIt += itemsThisIt;
			std::sort(goodItemsEnd, rangeAEnd);

			if(goodItemsEnd == rangeA.begin()) {
				goodItemsEnd = makeUniqueFull(goodItemsEnd, rangeAEnd);
			} else {
				goodItemsEnd = makeUniqueMerge(rangeA.begin(), goodItemsEnd, rangeAEnd, rangeB);
				std::swap(rangeA, rangeB);
			}

			const auto remainingItems = std::distance(goodItemsEnd, rangeA.end());
			++iteration;
			if(remainingItems == 0 || std::distance(indicesIt, result.end()) < remainingItems)
				break;
		}

		if(rangeA.data() != outputIndices.data())
			std::copy(rangeA.begin(), goodItemsEnd, outputIndices.begin());

		return FindUniqueResult(
			std::distance(rangeA.begin(), goodItemsEnd),
			std::distance(result.begin(), indicesIt)
		);
	}

	Span<Index>::iterator OldUniqueSearchLookup::makeUniqueFull(const Span<Index>::iterator begin,
																		const Span<Index>::iterator end) const {
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

	Span<Index>::iterator OldUniqueSearchLookup::makeUniqueMerge(const Span<const Index>::iterator begin,
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

	void CreateArray(const std::u16string_view text, const Span<Index> sa) {
		std::iota(sa.begin(), sa.end(), Index(0));
		SuffixSortInPlace(text, sa);
	}

	SuffixArray::SuffixArray(const Span<const Index> array) : sa_(array.begin(), array.end()) {}

	SuffixArray::SuffixArray(const std::u16string_view text) : sa_(text.size()) {
		CreateArray(text, sa_);
	}

	FindResult SuffixArray::find(const std::u16string_view text, const std::u16string_view pattern) const {
		const auto lower = lowerBound(sa_.begin(), sa_.end(), text, pattern);
		const auto upper = upperBound(lower, sa_.end(), text, pattern);

		return FindResult(lower, upper);
	}

	IndexPtr SuffixArray::lowerBound(const IndexPtr begin, const IndexPtr end,
												const std::u16string_view text, const std::u16string_view pattern) {
		return std::lower_bound(begin, end, Index(0), [&](const Index &index, auto) {
			const auto suffix = GetSuffix(text, index, pattern.size());
			return LessThan(suffix, pattern);
		});
	}

	IndexPtr SuffixArray::upperBound(const IndexPtr begin, const IndexPtr end,
												const std::u16string_view text, const std::u16string_view pattern) {
		return std::upper_bound(begin, end, Index(0), [&](auto, const Index &index) {
			const auto suffix = GetSuffix(text, index, pattern.size());
			return LessThan(pattern, suffix);
		});
	}

	Index SuffixArray::indexOf(const IndexPtr it) const noexcept {
		return Index(std::distance(sa_.begin(), it));
	}

	UniqueSearchLookup::UniqueSearchLookup(const std::u16string_view text, const SuffixArray &sa) : ItemsLookup(text), suffixArray_(sa) {
		previousEntryOfSameItem_.reserve(sa.get().size());
		std::vector<Index> lastIndexOfWord(itemCount(), Index(-1));
		for(auto it = sa.begin(); it != sa.end(); ++it) {
			const auto word = getItem(*it);
			auto &value = lastIndexOfWord[word];
			previousEntryOfSameItem_.emplace_back(value);
			value = Index(std::distance(sa.begin(), it));
		}
	}

	FindUniqueResult UniqueSearchLookup::findUnique(const FindResult result, const Span<Index> outputIndices, unsigned int offset) const {
		auto write = outputIndices.begin();
		auto it = uniqueItemsInRange(result, offset);
		for(; it != UniqueItemsIteratorEnd() && write != outputIndices.end(); ++it, ++write)
			*write = *it;

		return FindUniqueResult(
			size_t(std::distance(outputIndices.begin(), write)),
			it.offsetFromResultBegin()
		);
	}

	UniqueItemsIterator UniqueSearchLookup::uniqueItemsInRange(const FindResult result, const unsigned int offset) const noexcept {
		return UniqueItemsIterator(result, result.begin() + offset, *this);
	}

	Index UniqueSearchLookup::previousEntryOf(const Index saIndex) const noexcept {
		return previousEntryOfSameItem_[saIndex];
	}

	bool UniqueSearchLookup::isDuplicateInRange(const IndexPtr begin, const Index prev) const noexcept {
		return 0 <= prev && begin <= suffixArray_.begin() + prev;
	}

	bool UniqueSearchLookup::isDuplicateInRange(const IndexPtr begin, const IndexPtr ptr) const noexcept {
		return isDuplicateInRange(begin, previousEntryOf(suffixArray_.indexOf(ptr)));
	}

	FindUniqueInAllResult UniqueSearchLookup::findUniqueInAllOf(const Span<const FindResult> results, const Span<Index> outputIndices) const {
		struct ContainedInfo {
			unsigned Count;
			unsigned FirstContainedResultOffset;
		};
		std::unordered_map<Index, ContainedInfo> containedInCountMap;
		for(auto resultIt = results.begin(); resultIt != results.end(); ++resultIt) {
			const auto &result = *resultIt;
			const auto idx = std::distance(results.begin(), resultIt);
			for(auto it = uniqueItemsInRange(result, 0); it != UniqueItemsIteratorEnd(); ++it) {
				const auto suffix = *it;
				const auto item = getItem(suffix);
				const auto cit = containedInCountMap.find(item);
				if(cit == containedInCountMap.end()) {
					containedInCountMap.emplace(item, ContainedInfo{1, unsigned(idx)});
				} else {
					++cit->second.Count;
				}
			}
		}
		
		std::vector<std::pair<Index, ContainedInfo>> list(containedInCountMap.begin(), containedInCountMap.end());
		std::sort(list.begin(), list.end(), [](const std::pair<Index, ContainedInfo> &a, const std::pair<Index, ContainedInfo> &b) {
			return a.second.Count == b.second.Count ? a.second.FirstContainedResultOffset < b.second.FirstContainedResultOffset : a.second.Count > b.second.Count;
		});

		auto write = outputIndices.begin();
		for(auto it = list.begin(); it != list.end() && write != outputIndices.end(); ++it, ++write) {
			*write = it->first;
		}
		return FindUniqueInAllResult(list.size(), std::distance(outputIndices.begin(), write));
	}

	Search::Search(const std::u16string_view text)
		: suffixArray_(text),
			itemsLookup_(text, suffixArray()),
			text_(text) {}

	FindResult Search::find(const std::u16string_view pattern) const {
		return suffixArray_.find(text_, pattern);
	}

	FindUniqueResult Search::findUnique(const FindResult result, const Span<Index> outputIndices, const unsigned int offset) const {
		return itemsLookup().findUnique(result, outputIndices, offset);
	}

	FindUniqueInAllResult Search::findUniqueInAllOf(const Span<const FindResult> results, const Span<Index> outputIndices) const {
		return itemsLookup().findUniqueInAllOf(results, outputIndices);
	}

	void UniqueItemsIterator::next() noexcept {
		while(++it_ != result_.end() && isDuplicate()) {}
	}

	bool UniqueItemsIterator::isDuplicate() const noexcept {
		return itemsLookup_.isDuplicateInRange(result_.begin(), it_);
	}

	size_t UniqueItemsIterator::offsetFromResultBegin() const noexcept {
		return std::distance(result_.begin(), it_);
	}

	std::u16string_view GetSuffix(const std::u16string_view text, const Index index, const size_t length) {
		return text.substr(index, length);
	}
}
