#pragma once
#include <tcb/span.hpp>
#include <type_traits>

#define DEFINE_CONSTRUCTORS(clsbase, cls, what)\
	clsbase(cls) = what;\
	clsbase &operator=(cls) = what

#define DISABLE_COPY(cls) DEFINE_CONSTRUCTORS(cls, const cls &, delete)
#define DISABLE_MOVE(cls) DEFINE_CONSTRUCTORS(cls, cls &&, delete)
#define DEFAULT_COPY(cls) DEFINE_CONSTRUCTORS(cls, const cls &, default)
#define DEFAULT_MOVE(cls) DEFINE_CONSTRUCTORS(cls, cls &&, default)

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