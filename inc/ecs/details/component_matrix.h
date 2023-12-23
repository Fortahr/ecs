#pragma once

#include "../config.h"

namespace ecs::details
{
	template<typename _T>
	struct component_row
	{
		_T _elements[config::bucket_size];

		_T& operator[](size_t index)
		{
			return _elements[index];
		}
	};

	template<typename... _Components>
	struct component_matrix : public component_row<_Components>...
	{
	};
}