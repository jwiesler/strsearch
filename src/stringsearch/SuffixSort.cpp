#include "stringsearch/SuffixSort.hpp"
#include <algorithm>
#include <numeric>
#include <array>
#include <vector>
#include "stringsearch/Utf16Le.hpp"
#include <cassert>

namespace stringsearch {
	using TextIterator = Utf16LETextIterator;

	template<typename T>
	void RangeCheck(T *begin, T *end) {
		assert(begin <= end);
	}

	void RangeCheck(const Utf16LETextIterator begin, const Utf16LETextIterator end) {
		RangeCheck(begin.get(), end.get());
	}

	TextIterator AdvanceDouble(TextIterator it, const ptrdiff_t distance) {
		return it.advanceDouble(distance);
	}
	
	size_t ToBucketIndex(const TextIterator text, const TextIterator end, const Index suffix) {
		const auto it = AdvanceDouble(text, suffix);
		RangeCheck(it, end);
		const auto res = static_cast<size_t>(*it);
		assert(res < 0x100);
		return res;
	}

	void Count(const TextIterator text, const TextIterator end, const Span<const Index> sa, const Span<Index> buckets) {
		for(const auto suffix : sa) {
			const auto bucket = ToBucketIndex(text, end, suffix);
			buckets[bucket]++;
		}
	}

	void MoveElements(const TextIterator text, const TextIterator end, const Span<Index> bucketStarts, const Span<Index> buffer,
							const Span<Index> sa) {
		for(const auto suffix : sa) {
			const auto bucket = ToBucketIndex(text, end, suffix);
			const auto off = bucketStarts[bucket]++;
			buffer[off] = suffix;
		}

		std::copy(buffer.begin(), buffer.end(), sa.begin());
	}

	void MoveElementsInPlace(const TextIterator text, const TextIterator end, const Span<Index> bucketStarts, const Span<Index> sa) {
		for(auto it = sa.begin(); it != sa.end();) {
			const auto suffix = *it;
			const auto bucket = ToBucketIndex(text, end, suffix);
			const auto off = bucketStarts[bucket];

			// Swap only with elements in front of the current element
			if(off > it - sa.begin()) {
				++it;
				continue;
			}

			if(off == it - sa.begin()) {
				++it;
			} else {
				std::iter_swap(sa.begin() + off, it);
			}

			bucketStarts[bucket]++;
		}
	}

	void SuffixSortStd(const char16_t *text, const char16_t *end, const Span<Index> sa) {
		std::sort(sa.begin(), sa.end(), [&](const Index a, const Index b) {
			return std::lexicographical_compare(
				text + a, end, 
				text + b, end,
				LessThanUtf16Le
			);
		});
	}

	template<typename F>
	void DispatchBuckets(const TextIterator text, const std::array<Index, 0x100> &buckets, const Span<Index> sa, F &&f) {
		auto last = 0;

		for(auto v : buckets) {
			if(v == last)
				continue;
			if(v - last > 1)
				f(text, sa.subspan(last, size_t(v) - size_t(last)));

			last = v;
		}
	}

	struct SharedBuffer {
		const Span<Index> Buffer;

		void moveElements(const TextIterator text, const TextIterator end, const Span<Index> bucketStarts, const Span<Index> sa) const {
			MoveElements(text, end, bucketStarts, Buffer.subspan(0, sa.size()), sa);
		}
	};

	struct OwnBuffer {
		void moveElements(const TextIterator text, const TextIterator end, const Span<Index> bucketStarts, const Span<Index> sa) const {
			std::vector<Index> buffer(sa.size());
			MoveElements(text, end, bucketStarts, buffer, sa);
		}
	};

	struct InPlace {
		void moveElements(const TextIterator text, const TextIterator end, const Span<Index> bucketStarts, const Span<Index> sa) const {
			MoveElementsInPlace(text, end, bucketStarts, sa);
		}
	};

	template<typename Derived>
	void SuffixSort(const TextIterator text, const TextIterator textEnd, const Span<Index> sa, Derived derived) {
		RangeCheck(text, textEnd);
		std::array<Index, 0x100> buckets{};

		Count(text, textEnd, sa, buckets);

		const auto allInOne = size_t(buckets[ToBucketIndex(text, textEnd, sa[0])]) == sa.size();
		const auto nextText = text++;

		if(allInOne) {
			SuffixSort(nextText, textEnd, sa, derived);
			return;
		}

		std::exclusive_scan(buckets.begin(), buckets.end(), buckets.begin(), Index(0));
		derived.moveElements(text, textEnd, buckets, sa);

		DispatchBuckets(nextText, buckets, sa, [&](const auto newText, const auto range) {
			SuffixSort(newText, textEnd, range, derived);
		});
	}

	TextIterator ToTextIterator(const char16_t *characters) {
		return Utf16LETextIterator(characters);
	}

	template<typename Derived>
	void SuffixSort(const std::u16string_view characters, const Span<Index> sa, Derived derived) {
		SuffixSort(ToTextIterator(BeginPtr(characters)), ToTextIterator(EndPtr(characters)), sa, derived);
	}
	
	template<typename Derived>
	void SuffixSortMax(const TextIterator text, const TextIterator textEnd, const Span<Index> sa, const size_t max, Derived derived) {
		RangeCheck(text, textEnd);
		if(sa.size() < max && text.isOddAddress()) {
			// We assume textEnd is always at an odd address by construction...
			SuffixSortStd(reinterpret_cast<const char16_t *>(text.get() - 1), reinterpret_cast<const char16_t *>(textEnd.get() - 1), sa);
			return;
		}
		
		std::array<Index, 0x100> buckets{};

		Count(text, textEnd, sa, buckets);

		const auto allInOne = size_t(buckets[ToBucketIndex(text, textEnd, sa[0])]) == sa.size();

		const auto nextText = text++;

		if(allInOne) {
			SuffixSortMax<Derived>(nextText, textEnd, sa, max, derived);
			return;
		}

		std::exclusive_scan(buckets.begin(), buckets.end(), buckets.begin(), Index(0));
		derived.moveElements(text, textEnd, buckets, sa);

		DispatchBuckets(nextText, buckets, sa, [&](const auto newText, const auto range) {
			SuffixSortMax<Derived>(newText, textEnd, range, max, derived);
		});
	}

	template<typename Derived>
	void SuffixSortMax(const std::u16string_view characters, const Span<Index> sa, const size_t max, Derived derived) {
		SuffixSortMax(ToTextIterator(BeginPtr(characters)), ToTextIterator(EndPtr(characters)), sa, max, derived);
	}
	
	void SuffixSortStd(const std::u16string_view characters, const Span<Index> sa) {
		SuffixSortStd(BeginPtr(characters), EndPtr(characters), sa);
	}
	
	void SuffixSortSharedBuffer(const std::u16string_view characters, const Span<Index> sa) {
		std::vector<Index> buffer(sa.size());
		SuffixSort(characters, sa, SharedBuffer{buffer});
	}

	void SuffixSortOwnBuffer(const std::u16string_view characters, const Span<Index> sa) {
		SuffixSort(characters, sa, OwnBuffer());
	}

	void SuffixSortInPlace(const std::u16string_view characters, const Span<Index> sa) {
		SuffixSort(characters, sa, InPlace());
	}

	void SuffixSortSharedBufferMax(const std::u16string_view characters, const Span<Index> sa, const size_t max) {
		std::vector<Index> buffer(sa.size());
		SuffixSortMax(characters, sa, max, SharedBuffer{buffer});
	}

	void SuffixSortOwnBufferMax(const std::u16string_view characters, const Span<Index> sa, const size_t max) {
		SuffixSortMax(characters, sa, max, OwnBuffer());
	}

	void SuffixSortInPlaceMax(const std::u16string_view characters, const Span<Index> sa, const size_t max) {
		SuffixSortMax(characters, sa, max, InPlace());
	}
}
