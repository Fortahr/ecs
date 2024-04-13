#pragma once

#include "utils.h"

namespace ecs
{
	class entity;

	template<typename... _Components>
	class registry
	{
	public:
		using components = pack<_Components...>;

		constexpr static uint16_t _component_size[sizeof...(_Components)] = { sizeof(_Components)... };

		// Get the amount of components
		constexpr static size_t count = sizeof...(_Components);

		// Get the amount of components, STL style
		constexpr static size_t size();

		// Get the registry index of the component
		template<typename _T, typename = std::enable_if_t<std::disjunction_v<is_entity<_T>, std::is_same<_T, _Components>...>>>
		constexpr static uint8_t index_of = std::conditional_t<is_entity_v<_T>, std::integral_constant<size_t, 0>, ::ecs::param_index<_T, _Components...>>::value;

		// Get the registry index of the component
		template<typename _T>
		constexpr static bool contains = std::disjunction_v<std::is_same<_T, _Components>...>;

		// Create the bit mask of the given components
		template<typename... _Ts>
		constexpr static size_t bit_mask_of = (0 | ... | bit_mask_of<_Ts>);

		// Bit mask specialization
		template<typename _T> constexpr static size_t bit_mask_of<_T> = 1ull << index_of<_T>;
		template<typename _T> constexpr static size_t bit_mask_of<::ecs::exclude<_T>> = 0;
		template<> constexpr static size_t bit_mask_of<::ecs::entity> = 0;

		// Checks if components are within the archetype,
		// includes will override excludes, e.g.: C overrides exclude<C>.
		template<typename... _Other, typename... _Extra>
		constexpr static bool qualifies(size_t mask, pack<_Extra...> = {});
	};
}

#include "registry.inl"
