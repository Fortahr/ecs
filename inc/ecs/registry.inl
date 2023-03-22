#pragma once

#include "entity.h"
#include "registry.h"

namespace ecs
{
	template<typename... _Components>
	template<typename _T, typename>
	constexpr static uint8_t registry<_Components...>::index_of()
	{
		if constexpr (std::is_same_v<_T, entity>)
			return 0; // allow, but ignore

		uint8_t index = 0;
		((!std::is_same_v<_T, _Components> && ++index) && ...);
		return index;
	}

	template<typename... _Components>
	template<typename _T>
	constexpr static size_t registry<_Components...>::bit_mask_of()
	{
		if constexpr (std::is_same_v<_T, entity> || is_exclude_v<_T>)
			return 0; // allow, but ignore
		else
			return 1ull << index_of<_T>();
	}

	template<typename... _Components>
	template<typename _T, typename _T2, typename... _Other>
	constexpr static size_t registry<_Components...>::bit_mask_of()
	{
		return ((bit_mask_of<_T>() | bit_mask_of<_T2>()) | ... | bit_mask_of<_Other>());
	}

	template<typename... _Components>
	template<typename... _Other, typename... _Extra>
	constexpr static bool registry<_Components...>::qualifies(size_t mask, pack<_Extra...>)
	{
		// if after masking any exclude bit is still present then the `== include` will fail
		constexpr size_t include = (0 | ... | bit_mask_of<_Other>()) | (0 | ... | bit_mask_of<_Extra>());
		constexpr size_t both = (0 | ... | bit_mask_of<decay_exclude_t<_Other>>()) | (0 | ... | bit_mask_of<decay_exclude_t<_Extra>>());
		return (mask & both) == include;
	}
}
