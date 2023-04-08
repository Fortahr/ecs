#include <ecs/registry.h>

#include "Components.h"

namespace ecs
{
	class entity;
}

namespace Esteem
{
	typedef ecs::entity Entity;

	// Example on how to separate engine from game modules
	template<typename... _Components>
	using EngineRegistry = ecs::registry
		<
		struct Zero,
		struct One,
		struct Two,
		struct Three,
		struct Four,
		struct Five,
		_Components...
		>;

	typedef EngineRegistry<Six, Seven, Eight, Nine> GameRegistry;
}
