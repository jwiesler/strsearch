#include "stringsearch/SuffixSort.hpp"
#include <numeric>
#include <array>
#include <vector>
#include <ios>

namespace stringsearch {
	class Utf16TextIterator {
		const std::byte * ptr_;

	public:
		explicit Utf16TextIterator(const std::byte *ptr)
			: ptr_(ptr) {}

		Utf16TextIterator &operator++() noexcept {
			const auto isOddAddress = (reinterpret_cast<uintptr_t>(ptr_) & 1) == 1;
			const auto nextText = isOddAddress ? ptr_ - 1 : ptr_ + 3;
			ptr_ = nextText;
			return *this;
		}

		[[nodiscard]] const std::byte &operator*() const noexcept {
			return *ptr_;
		}

		[[nodiscard]] const std::byte *get() const noexcept {
			return ptr_;
		}

		bool operator!=(const Utf16TextIterator &o) const noexcept {
			return !(*this == o);
		}

		bool operator==(const Utf16TextIterator &o) const noexcept {
			return ptr_ == o.ptr_;
		}
	};
	
	size_t ToBucketIndex(const std::byte *const text, const Index suffix) {
		return static_cast<size_t>(*(text + 2 * size_t(suffix)));
	}

	void Count(const std::byte *const text, const Span<const Index> sa, const Span<Index> buckets) {
		for(const auto suffix : sa) {
			const auto bucket = ToBucketIndex(text, suffix);
			buckets[bucket]++;
		}
	}

	void MoveElements(const std::byte *const text, const Span<Index> bucketStarts, const Span<Index> buffer,
							const Span<Index> sa) {
		for(const auto suffix : sa) {
			const auto bucket = ToBucketIndex(text, suffix);
			const auto off = bucketStarts[bucket]++;
			buffer[off] = suffix;
		}

		std::copy(buffer.begin(), buffer.end(), sa.begin());
	}

	void MoveElementsInPlace(const std::byte *const text, const Span<Index> bucketStarts, const Span<Index> sa) {
		for(auto it = sa.begin(); it != sa.end();) {
			const auto suffix = *it;
			const auto bucket = ToBucketIndex(text, suffix);
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

	void SuffixSortStd(const std::byte *const text, const Span<Index> sa) {
		std::sort(sa.begin(), sa.end(), [&](const Index a, const Index b) {
			return std::lexicographical_compare(
				Utf16TextIterator(text + 2 * size_t(a)), Utf16TextIterator(nullptr), 
				Utf16TextIterator(text + 2 * size_t(b)), Utf16TextIterator(nullptr)
			);
		});
	}

	template<typename F>
	void DispatchBuckets(const std::byte *nextText, const std::array<Index, 0x100> &buckets, const Span<Index> sa, F &&f) {
		auto last = 0;//isOddAddress ? 0 : buckets[0];

		for(auto v : buckets) {
			if(v == last)
				continue;
			if(v - last > 1)
				f(nextText, sa.subspan(last, size_t(v) - size_t(last)));

			last = v;
		}
	}

	struct SharedBuffer {
		const Span<Index> Buffer;

		void moveElements(const std::byte *const text, const Span<Index> bucketStarts, const Span<Index> sa) const {
			MoveElements(text, bucketStarts, Buffer.subspan(0, sa.size()), sa);
		}
	};

	struct OwnBuffer {
		void moveElements(const std::byte *const text, const Span<Index> bucketStarts, const Span<Index> sa) const {
			std::vector<Index> buffer(sa.size());
			MoveElements(text, bucketStarts, buffer, sa);
		}
	};

	struct InPlace {
		void moveElements(const std::byte *const text, const Span<Index> bucketStarts, const Span<Index> sa) const {
			MoveElementsInPlace(text, bucketStarts, sa);
		}
	};

	template<typename Derived>
	void SuffixSort(const std::byte *const text, const Span<Index> sa, Derived derived) {
		std::array<Index, 0x100> buckets{};

		Count(text, sa, buckets);

		const auto allInOne = size_t(buckets[ToBucketIndex(text, sa[0])]) == sa.size();
		const auto isOddAddress = (reinterpret_cast<uintptr_t>(text) & 1) == 1;

		const auto nextText = isOddAddress ? text - 1 : text + 3;

		if(allInOne) {
			SuffixSort(nextText, sa, derived);
			return;
		}

		std::exclusive_scan(buckets.begin(), buckets.end(), buckets.begin(), Index(0));
		derived.moveElements(text, buckets, sa);

		DispatchBuckets(nextText, buckets, sa, [&](const std::byte *const t, const Span<Index> s) {
			SuffixSort(t, s, derived);
		});
	}

	template<typename Derived>
	void SuffixSortMax(const std::byte *const text, const Span<Index> sa, const size_t max, Derived derived) {
		if(sa.size() < max) {
			SuffixSortStd(text, sa);
			return;
		}
		
		std::array<Index, 0x100> buckets{};

		Count(text, sa, buckets);

		const auto allInOne = size_t(buckets[ToBucketIndex(text, sa[0])]) == sa.size();
		const auto isOddAddress = (reinterpret_cast<uintptr_t>(text) & 1) == 1;

		const auto nextText = isOddAddress ? text - 1 : text + 3;

		if(allInOne) {
			SuffixSortMax<Derived>(nextText, sa, max, derived);
			return;
		}

		std::exclusive_scan(buckets.begin(), buckets.end(), buckets.begin(), Index(0));
		derived.moveElements(text, buckets, sa);

		DispatchBuckets(nextText, buckets, sa, [&](const std::byte *const t, const Span<Index> s) {
			SuffixSortMax<Derived>(t, s, max, derived);
		});
	}

	void SuffixSortStd(const wchar_t *characters, const Span<Index> sa) {
		SuffixSortStd(reinterpret_cast<const std::byte*>(characters) + 1, sa);
	}
	
	void SuffixSortSharedBuffer(const wchar_t *characters, const Span<Index> sa) {
		std::vector<Index> buffer(sa.size());
		SuffixSort(reinterpret_cast<const std::byte*>(characters) + 1, sa, SharedBuffer{buffer});
	}

	void SuffixSortOwnBuffer(const wchar_t *characters, const Span<Index> sa) {
		SuffixSort(reinterpret_cast<const std::byte*>(characters) + 1, sa, OwnBuffer());
	}

	void SuffixSortInPlace(const wchar_t *characters, const Span<Index> sa) {
		SuffixSort(reinterpret_cast<const std::byte*>(characters) + 1, sa, InPlace());
	}

	void SuffixSortSharedBufferMax(const wchar_t *characters, const Span<Index> sa, const size_t max) {
		std::vector<Index> buffer(sa.size());
		SuffixSortMax(reinterpret_cast<const std::byte*>(characters) + 1, sa, max, SharedBuffer{buffer});
	}

	void SuffixSortOwnBufferMax(const wchar_t *characters, const Span<Index> sa, const size_t max) {
		SuffixSortMax(reinterpret_cast<const std::byte*>(characters) + 1, sa, max, OwnBuffer());
	}

	void SuffixSortInPlaceMax(const wchar_t *characters, const Span<Index> sa, const size_t max) {
		const auto ptr = reinterpret_cast<const std::byte*>(characters);
		//Span<const std::byte>(ptr, );
		SuffixSortMax(ptr + 1, sa, max, InPlace());
	}
}
