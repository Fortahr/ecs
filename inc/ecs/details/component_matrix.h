#pragma once

#include "../config.h"

namespace ecs::details
{
	template<typename _T>
	struct component_row
	{
	public:
		_T _elements[config::bucket_size];
	};

	template<typename... _Components>
	struct component_matrix : public component_row<_Components>...
	{
	};
}