#pragma once

#include "utils.h"

namespace ecs
{
	template<typename... _Components>
	class registry
	{
	public:
		// Get the registry index of the component
		template<typename _T, typename = std::enable_if_t<std::disjunction_v<std::is_same<_T, entity>, std::is_same<_T, _Components>...>>>
		constexpr static uint8_t index_of();

		// Create the bit mask of the given component
		template<typename _T>
		constexpr static size_t bit_mask_of();

		// Create the bit mask of the given components
		template<typename _T, typename _T2, typename... _Other>
		constexpr static size_t bit_mask_of();

		// Checks if components are within the archetype,
		// includes will override excludes, e.g.: Transform overrides Not<Transform>.
		template<typename... _Other, typename... _Extra>
		constexpr static bool qualifies(size_t mask, pack<_Extra...> = {});
	};
}

#include "registry.inl"
