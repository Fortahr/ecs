#pragma once

#include <cstdint>

#include "config.h"

#if !__has_include("world.h")
#include "world.h"
#define ENABLE_ENTITY_GET true
#endif

namespace ecs
{
	class world;

	namespace details
	{
		template<typename...> class archetype_storage;
	}

	class entity
	{
		friend class world;
		template<typename...> friend class details::archetype_storage;

	private:
		union
		{
			struct
			{
				uint32_t _version : 32 - config::world_bits;
				uint32_t _world : config::world_bits;
			};

			// Quick uint32 == uint32 (version + world) check.
			uint32_t _equality;
		};

		// External id, used in lookup table.
		uint32_t _id;

	public:
		// get_id() == npos when it's not used/occupied.
		static constexpr uint32_t npos = ~0;

	private:
		constexpr entity(uint32_t id, uint32_t version, uint8_t world);

		void invalidate();

	public:
		constexpr entity();

#if ENABLE_ENTITY_GET
		template<typename _T>
		_T* get();
#endif

		uint32_t get_id() const;

		uint32_t get_version() const;

		uint8_t get_world() const;

		bool valid() const;

		inline bool operator==(const entity& r) const
		{
			return this->_equality == r._equality;
		}
	};

	static_assert(sizeof(entity) == sizeof(uint64_t));
}

#include "entity.inl"

#undef ENABLE_ENTITY_GET
