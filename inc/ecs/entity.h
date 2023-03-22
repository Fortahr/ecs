#pragma once

#include <cstdint>

namespace ecs
{
	template<typename _Registry, size_t _BucketSize>
	class world;

	struct entity
	{
		template<typename, size_t> friend class world;

	private:
		union
		{
			struct
			{
				// 16,777,216 reuses of the id before any false positives could theoretically happen.
				uint32_t _version : 24;

				// Supports up to 256 worlds.
				uint32_t _world : 8;
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

		uint32_t get_id();

		uint32_t get_version();

		uint8_t get_world();

		bool valid();

		void invalidate();
	};
}

#include "entity.inl"
