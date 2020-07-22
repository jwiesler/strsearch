#pragma once
#include "Definitions.hpp"

namespace stringsearch {
	class Utf16TextIterator {
		const char * ptr_;

	public:
		using value_type = const std::byte;
		using pointer = const std::byte *;
		using reference = const std::byte &;
		using difference_type = ptrdiff_t;
		using iterator_category = std::forward_iterator_tag;
		
		explicit Utf16TextIterator(const char *ptr)
			: ptr_(ptr) {}

		explicit Utf16TextIterator(const wchar_t *ptr)
			: ptr_(reinterpret_cast<const char *>(ptr) + 1) {}

		Utf16TextIterator &operator++() noexcept {
			const auto isOddAddress = (reinterpret_cast<uintptr_t>(ptr_) & 1) == 1;
			const auto nextText = isOddAddress ? ptr_ - 1 : ptr_ + 3;
			ptr_ = nextText;
			return *this;
		}

		[[nodiscard]] const char &operator*() const noexcept {
			return *ptr_;
		}

		[[nodiscard]] const char *get() const noexcept {
			return ptr_;
		}

		bool operator!=(const Utf16TextIterator &o) const noexcept {
			return !(*this == o);
		}

		bool operator==(const Utf16TextIterator &o) const noexcept {
			return ptr_ == o.ptr_;
		}

		Utf16TextIterator &advanceDouble(const ptrdiff_t difference) noexcept {
			ptr_ += 2 * difference;
			return *this;
		}

		Utf16TextIterator operator++(int) const noexcept {
			auto it = *this;
			++it;
			return it;
		}
	};
	
	void SuffixSortStd(std::wstring_view characters, Span<Index> sa);
	
	void SuffixSortSharedBuffer(std::wstring_view characters, Span<Index> sa);

	void SuffixSortOwnBuffer(std::wstring_view characters, Span<Index> sa);

	void SuffixSortInPlace(std::wstring_view characters, Span<Index> sa);
	
	void SuffixSortSharedBufferMax(std::wstring_view characters, Span<Index> sa, size_t max = 80);

	void SuffixSortOwnBufferMax(std::wstring_view characters, Span<Index> sa, size_t max = 80);

	void SuffixSortInPlaceMax(std::wstring_view characters, Span<Index> sa, size_t max = 80);
}
