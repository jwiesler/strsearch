#pragma once
#include <tcb/span.hpp>

namespace stringsearch {
	using Index = int;

	template<typename T, std::size_t Extent = tcb::dynamic_extent>
	using Span = tcb::span<T, Extent>;

	template<typename T, std::size_t Extent = tcb::dynamic_extent>
	Span<T, Extent> MakeSpan(T *begin, T *end) {
		return Span<T, Extent>(begin, end);
	}
}