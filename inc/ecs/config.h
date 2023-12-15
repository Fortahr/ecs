#pragma once

#ifdef ECS_REGISTRY_INCLUDE
#include ECS_REGISTRY_INCLUDE
#endif

#ifndef ECS_BUCKET_SIZE
#define ECS_BUCKET_SIZE 64
#endif

#ifndef ECS_REGISTRY_CLASS
static_assert(false, "Missing ECS registry, define the ECS_REGISTRY_CLASS macro or edit this config file.");
#endif

namespace ecs::config
{
	// The global registry, should be (a sub-class of) ecs::registry<...>
	typedef ECS_REGISTRY_CLASS registry;

	// Amount of entities (their components) that will be stored in each bucket
	constexpr size_t bucket_size = ECS_BUCKET_SIZE;

	// Entities use 32 bits to define the world they belong to and the reuse version, inclusive.
	// define how much of these bits are reserved for the world; 
	// 8:
	//   - Supports up to 256 worlds.
	//   - 16,777,216 reuses of the id before any false positives could theoretically happen.
	constexpr size_t world_bits = 8;

	// Worlds are saved as pointers and can be inherited, put to false to store them in an array directly.
	constexpr bool world_inheritable = false;

	// If > 0 then worlds are saved in a fixed sized array in the world object, otherwise they are stored with std::vector.
	constexpr size_t world_fixed_vector = 1;

	// If > 0 then archetypes are saved in a fixed sized array in the world object, otherwise they are stored with std::vector.
	constexpr size_t archetype_fixed_vector = 0;

	// Checks
	static_assert(world_fixed_vector < (1 << world_bits), "world_fixed_vector must fit within an integer of size world_bits");
}
