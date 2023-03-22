#pragma once

#include "world.h"

#pragma once

#include <array>
#include <vector>
#include <memory>
#include <queue>

#include "entity.h"
#include "registry.h"
#include "components.h"
#include "archetype_storage.h"

namespace ecs
{

	template<typename _Registry, size_t _BucketSize>
	template<typename... _Components>
	inline details::archetype_storage<_BucketSize, _Components...>& world<_Registry, _BucketSize>::emplace_archetype()
	{
		constexpr auto bitmask = _Registry::template bit_mask_of<_Components...>();
		for (auto& archetype : _archetypes)
		{
			if (archetype.component_mask() == bitmask)
				return reinterpret_cast<details::archetype_storage<_BucketSize, _Components...>&>(archetype);
		}

		return _archetypes.emplace_back().initialize<_Registry, _Components...>();
	}

	template<typename _Registry, size_t _BucketSize>
	template<typename... _Components>
	inline entity world<_Registry, _BucketSize>::emplace_entity()
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

		mapping.set((details::archetype_storage<64>*)&storage, storage_index);

		return entity(entity_id, entity_version, _world_index);
	}

	template<typename _Registry, size_t _BucketSize>
	template<typename... _Components, typename>
	inline entity world<_Registry, _BucketSize>::emplace_entity(_Components&&... move)
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

		mapping.set((details::archetype_storage<64>*) & storage, storage_index);

		return entity(entity_id, entity_version, _world_index);
	}

	template<typename _Registry, size_t _BucketSize>
	inline void world<_Registry, _BucketSize>::erase_entity(entity _entity)
	{
		auto entity_reference = _entity_mapping[_entity.get_id()];
		if (_entity._equality == entity_reference._version)
		{
			auto replaced = entity_reference.archetype->erase(entity_reference._index);

			entity_reference.remove();
			_entity_mapping_queue.push(_entity.get_id());

			if (replaced.first != entity::npos)
			{
				_entity_mapping[replaced.first].move(replaced.second);
				_entity_mapping[replaced.second].invalidate();
			}
		}
	}

	/*template<typename... _Components>
	inline void world<_Registry, _BucketSize>::reserve_entities(size_t size)
	{
		auto& storage = (details::archetype_storage<bucketSize, _Components...>&)emplace_archetype<_Components...>();
		storage.reserve(size);
	}*/

	template<typename _Registry, size_t _BucketSize>
	template<typename... _Components, std::size_t... _Indices, class _Func>
	__forceinline constexpr void world<_Registry, _BucketSize>::apply_to_archetype_entities(_Func&& func, details::archetype_storage<_BucketSize>& archetype, std::index_sequence<_Indices...>)
	{
		const uintptr_t position[]{ (archetype.component_offset<_Registry::template index_of<std::decay_t<_Components>>()>())... };
		const size_t lastbucketSize = archetype.size() % _BucketSize;

		auto* bucket = archetype.get_buckets().data();
		const auto* endbucket = bucket + (archetype.size() / _BucketSize);

		for (; bucket < endbucket; ++bucket)
		{
			const uintptr_t componentStart = uintptr_t(&(*bucket)->components());
			for (size_t i = 0; i < _BucketSize; ++i)
				func(conditional_v<std::is_same_v<_Components, entity>, const entity, _Components>(
					(*bucket)->entities()[i],
					reinterpret_cast<std::decay_t<_Components>*>(componentStart + position[_Indices])[i])
					...);
		}

		// won't run anything if it doesn't exist, as that'll run it `lastbucketSize` times, which will be 0
		const uintptr_t componentStart = uintptr_t(&(*bucket)->components());
		for (size_t i = 0; i < lastbucketSize; ++i)
			func(conditional_v<std::is_same_v<_Components, entity>, const entity, _Components>(
				(*bucket)->entities()[i],
				reinterpret_cast<std::decay_t<_Components>*>(componentStart + position[_Indices])[i])
				...);
	}

	template<typename _Registry, size_t _BucketSize>
	template<typename _Func, typename... _Args, typename... _Extra>
	inline void world<_Registry, _BucketSize>::apply_to_qualifying_entities(_Func&& func, ecs::pack<_Extra...>)
	{
		// cache it, preventing .end() rereads on each iteration
		auto archetype = _archetypes.begin(), endArchetype = _archetypes.end();
		for (; archetype != endArchetype; ++archetype)
		{
			if (_Registry::template qualifies<_Args...>(archetype->component_mask(), ecs::pack<_Extra...>()))
				apply_to_archetype_entities<_Args...>(func, *archetype, std::make_index_sequence<sizeof...(_Args)>());
		}
	}

	template<typename _Registry, size_t _BucketSize>
	template<typename... _Extra, typename _Func, typename _Ret, typename... _Args>
	inline void world<_Registry, _BucketSize>::query(_Func&& func, _Ret(_Func::*)(_Args...) const)
	{
		apply_to_qualifying_entities<_Func, std::decay_t<_Args>...>(std::forward<_Func>(func), ecs::pack<_Extra...>());
	}

	template<typename _Registry, size_t _BucketSize>
	template<typename... _Extra, typename _Func, typename _Ret, typename... _Args>
	inline void world<_Registry, _BucketSize>::query(_Func&& func, _Ret(*)(_Args...))
	{
		apply_to_qualifying_entities<_Func, std::decay_t<_Args>...>(std::forward<_Func>(func), ecs::pack<_Extra...>());
	}

	template<typename _Registry, size_t _BucketSize>
	template<typename... _Extra, typename _Func>
	inline void world<_Registry, _BucketSize>::query(_Func&& func)
	{
		if constexpr (std::is_class_v<_Func>)
			query<_Extra...>(std::forward<_Func>(func), &_Func::operator());
		else
			query<_Extra...>(std::forward<_Func>(func), &func);
	}

	template<typename _Registry, size_t _BucketSize>
	template<typename... _Extra>
	inline size_t world<_Registry, _BucketSize>::count()
	{
		size_t count = 0;

		auto archetype = _archetypes.begin(), endArchetype = _archetypes.end();
		for (; archetype != endArchetype; ++archetype)
		{
			if (_Registry::template qualifies<_Extra...>(archetype->component_mask()))
				count += archetype->size();
		}

		return count;
	}
}
