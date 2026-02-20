#pragma once

#include <cassert>
#include <utility>

#include "../config.h"

#include "archetype_storage.h"

namespace ecs::details
{
	template<typename... _Components>
	constexpr auto archetype_storage<_Components...>::bucket::components() -> ComponentData&
	{
		return _component_data;
	}

	template<typename... _Components>
	constexpr auto archetype_storage<_Components...>::bucket::components() const -> const ComponentData&
	{
		return _component_data;
	}

	template<typename... _Components>
	constexpr const entity& archetype_storage<_Components...>::bucket::get_entity(size_t index) const
	{
		assert(index < config::bucket_size);
		return _to_entity[index];
	}

	template<typename... _Components>
	constexpr size_t archetype_storage<_Components...>::bucket::size()
	{
		return max_size;
	}

	template<typename... _Components>
	template<typename _T>
	constexpr component_row<_T>& archetype_storage<_Components...>::bucket::get()
	{
		return static_cast<component_row<_T>&>(_component_data);
	}

	template<typename... _Components>
	template<typename _T>
	constexpr const component_row<_T>& archetype_storage<_Components...>::bucket::get() const
	{
		return static_cast<const component_row<_T>&>(_component_data);
	}

	template<typename... _Components>
	template<typename _T>
	constexpr _T& archetype_storage<_Components...>::bucket::get(size_t index)
	{
		return static_cast<component_row<_T>&>(_component_data)[index];
	}

	template<typename... _Components>
	template<typename _T>
	constexpr component_row<_T>& archetype_storage<_Components...>::bucket::get_unsafe(size_t offset)
	{
		return *reinterpret_cast<component_row<_T>*>(uintptr_t(&_component_data) + offset);
	}

	template<typename... _Components>
	template<typename _T>
	constexpr _T& archetype_storage<_Components...>::bucket::get_unsafe(size_t offset, size_t index)
	{
		return get_unsafe<_T>(offset)[index];
	}

	template<typename... _Components>
	void* archetype_storage<_Components...>::bucket::operator new(size_t count)
	{
#if _WIN32
		return _aligned_malloc(count, config::bucket_size);
#else
		return aligned_alloc(config::bucket_size, count);
#endif
	}

	template<typename... _Components>
	void archetype_storage<_Components...>::bucket::operator delete(void* ptr)
	{
#if _WIN32
		_aligned_free(ptr);
#else
		free(ptr);
#endif
	}

	template<typename... _Components>
	template<size_t _Index>
	constexpr component_row<std::tuple_element_t<_Index, std::tuple<_Components...>>>& archetype_storage<_Components...>::bucket::get()
	{
		using Type = std::tuple_element_t<_Index, std::tuple<_Components...>>;
		return static_cast<const component_row<Type>&>(_component_data);
	}

	template<typename... _Components>
	template<size_t _Index>
	constexpr const component_row<std::tuple_element_t<_Index, std::tuple<_Components...>>>& archetype_storage<_Components...>::bucket::get() const
	{
		using Type = std::tuple_element_t<_Index, std::tuple<_Components...>>;
		return static_cast<const component_row<Type>&>(_component_data);
	}

	template<typename... _Components>
	archetype_storage<_Components...>::archetype_storage()
		: _bucket_size(sizeof(bucket))
	{
		std::fill(_component_offsets, _component_offsets + std::size(_component_offsets), ~0);

#if !ECS_ONLY_USE_RUNTIME_REMOVE_FUNC
		if constexpr (sizeof...(_Components) > 0)
			_removeOperation = &archetype_storage::remove;
		else
			_removeOperation = &archetype_storage::runtime_remove;
#endif
	}

	template<typename... _Components>
	inline auto archetype_storage<_Components...>::get_buckets() const -> const std::vector<std::unique_ptr<bucket>>&
	{
		return _buckets;
	}

	template<typename... _Components>
	template<typename _T>
	inline void archetype_storage<_Components...>::destruct(_T& object)
	{
		object.~_T();
	}

	template<typename... _Components>
	template<typename _T>
	inline void archetype_storage<_Components...>::move_and_destruct(_T& to, _T&& from)
	{
		to = std::move(from);
		from.~_T();
	}

	template<typename... _Components>
	template<typename... _Cs, typename _MoveFunc, typename _RemoveFunc>
	inline uint32_t archetype_storage<_Components...>::remove_internal(size_t index, _MoveFunc move_func, _RemoveFunc remove_func)
	{
		assert(index < _entity_count);

		size_t entityCount = --_entity_count;
		if (entityCount > 0)
		{
			size_t toIndex = index % config::bucket_size;
			auto& to = _buckets[index / config::bucket_size];

			size_t fromIndex = entityCount % config::bucket_size;
			size_t fromBucketIndex = entityCount / config::bucket_size;
			auto& from = _buckets[fromBucketIndex];

			entity replaced = to->_to_entity[toIndex] = from->_to_entity[fromIndex];
			from->_to_entity[fromIndex].invalidate();

			size_t reverse = 0;
			if constexpr (sizeof...(_Components) > 0)
				((move_func(to->get<_Cs>(toIndex), std::move(from->get<_Cs>(fromIndex)), _component_mask), reverse) = ... = 0);
			else
			{
				(([&]()
				{
					const size_t offset = component_offset<config::registry::template index_of<_Cs>>();
					move_func(to->get_unsafe<_Cs>(offset, toIndex), std::move(from->get_unsafe<_Cs>(offset, fromIndex)), _component_mask);
				}, reverse) = ... = 0);
			}

			if (fromIndex == 0)
				_buckets.pop_back();

			return replaced.get_id();
		}
		else
		{
			assert(_buckets.size() == 1);

			size_t reverse = 0;
			if constexpr (sizeof...(_Components) > 0)
				((remove_func(_buckets[0]->get<_Cs>(0), _component_mask), reverse) = ... = 0);
			else
			{
				(([&]()
				{
					const size_t offset = component_offset<config::registry::template index_of<_Cs>>();
					remove_func(_buckets[0]->get_unsafe<_Cs>(offset, 0), _component_mask);
				}, reverse) = ... = 0);
			}

			_buckets.clear();

			return entity::npos;
		}
	}

	template<typename... _Components>
	inline uint32_t archetype_storage<_Components...>::remove(size_t index)
	{
		return remove_internal<_Components...>(index,
			[](auto& to, auto&& from, auto mask) { move_and_destruct(to, std::move(from)); },
			[](auto& remove, auto mask) { destruct(remove); });
	}

	template<typename... _Components>
	template<typename _T, typename _ComponentMatrix>
	void archetype_storage<_Components...>::initialize_component_offset()
	{
		typedef component_row<_T> Row;
		constexpr size_t offset = offsetof(_ComponentMatrix, Row::_elements) / config::bucket_size;
		static_assert(offset < std::numeric_limits<uint16_t>::max(), "Component offset can no longer fit in uint16_t storage, consider upgrading to uint32_t.");
		_component_offsets[config::registry::template index_of<_T>] = uint16_t(offset);
	}

	template<typename... _Components>
	template<typename... _Cs>
	inline void archetype_storage<_Components...>::initialize()
	{
		typedef component_matrix<_Cs...> ComponentMatrix;
		_component_mask = config::registry::template bit_mask_of<_Cs...>;
		_bucket_size = sizeof(archetype_storage<_Cs...>::bucket);

		(initialize_component_offset<_Cs, ComponentMatrix>(), ...);
	}

#pragma region runtime functions

	template<typename... _Components>
	template<typename... _Cs>
	inline void archetype_storage<_Components...>::runtime_initialize(size_t mask, ecs::pack<_Cs...>)
	{
		using offset_t = std::remove_reference_t<decltype(*_component_offsets)>;

		size_t bucketSize = 0;

		([&]()
			{
				constexpr size_t i = config::registry::template index_of<_Cs>;
				constexpr size_t componentMask = 1ull << i;

				if (componentMask & mask)
				{
					assert(bucketSize < std::numeric_limits<offset_t>::max());

					// size storing as: sizeof(T[config::bucket_size]) / config::bucket_size = sizeof(T)
					_component_offsets[i] = offset_t(bucketSize);

					bucketSize += offset_t(ecs::config::registry::_component_size[i]);

				}
			}(), ...);

		_component_mask = mask;
		_bucket_size = bucketSize * config::bucket_size + sizeof(archetype_storage<>::bucket);
	}

	template<typename... _Components>
	template<typename... _Cs>
	inline auto archetype_storage<_Components...>::runtime_move(size_t index, archetype_storage<>& new_storage, ecs::pack<_Cs...>)
		-> std::tuple<uint32_t, bucket*, uint32_t>
	{
		size_t newIndex = new_storage._entity_count++;
		size_t newBucketIndex = newIndex / config::bucket_size;
		size_t newElementIndex = newIndex % config::bucket_size;

		auto& newBucket = newBucketIndex < new_storage._buckets.size()
			? new_storage._buckets[newBucketIndex]
			: new_storage._buckets.emplace_back(new (new_storage._bucket_size) bucket());

		size_t toIndex = index % config::bucket_size;
		auto& to = _buckets[index / config::bucket_size];

		newBucket->_to_entity[newElementIndex] = to->_to_entity[toIndex];

		return { uint32_t(newIndex), newBucket.get(), remove_internal<_Cs...>(index,
			[&](auto& to, auto&& from, auto mask)
			{
				typedef std::remove_reference_t<decltype(to)> _Cs;
				
				if (config::registry::template bit_mask_of<_Cs> & mask)
				{
					const size_t newOffset = new_storage.component_offset<config::registry::template index_of<_Cs>>();

					newBucket->get_unsafe<_Cs>(newOffset, newElementIndex) = std::move(to);
					move_and_destruct(to, std::move(from));
				}
			},
			[&](auto& remove, auto mask)
			{
				typedef std::remove_reference_t<decltype(remove)> _Cs;

				if (config::registry::template bit_mask_of<_Cs> & mask)
				{
					const size_t newOffset = new_storage.component_offset<config::registry::template index_of<_Cs>>();

					move_and_destruct(newBucket->get_unsafe<_Cs>(newOffset, newElementIndex), std::move(remove));
				}
			}) };
	}

	template<typename... _Components>
	template<typename... _Cs>
	inline uint32_t archetype_storage<_Components...>::runtime_remove_internal(size_t index, ecs::pack<_Cs...>)
	{
		return remove_internal<_Cs...>(index,
			[](auto& to, auto&& from, auto mask)
			{
				typedef std::remove_reference_t<decltype(to)> _Cs;

				if (config::registry::template bit_mask_of<_Cs> & mask)
					move_and_destruct(to, std::move(from));
			},
			[](auto& remove, auto mask)
			{
				typedef std::remove_reference_t<decltype(remove)> _Cs;

				if constexpr (!std::is_trivially_destructible_v<_Cs>)
				{
					if (config::registry::template bit_mask_of<_Cs> & mask)
						remove.~_Cs();
				}
			});
	}

	template<typename... _Components>
	inline uint32_t archetype_storage<_Components...>::runtime_remove(size_t index)
	{
		return runtime_remove_internal(index, ecs::config::registry::components());
	}

	template<typename... _Components>
	template<typename... _Cs>
	inline uint32_t archetype_storage<_Components...>::runtime_emplace(entity entity, ecs::pack<_Cs...>)
	{
		size_t size = _entity_count++;
		size_t index = size % config::bucket_size, bucketIndex = size / config::bucket_size;

		auto& _bucket = bucketIndex < _buckets.size()
			? _buckets[bucketIndex]
			: _buckets.emplace_back((bucket*)new uint8_t[this->_bucket_size]);

		_bucket->_to_entity[index] = entity;

		([&]()
			{
				//if constexpr (!std::is_trivially_constructible_v<_Cs>)
				{
					constexpr size_t i = config::registry::template index_of<_Cs>();
					constexpr size_t mask = 1ull << i;

					if (mask & _component_mask)
						new (&_bucket->get_unsafe<_Cs>(_component_offsets[i], index)) _Cs();
				}
			}(), ...);

		return uint32_t(bucketIndex * config::bucket_size + index);
	}

#pragma endregion

	template<typename... _Components>
	size_t archetype_storage<_Components...>::size() const
	{
		return _entity_count;
	}

	template<typename... _Components>
	inline size_t archetype_storage<_Components...>::component_mask() const
	{
		return _component_mask;
	}

	template<typename... _Components>
	template<size_t _Index>
	inline size_t archetype_storage<_Components...>::component_offset() const
	{
		return _component_offsets[_Index] * config::bucket_size;
	}

	template<typename... _Components>
	inline size_t archetype_storage<_Components...>::component_offset(size_t index) const
	{
		return _component_offsets[index] * config::bucket_size;
	}

	template<typename... _Components>
	template<typename _T>
	inline _T* archetype_storage<_Components...>::get_component(entity_target entity) const
	{
		size_t offset = _component_offsets[config::registry::template index_of<_T>];
		if (offset != std::numeric_limits<uint16_t>::max())
		{
			size_t index = entity._index % config::bucket_size, bucketIndex = entity._index / config::bucket_size;

			auto& bucket = _buckets[bucketIndex];
			const uintptr_t componentStart = uintptr_t(&bucket->components());

			return reinterpret_cast<_T*>(componentStart + (offset * config::bucket_size)) + index;
		}

		return nullptr;
	}

	template<typename... _Components>
	template<typename... _Cs>
	inline uint32_t archetype_storage<_Components...>::emplace_internal(entity entity, _Cs&&... move)
	{
		size_t size = _entity_count++;
		size_t index = size % config::bucket_size, bucketIndex = size / config::bucket_size;

		auto& _bucket = bucketIndex < _buckets.size()
			? _buckets[bucketIndex]
			: _buckets.emplace_back(std::make_unique<bucket>());

		_bucket->_to_entity[index] = entity;

		if constexpr (sizeof...(_Cs) > 0)
			((_bucket->get<_Cs>(index) = std::move(move)), ...);
		else
			((_bucket->get<_Cs>(index)._Cs()), ...);

		return uint32_t(bucketIndex * config::bucket_size + index);
	}

	template<typename... _Components>
	inline uint32_t archetype_storage<_Components...>::emplace(entity entity)
	{
		return emplace_internal(entity);
	}

	template<typename... _Components>
	template<typename>
	inline uint32_t archetype_storage<_Components...>::emplace(entity entity, _Components&&... move)
	{
		return emplace_internal(entity, std::forward<_Components>(move)...);
	}

	template<typename... _Components>
	inline uint32_t archetype_storage<_Components...>::erase(size_t index)
	{
#if ECS_ONLY_USE_RUNTIME_REMOVE_FUNC
		return runtime_remove(index);
#else
		return (this->*_removeOperation)(index);
#endif
	}

	/*template<typename... _Components>
	inline void archetype_storage<_Components...>::reserve(size_t size)
	{
		size_t index = size % config::bucket_size, bucketSize = size / config::bucket_size;

		buckets.reserve(bucketSize + 1);

		auto it = buckets.data(), end = it + (buckets.capacity() - buckets.size());
		for (; it != end; ++it)
			*it = new bucket();
	}*/
}
