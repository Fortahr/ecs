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
	private:
		template <bool _Inheritable>
		struct world_storage_internal_t
		{
			using store_t = std::conditional_t<_Inheritable, world*, world>;
			using create_t = std::conditional_t<_Inheritable, world, world&>;
		};

	public:
		using world_storage_type = world_storage_internal_t<ecs::config::world_inheritable>;

		using world_index_type = 
			std::conditional_t<ecs::config::world_bits <= 8, uint8_t,
			std::conditional_t<ecs::config::world_bits <= 16, uint16_t,
			std::conditional_t<ecs::config::world_bits <= 32, uint32_t,
			uint64_t>>>;

	private:
		template <typename _T>
		struct private_allocator : public std::allocator<_T>
		{
		public:
			template <typename _Other> struct rebind { typedef private_allocator<_Other> other; };
			
			using std::allocator<_T>::allocator;

			template <typename _T2, typename... _Args>
			static inline void construct(_T2* ptr, _Args&&... args) { ::new ((void*)ptr) _T(std::forward<_Args>(args)...); }
		};

		template <typename _W> using world_vector_fixed = details::fixed_vector<_W, ecs::config::world_fixed_vector, private_allocator<_W>>;
		template <typename _W> using world_vector_dynamic = std::vector<_W, private_allocator<_W>>;
		template <typename _W> using world_vector_t = std::conditional_t<ecs::config::world_fixed_vector != 0, world_vector_fixed<_W>, world_vector_dynamic<_W>>;
		using world_vector_type = world_vector_t<world_storage_type::store_t>;

		using archetype_vector_fixed = details::fixed_vector<details::archetype_storage<>, ecs::config::archetype_fixed_vector>;
		using archetype_vector_dynamic = details::bucket_vector<details::archetype_storage<>, 32>;
		using archetype_vector_type = std::conditional_t<ecs::config::archetype_fixed_vector != 0, archetype_vector_fixed, archetype_vector_dynamic>;

		static world_vector_type _worlds;
		static std::queue<world_index_type> _world_index_queue;

	private:
		archetype_vector_type _archetypes;
		uint32_t _entity_max = 0;
		uint8_t _world_index;

		std::vector<details::entity_target> _entity_mapping;
		std::queue<uint32_t> _entity_mapping_queue;

		template<bool _Inheritable>
		static typename world_storage_internal_t<_Inheritable>::create_t create_world_internal(world_vector_t<typename world_storage_internal_t<_Inheritable>::store_t>& worlds);

		std::pair<entity, details::entity_target&> allocate_entity();

		details::archetype_storage<>& runtime_emplace_archetype(size_t bitmask);

		world(world_index_type index);

	public:
		template <typename = std::enable_if_t<ecs::config::world_inheritable>>
		world();

		template <typename = std::enable_if_t<ecs::config::world_inheritable>>
		world(world&& move);

		~world();

		static world_storage_type::create_t create_world();

		template<typename... _Components>
		details::archetype_storage<_Components...>& emplace_archetype();

		template<typename... _Components>
		entity emplace_entity();

		template<typename... _Components, typename = std::enable_if_t<(sizeof...(_Components) > 0)>>
		entity emplace_entity(_Components&&... move);

		bool erase_entity(entity entity);

		/*template<typename... _Components>
		inline void reserve_entities(size_t size);*/

		template<typename _Component, typename... _Args, typename = std::enable_if_t<ecs::config::registry::template contains<_Component> && std::is_constructible_v<_Component, _Args...>>>
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
