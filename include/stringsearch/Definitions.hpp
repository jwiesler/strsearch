#pragma once
#include <tcb/span.hpp>
#include <type_traits>

namespace stringsearch {
	using Index = int;

	static_assert(std::is_signed_v<int>);

	template<typename T, std::size_t Extent = tcb::dynamic_extent>
	using Span = tcb::span<T, Extent>;

	template<typename T, std::size_t Extent = tcb::dynamic_extent>
	Span<T, Extent> MakeSpan(T *begin, T *end) {
		return Span<T, Extent>(begin, end);
	}
}