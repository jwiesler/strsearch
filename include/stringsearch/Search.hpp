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

	using IndexPtr = std::vector<Index>::const_iterator;

	class FindResult {
		IndexPtr begin_;
		IndexPtr end_;

	public:
		FindResult(const IndexPtr begin, const IndexPtr end)
			: begin_(begin),
				end_(end) {}

		[[nodiscard]] IndexPtr begin() const noexcept { return begin_; }

		[[nodiscard]] IndexPtr end() const noexcept { return end_; }

		[[nodiscard]] size_t size() const noexcept { return std::distance(begin_, end_); }
	};

	void CreateArray(std::wstring_view text, Span<Index> sa);
	
	class SuffixArray {
		std::vector<Index> sa_;

	public:
		explicit SuffixArray(Span<const Index> array);
		explicit SuffixArray(std::u16string_view text);

		[[nodiscard]] FindResult find(std::u16string_view text, std::u16string_view pattern) const;

		[[nodiscard]] static IndexPtr lowerBound(IndexPtr begin, IndexPtr end, std::u16string_view text,
																std::u16string_view pattern);

		[[nodiscard]] static IndexPtr upperBound(IndexPtr begin, IndexPtr end, std::u16string_view text,
																std::u16string_view pattern);

		[[nodiscard]] IndexPtr begin() const noexcept { return sa_.begin(); }

		[[nodiscard]] IndexPtr end() const noexcept { return sa_.end(); }

		[[nodiscard]] const std::vector<Index>& get() const { return sa_; }

		[[nodiscard]] Index indexOf(IndexPtr it) const noexcept;
	};

	struct FindUniqueResult {
		size_t Count;
		size_t Consumed;

		FindUniqueResult(size_t count, size_t consumed)
			: Count(count),
				Consumed(consumed) {}
	};

	struct FindUniqueInAllResult {
		size_t TotalCount;
		size_t Count;

		explicit FindUniqueInAllResult(size_t totalCount, size_t count)
			: TotalCount(totalCount), Count(count) {}
	};

	class ItemsLookup {
		std::vector<Index> items_;
		size_t itemCount_;

	public:
		explicit ItemsLookup(std::u16string_view text);

		[[nodiscard]] Index getItem(const Index suffix) const { return items_[suffix]; }

		[[nodiscard]] size_t itemCount() const noexcept { return itemCount_; }
	};

	class OldUniqueSearchLookup : ItemsLookup {
		std::vector<Index> itemEnds_;

	public:
		explicit OldUniqueSearchLookup(std::u16string_view text);

		[[nodiscard]] Index getItemEnd(const Index item) const { return itemEnds_[item]; }

		[[nodiscard]] Span<Index>::iterator makeUniqueMerge(Span<const Index>::iterator begin,
																			Span<const Index>::iterator secondHalfStart,
																			Span<const Index>::iterator end,
																			Span<Index> target) const;

		[[nodiscard]] Span<Index>::iterator makeUniqueFull(Span<Index>::iterator begin, Span<Index>::iterator end) const;

		[[nodiscard]] size_t itemCount() const { return itemEnds_.size(); }

		[[nodiscard]] FindUniqueResult findUnique(FindResult result, Span<Index> outputIndices) const;

		[[nodiscard]] FindUniqueResult findUnique(FindResult result, Span<Index> outputIndices,
																	Span<Index> buffer) const;
	};

	class UniqueItemsIterator;
	
	class UniqueSearchLookup : public ItemsLookup {
		const SuffixArray &suffixArray_;
		std::vector<Index> previousEntryOfSameItem_;

	public:
		UniqueSearchLookup(std::u16string_view text, const SuffixArray &sa);

		[[nodiscard]] Index previousEntryOf(Index saIndex) const noexcept;

		[[nodiscard]] bool isDuplicateInRange(IndexPtr begin, Index prev) const noexcept;
		
		[[nodiscard]] bool isDuplicateInRange(IndexPtr begin, IndexPtr ptr) const noexcept;

		[[nodiscard]] const std::vector<Index>& previousEntryOfSameItem() const noexcept {
			return previousEntryOfSameItem_;
		}

		[[nodiscard]] FindUniqueResult findUnique(FindResult result, Span<Index> outputIndices, unsigned int offset = 0) const;

		[[nodiscard]] FindUniqueInAllResult findUniqueInAllOf(Span<const FindResult> results, Span<Index> outputIndices) const;

		[[nodiscard]] UniqueItemsIterator uniqueItemsInRange(FindResult result, unsigned int offset) const noexcept;
	};

	[[nodiscard]] std::u16string_view GetSuffix(std::u16string_view text, Index index, size_t length);

	class Search {
		SuffixArray suffixArray_;
		UniqueSearchLookup itemsLookup_;
		std::u16string_view text_;

	public:
		explicit Search(std::u16string_view text);

		DISABLE_COPY(Search);
		DISABLE_MOVE(Search);

		[[nodiscard]] FindResult find(std::u16string_view pattern) const;

		[[nodiscard]] FindUniqueResult findUnique(FindResult result, Span<Index> outputIndices, unsigned int offset) const;
		
		[[nodiscard]] FindUniqueInAllResult findUniqueInAllOf(Span<const FindResult> results, Span<Index> outputIndices) const;

		[[nodiscard]] const SuffixArray& suffixArray() const noexcept { return suffixArray_; }

		[[nodiscard]] const UniqueSearchLookup& itemsLookup() const noexcept { return itemsLookup_; }
	};

	class UniqueItemsIteratorEnd {};

	class UniqueItemsIterator {
		const FindResult result_;
		IndexPtr it_;
		const UniqueSearchLookup& itemsLookup_;

	public:
		UniqueItemsIterator(const FindResult result, const IndexPtr it, const UniqueSearchLookup& itemsLookup)
			: result_(result),
				it_(it),
				itemsLookup_(itemsLookup) {
			if(it_ != result_.end() && isDuplicate())
				next();
		}

		UniqueItemsIterator(const FindResult result, const UniqueSearchLookup& itemsLookup)
			: UniqueItemsIterator(result, result.begin(), itemsLookup) {}

		[[nodiscard]] explicit operator bool() const noexcept {
			return it_ == result_.end();
		}

		[[nodiscard]] const Index& operator*() const noexcept {
			return *it_;
		}

		UniqueItemsIterator& operator++() noexcept {
			next();
			return *this;
		}

		bool operator==(UniqueItemsIteratorEnd) const noexcept {
			return operator bool();
		}

		bool operator!=(const UniqueItemsIteratorEnd &e) const noexcept {
			return !(*this == e);
		}

		void next() noexcept;

		[[nodiscard]] bool isDuplicate() const noexcept;

		size_t offsetFromResultBegin() const noexcept;
	};
}
