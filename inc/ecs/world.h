#pragma once

#include <array>
#include <vector>
#include <memory>
#include <queue>

#include "registry.h"
#include "config.h"
#include "details/archetype_storage.h"

namespace ecs
{
	class world
	{
	private:
		std::list<details::archetype_storage<>> _archetypes;
		uint32_t _entity_max = 0;

		uint8_t _world_index;
		static std::vector<world*> _worlds;
		static std::queue<uint8_t> _world_index_queue;

		std::vector<details::entity_target> _entity_mapping;
		std::queue<uint32_t> _entity_mapping_queue;

	public:
		world();
		~world();

		template<typename... _Components>
		details::archetype_storage<_Components...>& emplace_archetype();

		template<typename... _Components>
		entity emplace_entity();

		template<typename... _Components, typename = std::enable_if_t<(sizeof...(_Components) > 0)>>
		entity emplace_entity(_Components&&... move);

		void erase_entity(entity entity);

		/*template<typename... _Components>
		inline void reserve_entities(size_t size);*/

		template<typename _T>
		_T* get_entity_component_this_world(entity entity);

		template<typename _T>
		static _T* get_entity_component(entity entity);

	private:
		bool get_entity(entity entity, details::entity_target& target);

		template<typename... _Components, std::size_t... _Indices, class _Func>
		constexpr void apply_to_archetype_entities(_Func&& func, details::archetype_storage<>& archetype, std::index_sequence<_Indices...>);

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
