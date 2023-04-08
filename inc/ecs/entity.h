#pragma once

#include <cstdint>

#include "config.h"

namespace ecs
{
	class world;

	class entity
	{
		friend class world;

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
		entity(uint32_t id, uint32_t version, uint8_t world);

	public:
		entity();

		template<typename _T>
		_T* get();

		uint32_t get_id();

		uint32_t get_version();

		uint8_t get_world();

		bool valid();

		void invalidate();
	};
}

#include "entity.inl"
