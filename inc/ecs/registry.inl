#pragma once

#include "entity.h"
#include "registry.h"

namespace ecs
{
	template<typename... _Components>
	constexpr size_t registry<_Components...>::size()
	{
		return count;
	}

	template<typename... _Components>
	template<typename... _Other, typename... _Extra>
	constexpr bool registry<_Components...>::qualifies(size_t mask, pack<_Extra...>)
	{
		// if after masking any exclude bit is still present then the `== include` will fail
		constexpr size_t include = (0 | ... | bit_mask_of<_Other>) | (0 | ... | bit_mask_of<_Extra>);
		constexpr size_t both = (0 | ... | bit_mask_of<decay_exclude_t<_Other>>) | (0 | ... | bit_mask_of<decay_exclude_t<_Extra>>);
		return (mask & both) == include;
	}
}
