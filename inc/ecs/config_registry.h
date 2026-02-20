#pragma once

#include "config.h"

#ifdef ECS_REGISTRY_INCLUDE
#include ECS_REGISTRY_INCLUDE
#endif

#ifndef ECS_REGISTRY_CLASS
static_assert(false, "Missing ECS registry, define the ECS_REGISTRY_CLASS macro or edit this config file.");
#endif

namespace ecs::config
{
	// The global registry, should be (a sub-class of) ecs::registry<...>
	typedef ECS_REGISTRY_CLASS registry;
}
