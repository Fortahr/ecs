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
	inline std::vector<world*> world::_worlds;
	inline std::queue<uint8_t> world::_world_index_queue;

	world::world()
	{
		if (_world_index_queue.empty())
		{
			_world_index = uint8_t(_worlds.size());
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

	world::~world()
	{
		_world_index_queue.push(_world_index);
	}
		
	template<typename... _Components>
	inline details::archetype_storage<_Components...>& world::emplace_archetype()
	{
		constexpr auto bitmask = config::Registry::template bit_mask_of<_Components...>();
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

		mapping.set((details::archetype_storage<>*) & storage, storage_index);

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
		else
			return false;
	}
		
	template<typename... _Components, std::size_t... _Indices, class _Func>
	__forceinline constexpr void world::apply_to_archetype_entities(_Func&& func, details::archetype_storage<>& archetype, std::index_sequence<_Indices...>)
	{
		const uintptr_t position[]{ (archetype.component_offset<config::Registry::template index_of<std::decay_t<_Components>>()>())... };
		const size_t lastbucketSize = archetype.size() % config::bucket_size;

		auto* bucket = archetype.get_buckets().data();
		const auto* endbucket = bucket + (archetype.size() / config::bucket_size);

		for (; bucket < endbucket; ++bucket)
		{
			const uintptr_t componentStart = uintptr_t(&(*bucket)->components());
			for (size_t i = 0; i < config::bucket_size; ++i)
				func(conditional_v<std::is_same_v<_Components, entity>, const entity, _Components>(
					(*bucket)->get_entity(i),
					reinterpret_cast<std::decay_t<_Components>*>(componentStart + position[_Indices])[i])
					...);
		}

		// won't run anything if it doesn't exist, as that'll run it `lastbucketSize` times, which will be 0
		const uintptr_t componentStart = uintptr_t(&(*bucket)->components());
		for (size_t i = 0; i < lastbucketSize; ++i)
			func(conditional_v<std::is_same_v<_Components, entity>, const entity, _Components>(
				(*bucket)->get_entity(i),
				reinterpret_cast<std::decay_t<_Components>*>(componentStart + position[_Indices])[i])
				...);
	}
		
	template<typename _Func, typename... _Args, typename... _Extra>
	inline void world::apply_to_qualifying_entities(_Func&& func, ecs::pack<_Extra...>)
	{
		// cache it, preventing .end() rereads on each iteration
		auto archetype = _archetypes.begin(), endArchetype = _archetypes.end();
		for (; archetype != endArchetype; ++archetype)
		{
			if (config::Registry::template qualifies<_Args...>(archetype->component_mask(), ecs::pack<_Extra...>()))
				apply_to_archetype_entities<_Args...>(func, *archetype, std::make_index_sequence<sizeof...(_Args)>());
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
			if (config::Registry::template qualifies<_Extra...>(archetype->component_mask()))
				count += archetype->size();
		}

		return count;
	}
}
