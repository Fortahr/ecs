#pragma once

#include <type_traits>

namespace ecs
{
	class entity;

	// Exclude component from query.
	// Note that includes override excludes, e.g.: `world.query<ecs::exclude<C>>([](C& c) {});` will not exclude entities with type `C`
	template<typename T> struct exclude {};
	template<typename T> using ex = exclude<T>;

	template<typename... _Ts> struct pack {};

	template<typename T> struct is_exclude : std::false_type {};
	template<typename T> struct is_exclude<exclude<T>> : std::true_type {};
	template<typename T> using is_exclude_t = typename is_exclude<T>::type;
	template<typename T> constexpr bool is_exclude_v = is_exclude<T>::value;

	template<typename T> struct decay_exclude { using type = T; };
	template<typename T> struct decay_exclude<exclude<T>> { using type = T; };
	template<typename T> using decay_exclude_t = typename decay_exclude<T>::type;

	// Mark entity as erasable in this query, it'll run a different loop that compensates for erasure of the current entity
	template<typename T, typename = std::enable_if_t<std::is_same_v<T, entity>>> struct erasable : public T { };

	template<typename T> struct is_erasable : std::false_type {};
	template<typename T> struct is_erasable<erasable<T>> : std::true_type {};
	template<typename T> using is_erasable_t = typename is_erasable<T>::type;
	template<typename T> constexpr bool is_erasable_v = is_erasable<T>::value;

	template<typename... _Ts> constexpr bool requires_erasable_query_v = (is_erasable_v<_Ts> || ...);

	template<typename T> struct is_entity : std::false_type {};
	template<> struct is_entity<entity> : std::true_type {};
	template<> struct is_entity<exclude<entity>> : std::true_type {};
	template<> struct is_entity<erasable<entity>> : std::true_type {};
	template<typename T> using is_entity_t = typename is_entity<T>::type;
	template<typename T> constexpr bool is_entity_v = is_entity<T>::value;
}
