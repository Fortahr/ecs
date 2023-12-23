#pragma once

#include "world.h"

#include <vector>
#include <memory>
#include <queue>
#include <cassert>
#include <cmath>

#include "entity.h"
#include "registry.h"
#include "details/archetype_storage.h"

namespace ecs
{
	inline decltype(world::_worlds) world::_worlds;
	inline decltype(world::_world_index_queue) world::_world_index_queue;

	template <typename>
	world::world(world_index_type index)
		: _world_index(index)
	{
	}

	template <typename>
	world::world()
	{
		if (_world_index_queue.empty())
		{
			_world_index = world_index_type(_worlds.size());
			assert(_world_index < (1 << config::world_bits));
			_worlds.emplace_back(this);
		}
		else
		{
			auto index = _world_index = _world_index_queue.front();
			_world_index_queue.pop();
			_worlds[index] = this;
		}
	}

	template <typename>
	world::world(world&& move)
		: _archetypes(std::move(move._archetypes))
		, _entity_max(std::move(move._entity_max))
		, _world_index(std::move(move._world_index))
		, _entity_mapping(std::move(move._entity_mapping))
		, _entity_mapping_queue(std::move(move._entity_mapping_queue))
	{
		if constexpr (ecs::config::world_inheritable)
			_worlds[_world_index] = this;
	}

	template <typename _Res>
	static _Res world::create_world_internal()
	{
		if constexpr (std::is_same_v<_Res, world>)
			return world();
		else
		{
			if (_world_index_queue.empty())
			{
				auto index = world_index_type(_worlds.size());
				assert(index < (1 << config::world_bits));

				return _worlds.emplace_back(index);
			}
			else
			{
				auto index = _world_index_queue.front();
				_world_index_queue.pop();

				auto& w = _worlds[index];
				w.~world();
				new (&w) world(index);

				return w;
			}
		}
	}

	std::conditional_t<ecs::config::world_inheritable, world, world&> world::create_world()
	{
		return create_world_internal<std::conditional_t<ecs::config::world_inheritable, world, world&>>();
	}

	world::~world()
	{
		_world_index_queue.push(_world_index);
	}
		
	template<typename... _Components>
	inline details::archetype_storage<_Components...>& world::emplace_archetype()
	{
		constexpr auto bitmask = config::registry::template bit_mask_of<_Components...>();
		for (auto& archetype : _archetypes)
		{
			if (archetype.component_mask() == bitmask)
				return reinterpret_cast<details::archetype_storage<_Components...>&>(archetype);
		}

		auto& archetype = _archetypes.emplace_back();
		archetype.initialize<_Components...>();

		return reinterpret_cast<details::archetype_storage<_Components...>&>(archetype);
	}

	inline details::archetype_storage<>& world::runtime_emplace_archetype(size_t bitmask)
	{
		for (auto& archetype : _archetypes)
		{
			if (archetype.component_mask() == bitmask)
				return static_cast<details::archetype_storage<>&>(archetype);
		}

		auto& archetype = _archetypes.emplace_back();
		archetype.runtime_initialize(bitmask, config::registry::components());

		return archetype;
	}

	std::pair<entity, details::entity_target&> world::allocate_entity()
	{
		uint32_t entity_id;
		if (_entity_mapping_queue.empty())
		{
			entity_id = _entity_max++;

			if (entity_id >= _entity_mapping.size())
				_entity_mapping.resize(entity_id + 1ull);
		}
		else
		{
			entity_id = _entity_mapping_queue.front();
			_entity_mapping_queue.pop();
		}

		auto& mapping = _entity_mapping[entity_id];
		auto entity_version = mapping._version;

		return { entity(entity_id, entity_version, _world_index), mapping };
	}

	template<typename... _Components>
	inline entity world::emplace_entity()
	{
		auto [entity, mapping] = allocate_entity();

		auto& storage = emplace_archetype<_Components...>();
		auto storage_index = storage.emplace(entity);

		mapping.set(reinterpret_cast<details::archetype_storage<>*>(&storage), storage_index);

		return entity;
	}
		
	template<typename... _Components, typename>
	inline entity world::emplace_entity(_Components&&... move)
	{
		auto [entity, mapping] = allocate_entity();

		auto& storage = emplace_archetype<_Components...>();
		auto storage_index = storage.emplace(entity, std::forward<_Components>(move)...);

		mapping.set(static_cast<details::archetype_storage<>*>(&storage), storage_index);

		return entity;
	}
		
	inline bool world::erase_entity(entity entity)
	{
		details::entity_target entity_reference;
		if (get_entity(entity, entity_reference))
		{
			_entity_mapping_queue.push(entity.get_id());
			_entity_mapping[entity.get_id()].invalidate();

			auto replaced = entity_reference._archetype->erase(entity_reference._index);
			if (replaced != entity::npos)
				_entity_mapping[replaced].move(entity_reference._index);

			return true;
		}

		return false;
	}

	/*template<typename... _Components>
	inline void world::reserve_entities(size_t size)
	{
		auto& storage = (details::archetype_storage<bucketSize, _Components...>&)emplace_archetype<_Components...>();
		storage.reserve(size);
	}*/

	template<typename _Component, typename... _Args, typename>
	bool world::add_entity_component(entity entity, _Args&&... args)
	{
		details::entity_target entity_reference;
		if (get_entity(entity, entity_reference))
		{
			auto mask = entity_reference._archetype->component_mask();
			auto new_mask = mask | ecs::config::registry::template bit_mask_of<_Component>();

			if (new_mask != mask)
			{
				auto& archetype = runtime_emplace_archetype(new_mask);
				auto [ newIndex, bucket, replaced ] = entity_reference._archetype->runtime_move(entity_reference._index, archetype,	config::registry::components());

				size_t component_offset = archetype.component_offset(ecs::config::registry::template index_of<_Component>());
				auto& component = bucket->get_unsafe<_Component>(component_offset, newIndex % config::bucket_size);
				new (&component) _Component(std::forward<_Args>(args)...);

				_entity_mapping[entity.get_id()].move(newIndex, archetype);

				if (replaced != entity::npos)
					_entity_mapping[replaced].move(entity_reference._index);

				return true;
			}
		}

		return false;
	}
		
	template<typename _T>
	inline _T* world::get_entity_component(entity entity)
	{
		details::entity_target entity_reference;
		return get_entity(entity, entity_reference)
			? entity_reference._archetype->get_component<_T>(entity_reference)
			: nullptr;
	}
		
	template<typename _T>
	inline _T* world::get_entity_component_any_world(entity entity)
	{
		return entity.get_world() < _worlds.size()
			? _worlds[entity.get_world()]->get_entity_component<_T>(entity)
			: nullptr;
	}
	
	inline bool world::get_entity(entity entity, details::entity_target& target)
	{
		if (entity.get_id() < _entity_mapping.size())
		{
			target = _entity_mapping[entity.get_id()];
			return entity._equality == target._version;
		}

		return false;
	}

	template<typename _Arg>
	__forceinline constexpr decltype(auto) world::forward_argument(size_t i, const details::archetype_storage<>::bucket& bucket, uintptr_t offset)
	{
		uintptr_t matrix = reinterpret_cast<uintptr_t>(&bucket.components());
		return reinterpret_cast<_Arg*>(matrix + offset)[i];
	}

	template<>
	__forceinline constexpr decltype(auto) world::forward_argument<entity>(size_t i, const details::archetype_storage<>::bucket& bucket, uintptr_t offset)
	{
		return (bucket.get_entity(i));
	}

	template<typename _Func, typename... _Args>
	__forceinline constexpr void world::apply_to_bucket_entities(const details::query_func<_Func, _Args...>& func, size_t count, const details::archetype_storage<>::bucket& bucket, const std::array<uintptr_t, sizeof...(_Args)>& position)
	{
		for (size_t i = 0; i < count; ++i)
		{
			size_t p = 0;
#pragma warning( suppress : 28020 ) // MSVC code analyzer shows false positives on std::array[p < n]
			func(forward_argument<_Args>(i, bucket, position[p++])...);
		}
	}

	template<typename _Func, typename... _Args>
	__forceinline constexpr void world::apply_to_archetype_entities(const details::query_func<_Func, _Args...>& func, details::archetype_storage<>& archetype)
	{
		const std::array<uintptr_t, sizeof...(_Args)> position{(archetype.component_offset<config::registry::template index_of<_Args>()>())...};
		const size_t lastbucketSize = archetype.size() % config::bucket_size;

		auto* bucket = archetype.get_buckets().data();
		const auto* endbucket = bucket + (archetype.size() / config::bucket_size);

		for (; bucket < endbucket; ++bucket)
			apply_to_bucket_entities(func, config::bucket_size, **bucket, position);

		// apply to the remaining in our last bucket, if it doesn't exist then `lastbucketSize` will be 0 and we won't run anything
		apply_to_bucket_entities(func, lastbucketSize, **bucket, position);
	}
	
	template<typename _Func, typename... _Args>
	constexpr void world::apply_to_archetype_entities_mutable(const details::query_func<_Func, _Args...>& func, details::archetype_storage<>& archetype)
	{
		if (archetype.size() == 0)
			return;

		const std::array<uintptr_t, sizeof...(_Args)> position{ (archetype.component_offset<config::registry::template index_of<_Args>()>())... };

		auto* bucket = archetype.get_buckets().data();

		for (size_t count = 0, i = 0; ; )
		{
			const entity& ent = (*bucket)->get_entity(i);
			auto prevId = ent.get_id();

			size_t p = 0;
#pragma warning( suppress : 28020 ) // MSVC code analyzer shows false positives on std::array[p < n]
			func(forward_argument<_Args>(i, **bucket, position[p++])...);

			if (count >= archetype.size())
				return;
			else if (prevId == ent.get_id())
			{
				if (++count >= archetype.size())
					return;

				if (++i >= config::bucket_size)
				{
					i = 0;
					++bucket;
				}
			}
		}
	}

	template<typename _Func, typename... _Args, typename... _Extra>
	inline void world::apply_to_qualifying_entities(const details::query_func<_Func, _Args...>& func, ecs::pack<_Extra...>)
	{
		// cache it, preventing .end() rereads on each iteration
		auto archetype = _archetypes.begin(), endArchetype = _archetypes.end();
		for (; archetype != endArchetype; ++archetype)
		{
			if (config::registry::template qualifies<_Args...>(archetype->component_mask(), ecs::pack<_Extra...>()))
				apply_to_archetype_entities(func, *archetype);
		}
	}

	template<typename _Func, typename... _Args, typename... _Extra>
	inline void world::apply_to_qualifying_entities_mutable(const details::query_func<_Func, _Args...>& func, ecs::pack<_Extra...>)
	{
		// TODO: ignore any moved entities to archetypes later in the chain
		
		// cache it, preventing .end() rereads on each iteration
		auto archetype = _archetypes.begin(), endArchetype = _archetypes.end();
		for (; archetype != endArchetype; ++archetype)
		{
			if (config::registry::template qualifies<_Args...>(archetype->component_mask(), ecs::pack<_Extra...>()))
				apply_to_archetype_entities_mutable(func, *archetype);
		}
	}

	template<typename... _Extra, typename _Func>
	inline void world::query(_Func&& func)
	{
		apply_to_qualifying_entities(details::to_query_func(std::forward<_Func>(func)), ecs::pack<_Extra...>());
	}

	template<typename... _Extra, typename _Func>
	inline void world::query_mutable(_Func&& func)
	{
		apply_to_qualifying_entities_mutable(details::to_query_func(std::forward<_Func>(func)), ecs::pack<_Extra...>());
	}
	
	template<typename... _Extra>
	inline size_t world::count()
	{
		size_t count = 0;

		auto archetype = _archetypes.begin(), endArchetype = _archetypes.end();
		for (; archetype != endArchetype; ++archetype)
		{
			if (config::registry::template qualifies<_Extra...>(archetype->component_mask()))
				count += archetype->size();
		}

		return count;
	}
}
