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
		constexpr static size_t size();

		// Get the registry index of the component
		template<typename _T, typename = std::enable_if_t<std::disjunction_v<is_entity<_T>, std::is_same<_T, _Components>...>>>
		constexpr static uint8_t index_of();

		// Get the registry index of the component
		template<typename _T>
		constexpr static bool contains();

		// Create the bit mask of the given component
		template<typename _T>
		constexpr static size_t bit_mask_of();

		// Create the bit mask of the given components
		template<typename _T, typename _T2, typename... _Other>
		constexpr static size_t bit_mask_of();

		// Checks if components are within the archetype,
		// includes will override excludes, e.g.: C overrides exclude<C>.
		template<typename... _Other, typename... _Extra>
		constexpr static bool qualifies(size_t mask, pack<_Extra...> = {});
	};
}

#include "registry.inl"
