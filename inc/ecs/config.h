#pragma once

#ifdef ECS_REGISTRY_INCLUDE
#include ECS_REGISTRY_INCLUDE
#endif

namespace ecs::config
{
#ifndef ECS_REGISTRY_CLASS
	static_assert(false, "Missing ECS registry, define the ECS_REGISTRY_CLASS macro or edit this config.");
#else
	typedef ECS_REGISTRY_CLASS Registry;
#endif

#ifndef ECS_BUCKET_SIZE
	constexpr size_t bucket_size = 64;
#else
	constexpr size_t bucket_size = ECS_BUCKET_SIZE;
#endif

	// entities use 32 bits to define the world they belong to and the reuse version, inclusive.
	// define how much of these bits are reserved for the world; 
	// 8:
	//   - Supports up to 256 worlds.
	//   - 16,777,216 reuses of the id before any false positives could theoretically happen.
	constexpr static size_t world_bits = 8;
}
