#pragma once
#include <iterator>
#include <algorithm>

namespace stringsearch {
	constexpr auto LessThanUtf16Le = [](const char16_t a, const char16_t b) noexcept {
		const auto a1 = (a & 0xFF);
		const auto b1 = (b & 0xFF);
		if(a1 < b1)
			return true;
		if(b1 < a1)
			return false;
		return (a >> 8) < (b >> 8);
	};

	[[nodiscard]] inline bool LessThan(const std::u16string_view a, const std::u16string_view b) noexcept {
		return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), LessThanUtf16Le);
	}
	
	class Utf16LETextIterator {
		const unsigned char * ptr_;

	public:
		using value_type = const std::byte;
		using pointer = const std::byte *;
		using reference = const std::byte &;
		using difference_type = ptrdiff_t;
		using iterator_category = std::forward_iterator_tag;
		
		explicit Utf16LETextIterator(const unsigned char *ptr)
			: ptr_(ptr) {}

		explicit Utf16LETextIterator(const char16_t *ptr)
			: ptr_(reinterpret_cast<const unsigned char *>(ptr) + 1) {}

		Utf16LETextIterator &operator++() noexcept {
			const auto isOdd = isOddAddress();
			const auto nextText = isOdd ? ptr_ - 1 : ptr_ + 3;
			ptr_ = nextText;
			return *this;
		}

		[[nodiscard]] bool isOddAddress() const noexcept {
			return (reinterpret_cast<uintptr_t>(ptr_) & 1) != 0;
		}

		[[nodiscard]] const unsigned char &operator*() const noexcept {
			return *ptr_;
		}

		[[nodiscard]] const unsigned char *get() const noexcept {
			return ptr_;
		}

		bool operator!=(const Utf16LETextIterator &o) const noexcept {
			return !(*this == o);
		}

		bool operator==(const Utf16LETextIterator &o) const noexcept {
			return ptr_ == o.ptr_;
		}

		Utf16LETextIterator &advanceDouble(const ptrdiff_t difference) noexcept {
			ptr_ += 2 * difference;
			return *this;
		}

		Utf16LETextIterator operator++(int) const noexcept {
			auto it = *this;
			++it;
			return it;
		}
	};
}
