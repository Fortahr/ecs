#pragma once

#include <type_traits>

namespace ecs
{
	// Exclude component from query, note that includes override excludes.
	template<typename T> struct exclude {};

	template<typename... _Ts> struct pack {};

	template<typename T> struct is_exclude : std::false_type {};
	template<typename T> struct is_exclude<exclude<T>> : std::true_type {};
	template<typename T> constexpr bool is_exclude_v = is_exclude<T>::value;

	template<typename T> struct decay_exclude { using type = T; };
	template<typename T> struct decay_exclude<exclude<T>> { using type = T; };
	template<typename T> using decay_exclude_t = typename decay_exclude<T>::type;

	template<bool _First, typename A, typename B>
	constexpr std::conditional_t<_First, A, B>& conditional_v(A& a, B& b)
	{
		if constexpr (_First)
			return a;
		else
			return b;
	}
}
