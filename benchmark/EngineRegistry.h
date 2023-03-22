#include <ecs/registry.h>

namespace Esteem
{
	typedef ecs::entity Entity;

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
}
