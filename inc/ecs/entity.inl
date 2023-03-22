#pragma once

#include "entity.h"

namespace ecs
{
	entity::entity(uint32_t id, uint32_t version, uint8_t world)
		: _id(id)
		, _version(version)
		, _world(world)
	{
		static_assert(sizeof(entity) == 8, "entity should be of byte size 8");
	}

	entity::entity()
		: _id(npos)
		, _version(0)
		, _world(0)
	{
	}

	inline uint32_t entity::get_id()
	{
		return _id;
	}

	inline uint8_t entity::get_world()
	{
		return _world;
	}

	inline uint32_t entity::get_version()
	{
		return _version;
	}

	inline bool entity::valid()
	{
		return _id != npos;
	}

	inline void entity::invalidate()
	{
		_id = npos;
	}
}
