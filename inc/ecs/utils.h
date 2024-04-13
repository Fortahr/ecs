#pragma once

#include <type_traits>

namespace ecs
{
	class entity;

	// Exclude component from query.
	// Note that includes override excludes, e.g.: `world.query<ecs::exclude<C>>([](C& c) {});` will not exclude entities with type `C`
	template<typename _T> struct exclude {};
	template<typename _T> using ex = exclude<_T>;

	template<typename... _Ts> struct pack {};

	template<typename _T> struct is_exclude : std::false_type {};
	template<typename _T> struct is_exclude<exclude<_T>> : std::true_type {};
	template<typename _T> using is_exclude_t = typename is_exclude<_T>::type;
	template<typename _T> constexpr bool is_exclude_v = is_exclude<_T>::value;

	template<typename _T> struct decay_exclude { using type = _T; };
	template<typename _T> struct decay_exclude<exclude<_T>> { using type = _T; };
	template<typename _T> using decay_exclude_t = typename decay_exclude<_T>::type;

	template<typename _T> struct decay_non_entity : std::conditional<std::is_same_v<std::decay_t<_T>, entity>, std::remove_reference_t<_T>, std::decay_t<_T>> {};
	template<typename _T> using decay_non_entity_t = typename decay_non_entity<_T>::type;

	template<typename _T> struct is_entity : std::is_same<entity, _T> {};
	template<typename _T> using is_entity_t = typename is_entity<_T>::type;
	template<typename _T> constexpr bool is_entity_v = is_entity<_T>::value;

	template<typename _T, typename... _Ts> struct param_index;
	template<typename _T, typename... _Ts> struct param_index<_T, _T, _Ts...> : std::integral_constant<std::size_t, 0> {};
	template<typename _T, typename _T2, typename... _Ts> struct param_index<_T, _T2, _Ts...> : std::integral_constant<std::size_t, 1 + param_index<_T, _Ts...>::value> {};	
	template<typename _T> using param_index_t = typename param_index<_T>::type;
	template<typename _T> constexpr bool param_index_v = param_index<_T>::value;
}
