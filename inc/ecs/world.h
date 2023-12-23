#pragma once

#include <vector>
#include <memory>
#include <queue>
#include <unordered_map>

#include "registry.h"
#include "config.h"
#include "details/archetype_storage.h"
#include "details/bucket_vector.h"
#include "details/fixed_vector.h"
#include "details/query_func.h"

namespace ecs
{
	class world
	{
	public:
		using world_store_type = std::conditional_t<ecs::config::world_inheritable, world*, world>;

		using world_index_type = 
			std::conditional_t<ecs::config::world_bits <= 8, uint8_t,
			std::conditional_t<ecs::config::world_bits <= 16, uint16_t,
			std::conditional_t<ecs::config::world_bits <= 32, uint32_t,
			uint64_t>>>;

	private:
		struct private_allocator : std::allocator<world>
		{
			template <typename _T> struct rebind { typedef private_allocator other; };

			template <typename _T, typename... _Args>
			static inline void construct(_T* ptr, _Args&&... args) { ::new ((void*)ptr) _T(std::forward<_Args>(args)...); }
		};

		using world_vector = std::conditional_t<ecs::config::world_fixed_vector != 0,
			details::fixed_vector<world_store_type, ecs::config::world_fixed_vector, private_allocator>,
			std::vector<world_store_type, private_allocator>>;

		using archetype_vector = std::conditional_t<ecs::config::archetype_fixed_vector != 0,
			details::fixed_vector<details::archetype_storage<>, ecs::config::archetype_fixed_vector>,
			details::bucket_vector<details::archetype_storage<>, 32>>;

		static world_vector _worlds;
		static std::queue<world_index_type> _world_index_queue;


	private:
		archetype_vector _archetypes;
		uint32_t _entity_max = 0;
		uint8_t _world_index;

		std::vector<details::entity_target> _entity_mapping;
		std::queue<uint32_t> _entity_mapping_queue;
				
		template <typename _Res>
		static _Res create_world_internal();

		std::pair<entity, details::entity_target&> allocate_entity();

		details::archetype_storage<>& runtime_emplace_archetype(size_t bitmask);

		template <typename = std::enable_if_t<!ecs::config::world_inheritable>>
		world(world_index_type index);

	public:
		template <typename = std::enable_if_t<ecs::config::world_inheritable>>
		world();

		template <typename = std::enable_if_t<ecs::config::world_inheritable>>
		world(world&& move);

		~world();

		static std::conditional_t<ecs::config::world_inheritable, world, world&> create_world();

		template<typename... _Components>
		details::archetype_storage<_Components...>& emplace_archetype();

		template<typename... _Components>
		entity emplace_entity();

		template<typename... _Components, typename = std::enable_if_t<(sizeof...(_Components) > 0)>>
		entity emplace_entity(_Components&&... move);

		bool erase_entity(entity entity);

		/*template<typename... _Components>
		inline void reserve_entities(size_t size);*/

		template<typename _Component, typename... _Args, typename = std::enable_if_t<ecs::config::registry::template contains<_Component>() && std::is_constructible_v<_Component, _Args...>>>
		bool add_entity_component(entity entity, _Args&&... args);

		template<typename _T>
		_T* get_entity_component(entity entity);

		template<typename _T>
		static _T* get_entity_component_any_world(entity entity);

	private:
		bool get_entity(entity entity, details::entity_target& target);

		template<typename _Arg>
		static constexpr decltype(auto) forward_argument(size_t i, const details::archetype_storage<>::bucket& bucket, uintptr_t offset);

		template<typename _Func, typename... _Args>
		static constexpr void apply_to_bucket_entities(const details::query_func<_Func, _Args...>& func, size_t count, const details::archetype_storage<>::bucket& bucket, const std::array<uintptr_t, sizeof...(_Args)>& position);

		template<typename _Func, typename... _Args>
		static constexpr void apply_to_archetype_entities(const details::query_func<_Func, _Args...>& func, details::archetype_storage<>& archetype);

		template<typename _Func, typename... _Args>
		static constexpr void apply_to_archetype_entities_mutable(const details::query_func<_Func, _Args...>& func, details::archetype_storage<>& archetype);

		template<typename _Func, typename... _Args, typename... _Extra>
		void apply_to_qualifying_entities(const details::query_func<_Func, _Args...>& func, ecs::pack<_Extra...> = {});

		template<typename _Func, typename... _Args, typename... _Extra>
		void apply_to_qualifying_entities_mutable(const details::query_func<_Func, _Args...>& func, ecs::pack<_Extra...> = {});

	public:
		template<typename... _Extra, typename _Func>
		void query(_Func&& func);

		template<typename... _Extra, typename _Func>
		void query_mutable(_Func&& func);

		template<typename... _Components>
		size_t count();
	};
}

#include "world.inl"
