#pragma once
#include "ApiDefinitions.h"
#include <string_view>

namespace stringsearch::api {
	template<typename T>
	struct APIArg {
		static constexpr size_t argc = 1;
		static Result validate(T) noexcept {
			return Result::Ok;
		}

		static T convert(T value) noexcept {
			return value;
		}
	};

	template<>
	struct APIArg<std::u16string_view> {
		static constexpr size_t argc = 2;
		static Result validate(const char16_t *ptr, const size_t) noexcept {
			return ptr != nullptr ? Result::Ok : Result::NullPointer;
		}
		
		static std::u16string_view convert(const char16_t *ptr, const size_t count) noexcept {
			return std::u16string_view(ptr, count);
		}
	};

	template<typename T>
	struct APIArg<Span<T>> {
		static constexpr size_t argc = 2;
		
		static Result validate(T *ptr, const size_t) noexcept {
			return ptr != nullptr ? Result::Ok : Result::NullPointer;
		}
		
		static Span<T> convert(T *ptr, const size_t count) noexcept {
			return Span<T>(ptr, count);
		}
	};

	template<typename... Args>
	struct ApiFunctionCaller {
		template<size_t Offset, typename F, size_t... Idx, typename Tuple, typename... UnwrappedArgs>
		static Result call(F f, Tuple &&, UnwrappedArgs &&... unwrappedArgs) {
			static_assert(Offset == std::tuple_size_v<Tuple>);
			if constexpr(std::is_same_v<decltype(f(std::forward<UnwrappedArgs>(unwrappedArgs)...)), void>) {
				f(std::forward<UnwrappedArgs>(unwrappedArgs)...);
				return Result::Ok;
			} else {
				return f(std::forward<UnwrappedArgs>(unwrappedArgs)...);
			}
		}
	};

	template<typename T, typename... Args>
	struct ApiFunctionCaller<T, Args...> {
		using A = APIArg<std::remove_const_t<std::remove_reference_t<T>>>;
		
		template<size_t Offset, typename F, size_t... Idx, typename Tuple, typename... UnwrappedArgs>
		static Result call(F f, Tuple &&args, std::index_sequence<Idx...>, UnwrappedArgs &&... unwrappedArgs) {
			const auto res = A::validate(std::get<Offset + Idx>(args)...);
			if(res != Result::Ok)
				return res;

			return ApiFunctionCaller<Args...>::template call<Offset + sizeof...(Idx)>(f, std::forward<Tuple>(args), std::forward<UnwrappedArgs>(unwrappedArgs)..., A::convert(std::get<Offset + Idx>(args)...));
		}
		
		template<size_t Offset, typename F, typename Tuple, typename... UnwrappedArgs>
		static Result call(F f, Tuple &&args, UnwrappedArgs &&... unwrappedArgs) {
			return call<Offset>(f, std::forward<Tuple>(args), std::make_index_sequence<A::argc>(), std::forward<UnwrappedArgs>(unwrappedArgs)...);
		}
	};

	template<typename Signature>
	struct ApiFunction;

	template<typename T, typename... Args>
	struct ApiFunction<T(Args...)> {
		using Caller = ApiFunctionCaller<Args...>;
	};

	template<typename Signature, typename F, typename Tuple>
	Result CallApiFunctionImplementation(F f, Tuple &&args) {
		return ApiFunction<Signature>::Caller::template call<0>(f, std::forward<Tuple>(args));
	}
}