#pragma once
#include <iterator>

template<typename OutputIterator, typename F>
class MappingOutputIterator : F {
public:
	using iterator_category = std::output_iterator_tag;
	using value_type = void;
	using pointer = void;
	using reference = void;
	using difference_type = ptrdiff_t;

	MappingOutputIterator(OutputIterator iterator, F f)
		: F(f), iterator_(std::move(iterator)) {}

	MappingOutputIterator(MappingOutputIterator &&o) = default;
	MappingOutputIterator(const MappingOutputIterator &o) = default;

	MappingOutputIterator &operator=(const MappingOutputIterator &o) noexcept {
		iterator_ = o.iterator_;
		return *this;
	}
	
	MappingOutputIterator &operator=(MappingOutputIterator &&o) noexcept {
		iterator_ = o.iterator_;
		return *this;
	}

	template<typename T, typename = std::enable_if_t<std::is_invocable_v<const F, T &&>>>
	MappingOutputIterator& operator=(T &&val) {
		*iterator_ = F::operator()(std::forward<T>(val));
		return *this;
	}

	[[nodiscard]] MappingOutputIterator& operator*() noexcept {
		return *this;
	}

	MappingOutputIterator& operator++() noexcept {
		++iterator_;
		return *this;
	}

	MappingOutputIterator operator++(int) noexcept {
		MappingOutputIterator it = *this;
		++it;
		return it;
	}
	
private:
	OutputIterator iterator_;
};

template<typename OutputIterator, typename F> auto Map(OutputIterator iterator, F f) {
	return MappingOutputIterator<OutputIterator, F>(iterator, f);
}