#pragma once

#include <utility>

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
		return config::bucket_size;
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
	{
		memset(_component_offsets, ~0, sizeof(_component_offsets));
	}

	template<typename... _Components>
	inline auto archetype_storage<_Components...>::get_buckets() const -> const std::vector<std::unique_ptr<bucket>>&
	{
		return _buckets;
	}

	template<typename... _Components>
	template<typename _T>
	inline void archetype_storage<_Components...>::move_and_destruct(_T& to, _T&& from)
	{
		to = std::move(from);
		from.~_T();
	}

	template<typename... _Components>
	inline uint32_t archetype_storage<_Components...>::remove(size_t index)
	{
		size_t toIndex = index % config::bucket_size;
		auto& to = this->_buckets[index / config::bucket_size];

		if (this->_entity_count > 0)
		{
			size_t fromIndex = this->_entity_count % config::bucket_size;
			size_t fromBucketIndex = this->_entity_count / config::bucket_size;
			auto& from = this->_buckets[fromBucketIndex];

			entity replaced = to->_to_entity[toIndex] = from->_to_entity[fromIndex];
			from->_to_entity[fromIndex].invalidate();

			size_t reverse = 0;
			((move_and_destruct(to->get<_Components>()[toIndex], std::move(from->get<_Components>()[fromIndex])), reverse) = ... = 0);

			return replaced.get_id();
		}
		else
		{
			size_t reverse = 0;
			((to->get<_Components>()[index].~_Components(), reverse) = ... = 0);

			return entity::npos;
		}
	}

	template<typename... _Components>
	template<typename... _Cs, typename>
	inline uint32_t archetype_storage<_Components...>::remove_generic(archetype_storage& storage, size_t index, ecs::registry<_Cs...>)
	{
		size_t toIndex = index % config::bucket_size;
		bucket* to = (bucket*)storage._buckets[index / config::bucket_size];

		if (storage._entity_count > 0)
		{
			size_t fromIndex = storage._entity_count % config::bucket_size;
			size_t fromBucketIndex = storage._entity_count / config::bucket_size;
			bucket* from = (bucket*)storage._buckets[fromBucketIndex];

			entity replaced = to->toEntity[toIndex] = from->toEntity[fromIndex];
			from->toEntity[fromIndex].remove();

			([&]()
				{
					if (config::registry::template bit_mask_of<_Cs>() & storage.component_mask())
					{
						const size_t offset = storage.component_offset<config::registry::template index_of<_Cs>()>();
						auto& toComponent = reinterpret_cast<_Cs*>(uintptr_t(&to->components()) + offset)[toIndex];
						auto& fromComponent = reinterpret_cast<_Cs*>(uintptr_t(&from->components()) + offset)[fromIndex];

						toComponent = std::move(fromComponent);
						fromComponent.~_Cs();
					}
				}(), ...);

			return replaced.get_id();
		}
		else
		{
			([&]()
				{
					if constexpr (!std::is_trivially_destructible_v<_Cs>)
					{
						if (config::registry::template bit_mask_of<_Cs>() & storage.component_mask())
						{
							const size_t offset = storage.component_offset<config::registry::template index_of<_Cs>()>();
							auto& component = reinterpret_cast<_Cs*>(uintptr_t(&to->components()) + offset)[toIndex];

							component.~_Cs();
						}
					}
				}(), ...);

			storage._buckets.pop_back();

			return entity::npos;
		}
	}

	template<typename... _Components>
	template<typename _T, typename _ComponentMatrix>
	void archetype_storage<_Components...>::initialize_component_offset()
	{
		typedef component_row<_T> Row;
		constexpr size_t offset = offsetof(_ComponentMatrix, Row::_elements) / config::bucket_size;
		static_assert(offset < std::numeric_limits<uint16_t>::max(), "Component offset can no longer fit in uint16_t storage, consider upgrading to uint32_t.");
		_component_offsets[config::registry::template index_of<_T>()] = uint16_t(offset);
	}

	template<typename... _Components>
	template<typename... _Cs>
	inline archetype_storage<_Cs...>& archetype_storage<_Components...>::initialize()
	{
		typedef component_matrix<_Cs...> ComponentMatrix;
		_component_mask = config::registry::template bit_mask_of<_Cs...>();

		(initialize_component_offset<_Cs, ComponentMatrix>(), ...);

		return *reinterpret_cast<archetype_storage<_Cs...>*>(this);
	}

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
		size_t offset = _component_offsets[config::registry::template index_of<_T>()];
		if (offset != std::numeric_limits<uint16_t>::max())
		{
			size_t index = entity._index % config::bucket_size, bucketIndex = entity._index / config::bucket_size;

			auto bucket = _buckets[bucketIndex];
			const uintptr_t componentStart = uintptr_t(&bucket->components());

			return reinterpret_cast<_T*>(componentStart + (offset * config::bucket_size)) + index;
		}

		return nullptr;
	}

	template<typename... _Components>
	inline uint32_t archetype_storage<_Components...>::emplace(entity entity)
	{
		size_t size = _entity_count++;
		size_t index = size % config::bucket_size, bucketIndex = size / config::bucket_size;

		auto& _bucket = bucketIndex < _buckets.size()
			? _buckets[bucketIndex]
			: _buckets.emplace_back(std::make_unique<bucket>());

		_bucket->_to_entity[index] = entity;

		((_bucket->get<_Components>()._elements[index] = {}), ...);

		return uint32_t(bucketIndex * config::bucket_size + index);
	}

	template<typename... _Components>
	template<size_t _CSize, typename>
	inline uint32_t archetype_storage<_Components...>::emplace(entity entity, _Components&&... move)
	{
		size_t size = _entity_count++;
		size_t index = size % config::bucket_size, bucketIndex = size / config::bucket_size;

		auto& _bucket = bucketIndex < _buckets.size()
			? _buckets[bucketIndex]
			: _buckets.emplace_back(std::make_unique<bucket>());

		_bucket->toEntity[index] = entity;

		((_bucket->get<_Components>()[index] = std::move(move)), ...);

		return uint32_t(bucketIndex * config::bucket_size + index);
	}

	template<typename... _Components>
	inline uint32_t archetype_storage<_Components...>::erase(size_t index)
	{
		assert(index < _entity_count);

		--_entity_count;

		return (this->*removeOperation)(index);
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
