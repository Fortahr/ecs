#pragma once

#include <type_traits>

namespace ecs
{
	// Exclude component from query.
	// Note that includes override excludes, e.g.: `world.query<ecs::exclude<C>>([](C& c) {});` will not exclude entities with type `C`
	template<typename T> struct exclude {};
	template<typename T> using ex = exclude<T>;

	template<typename... _Ts> struct pack {};

	template<typename T> struct is_exclude : std::false_type {};
	template<typename T> struct is_exclude<exclude<T>> : std::true_type {};
	template<typename T> constexpr bool is_exclude_v = is_exclude<T>::value;

	template<typename T> struct decay_exclude { using type = T; };
	template<typename T> struct decay_exclude<exclude<T>> { using type = T; };
	template<typename T> using decay_exclude_t = typename decay_exclude<T>::type;
}
