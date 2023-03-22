#pragma once

#include <array>
#include <vector>
#include <memory>
#include <queue>

#include "registry.h"
#include "components.h"
#include "archetype_storage.h"

namespace ecs
{
	template<typename _Registry, size_t _BucketSize = 64>
	class world
	{
	private:
		std::vector<details::archetype_storage<_BucketSize>> _archetypes;
		uint32_t _entity_max = 0;
		uint8_t _world_index;

		std::vector<details::entity_target> _entity_mapping;
		std::queue<uint32_t> _entity_mapping_queue;

	public:
		world() = default;

		template<typename... _Components>
		details::archetype_storage<_BucketSize, _Components...>& emplace_archetype();

		template<typename... _Components>
		entity emplace_entity();

		template<typename... _Components, typename = std::enable_if_t<(sizeof...(_Components) > 0)>>
		entity emplace_entity(_Components&&... move);

		void erase_entity(entity entity);

		/*template<typename... _Components>
		inline void reserve_entities(size_t size);*/

	private:
		template<typename... _Components, std::size_t... _Indices, class _Func>
		constexpr void apply_to_archetype_entities(_Func&& func, details::archetype_storage<_BucketSize>& archetype, std::index_sequence<_Indices...>);

		template<typename _Func, typename... _Args, typename... _Extra>
		void apply_to_qualifying_entities(_Func&& func, ecs::pack<_Extra...> = {});

		template<typename... _Extra, typename _Func, typename _Ret, typename... _Args>
		void query(_Func&& func, _Ret(_Func::*)(_Args...) const);

		template<typename... _Extra, typename _Func, typename _Ret, typename... _Args>
		void query(_Func&& func, _Ret(*)(_Args...));

	public:
		template<typename... _Extra, typename _Func>
		void query(_Func&& func);

		template<typename... _Components>
		size_t count();
	};
}

#include "world.inl"
