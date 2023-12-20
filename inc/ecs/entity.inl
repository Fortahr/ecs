#pragma once

#include "entity.h"

namespace ecs
{
	constexpr entity::entity(uint32_t id, uint32_t version, uint8_t world)
		: _id(id)
		, _version(version)
		, _world(world)
	{
		static_assert(sizeof(entity) == 8, "entity should be of byte size 8");
	}

	constexpr entity::entity()
		: _id(npos)
		, _version(0)
		, _world(0)
	{
	}

#if ENABLE_ENTITY_GET
	template<typename _T>
	inline _T* entity::get()
	{
		return world::get_entity_component<_T>(*this);
	}
#endif

	inline uint32_t entity::get_id() const
	{
		return _id;
	}

	inline uint8_t entity::get_world() const
	{
		return _world;
	}

	inline uint32_t entity::get_version() const
	{
		return _version;
	}

	inline bool entity::valid() const
	{
		return _id != npos;
	}

	inline void entity::invalidate()
	{
		_id = npos;
	}
}
