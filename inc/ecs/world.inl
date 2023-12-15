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

		return _archetypes.emplace_back().initialize<_Components...>();
	}

	template<typename... _Components>
	inline entity world::emplace_entity()
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

		auto& storage = emplace_archetype<_Components...>();
		auto storage_index = storage.emplace(entity(entity_version, entity_id, _world_index));

		mapping.set((details::archetype_storage<>*)&storage, storage_index);

		return entity(entity_id, entity_version, _world_index);
	}
		
	template<typename... _Components, typename>
	inline entity world::emplace_entity(_Components&&... move)
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

		auto& storage = emplace_archetype<_Components...>();
		auto storage_index = storage.emplace({ entity_version, entity_id }, std::forward<_Components>(move)...);

		mapping.set((details::archetype_storage<>*) & storage, storage_index);

		return entity(entity_id, entity_version, _world_index);
	}
		
	inline void world::erase_entity(entity entity)
	{
		details::entity_target entity_reference;
		if (get_entity(entity, entity_reference))
		{
			auto replaced = entity_reference._archetype->erase(entity_reference._index);

			entity_reference.invalidate();
			_entity_mapping_queue.push(entity.get_id());

			if (replaced.first != entity::npos)
			{
				_entity_mapping[replaced.first].move(replaced.second);
				_entity_mapping[replaced.second].invalidate();
			}
		}
	}

	/*template<typename... _Components>
	inline void world::reserve_entities(size_t size)
	{
		auto& storage = (details::archetype_storage<bucketSize, _Components...>&)emplace_archetype<_Components...>();
		storage.reserve(size);
	}*/
		
	template<typename _T>
	inline _T* world::get_entity_component_this_world(entity entity)
	{
		details::entity_target entity_reference;
		return get_entity(entity, entity_reference)
			? entity_reference._archetype->get_component<_T>(entity_reference)
			: nullptr;
	}
		
	template<typename _T>
	inline _T* world::get_entity_component(entity entity)
	{
		return entity.get_world() < _worlds.size()
			? _worlds[entity.get_world()]->get_entity_component_this_world<_T>(entity)
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
	__forceinline constexpr auto& world::get_argument(size_t i, const details::archetype_storage<>::bucket& bucket, uintptr_t offset)
	{
		uintptr_t matrix = reinterpret_cast<uintptr_t>(&bucket.components());
		return reinterpret_cast<std::decay_t<_Arg>*>(matrix + offset)[i];
	}

	template<>
	__forceinline constexpr auto& world::get_argument<entity>(size_t i, const details::archetype_storage<>::bucket& bucket, uintptr_t offset)
	{
		return bucket.get_entity(i);
	}

	template<typename _Func, typename... _Args>
	__forceinline constexpr void world::apply_to_bucket_entities(_Func&& func, size_t count, const details::archetype_storage<>::bucket& bucket, const std::array<uintptr_t, sizeof...(_Args)>& position)
	{
		for (size_t i = 0; i < count; ++i)
		{
			size_t index = 0;
			func(get_argument<_Args>(i, bucket, position[index++])...);
		}
	}

	template<typename _Func, typename... _Args>
	__forceinline constexpr void world::apply_to_archetype_entities(_Func&& func, details::archetype_storage<>& archetype)
	{
		const std::array<uintptr_t, sizeof...(_Args)> position{(archetype.component_offset<config::registry::template index_of<std::decay_t<_Args>>()>())...};
		const size_t lastbucketSize = archetype.size() % config::bucket_size;

		auto* bucket = archetype.get_buckets().data();
		const auto* endbucket = bucket + (archetype.size() / config::bucket_size);

		for (; bucket < endbucket; ++bucket)
			apply_to_bucket_entities<_Func, _Args...>(std::forward<_Func>(func), config::bucket_size, **bucket, position);

		// apply to the remaining in our last bucket, if it doesn't exist then `lastbucketSize` will be 0 and we won't run anything
		apply_to_bucket_entities<_Func, _Args...>(std::forward<_Func>(func), lastbucketSize, **bucket, position);
	}
	
	template<typename _Func, typename... _Args, typename... _Extra>
	inline void world::apply_to_qualifying_entities(_Func&& func, ecs::pack<_Extra...>)
	{
		// cache it, preventing .end() rereads on each iteration
		auto archetype = _archetypes.begin(), endArchetype = _archetypes.end();
		for (; archetype != endArchetype; ++archetype)
		{
			if (config::registry::template qualifies<_Args...>(archetype->component_mask(), ecs::pack<_Extra...>()))
				apply_to_archetype_entities<_Func, _Args...>(std::forward<_Func>(func), *archetype);
		}
	}
	
	template<typename... _Extra, typename _Func, typename _Ret, typename... _Args>
	inline void world::query(_Func&& func, _Ret(_Func::*)(_Args...) const)
	{
		apply_to_qualifying_entities<_Func, std::decay_t<_Args>...>(std::forward<_Func>(func), ecs::pack<_Extra...>());
	}
	
	template<typename... _Extra, typename _Func, typename _Ret, typename... _Args>
	inline void world::query(_Func&& func, _Ret(*)(_Args...))
	{
		apply_to_qualifying_entities<_Func, std::decay_t<_Args>...>(std::forward<_Func>(func), ecs::pack<_Extra...>());
	}
		
	template<typename... _Extra, typename _Func>
	inline void world::query(_Func&& func)
	{
		if constexpr (std::is_class_v<_Func>)
			query<_Extra...>(std::forward<_Func>(func), &_Func::operator());
		else
			query<_Extra...>(std::forward<_Func>(func), &func);
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
