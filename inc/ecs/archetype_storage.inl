#pragma once

#include <utility>

#include "archetype_storage.h"

namespace ecs::details
{
	template<size_t _Size, typename... _Components>
	constexpr auto archetype_storage<_Size, _Components...>::bucket::components() -> ComponentData&
	{
		return componentData;
	}

	template<size_t _Size, typename... _Components>
	constexpr auto archetype_storage<_Size, _Components...>::bucket::components() const -> const ComponentData&
	{
		return componentData;
	}

	template<size_t _Size, typename... _Components>
	constexpr const std::array<entity, _Size>& archetype_storage<_Size, _Components...>::bucket::entities() const
	{
		return toEntity;
	}

	template<size_t _Size, typename... _Components>
	constexpr size_t archetype_storage<_Size, _Components...>::bucket::size()
	{
		return _Size;
	}

	template<size_t _Size, typename... _Components>
	template<typename T>
	constexpr std::array<T, _Size>& archetype_storage<_Size, _Components...>::bucket::get()
	{
		return std::get<std::array<T, _Size>>(componentData);
	}

	template<size_t _Size, typename... _Components>
	template<typename T>
	constexpr const std::array<T, _Size>& archetype_storage<_Size, _Components...>::bucket::get() const
	{
		return std::get<const std::array<T, _Size>>(componentData);
	}

	template<size_t _Size, typename... _Components>
	template<size_t _Index>
	constexpr std::array<std::tuple_element_t<_Index, std::tuple<_Components...>>, _Size>& archetype_storage<_Size, _Components...>::bucket::get()
	{
		using Type = std::tuple_element_t<_Index, std::tuple<_Components...>>;
		return std::get<std::array<Type, _Size>>(componentData);
	}

	template<size_t _Size, typename... _Components>
	template<size_t _Index>
	constexpr const std::array<std::tuple_element_t<_Index, std::tuple<_Components...>>, _Size>& archetype_storage<_Size, _Components...>::bucket::get() const
	{
		using Type = std::tuple_element_t<_Index, std::tuple<_Components...>>;
		return std::get<std::array<Type, _Size>>(componentData);
	}

	template<size_t _Size, typename... _Components>
	inline auto archetype_storage<_Size, _Components...>::get_buckets() const -> const std::vector<bucket*>&
	{
		return buckets;
	}

	template<size_t _Size, typename... _Components>
	inline std::pair<uint32_t, uint32_t> archetype_storage<_Size, _Components...>::remove(archetype_storage& storage, size_t index)
	{
		size_t toIndex = index % _Size;
		bucket* to = (bucket*)storage.buckets[index / _Size];

		if (storage.entityCount > 0)
		{
			size_t fromIndex = storage.entityCount % _Size;
			size_t fromBucketIndex = storage.entityCount / _Size;
			bucket* from = (bucket*)storage.buckets[fromBucketIndex];

			entity replaced = to->toEntity[toIndex] = from->toEntity[fromIndex];
			from->toEntity[fromIndex].invalidate();

			((to->get<_Components>()[toIndex] = std::move(from->get<_Components>()[fromIndex])), ...);

			return { replaced.get_id(), uint32_t(fromIndex + fromBucketIndex * _Size)};
		}

		((to->get<_Components>()[index].~_Components()), ...);

		return { entity::npos, ~0 };
	}

	template<size_t _Size, typename... _Components>
	template<typename... _Cs, typename>
	inline std::pair<uint32_t, uint32_t> archetype_storage<_Size, _Components...>::remove_generic(archetype_storage& storage, size_t index, ecs::registry<_Cs...>)
	{
		typedef ecs::registry<_Cs...> _Registry;

		size_t toIndex = index % _Size;
		bucket* to = (bucket*)storage.buckets[index / _Size];

		if (storage.entityCount > 0)
		{
			size_t fromIndex = storage.entityCount % _Size;
			size_t fromBucketIndex = storage.entityCount / _Size;
			bucket* from = (bucket*)storage.buckets[fromBucketIndex];

			entity replaced = to->toEntity[toIndex] = from->toEntity[fromIndex];
			from->toEntity[fromIndex].remove();

			([&]()
				{					
					if (_Registry::template bit_mask_of<_Cs>() & storage.component_mask())
					{
						const size_t offset = storage.component_offset<_Registry::template index_of<_Cs>()>();
						auto& toComponent = reinterpret_cast<_Cs*>(uintptr_t(&to->components()) + offset)[toIndex];
						auto& fromComponent = reinterpret_cast<_Cs*>(uintptr_t(&from->components()) + offset)[fromIndex];

						toComponent = std::move(fromComponent);
					}
				}(), ...);

			return { replaced.get_id(), uint32_t(fromIndex + fromBucketIndex * _Size)};
		}

		([&]()
			{
				if (_Registry::template bit_mask_of<_Cs>() & storage.component_mask())
				{
					const size_t offset = storage.component_offset<_Registry::template index_of<_Cs>()>();
					auto& component = reinterpret_cast<_Cs*>(uintptr_t(&to->components()) + offset)[toIndex];

					component.~_Cs();
				}
			}(), ...);

		return { entity::npos, ~0 };
	}

	template<size_t _Size, typename... _Components>
	template<typename _Registry, typename... _Cs>
	inline archetype_storage<_Size, _Cs...>& archetype_storage<_Size, _Components...>::initialize()
	{
		typedef std::tuple<std::array<_Cs, _Size>...> ComponentMatrix;
		componentMask = _Registry::template bit_mask_of<_Cs...>();

		((
			componentOffsets[_Registry::template index_of<_Cs>()] = uint16_t(
				ptrdiff_t((std::array<_Cs, _Size>*)(ComponentMatrix*)0x100) - ptrdiff_t((ComponentMatrix*)0x100)) / _Size
			), ...);

		return *reinterpret_cast<archetype_storage<_Size, _Cs...>*>(this);
	}

	template<size_t _Size, typename... _Components>
	size_t archetype_storage<_Size, _Components...>::size() const
	{
		return entityCount;
	}

	template<size_t _Size, typename... _Components>
	inline size_t archetype_storage<_Size, _Components...>::component_mask() const
	{
		return componentMask;
	}

	template<size_t _Size, typename... _Components>
	template<size_t _Index>
	inline uint16_t archetype_storage<_Size, _Components...>::component_offset() const
	{
		return componentOffsets[_Index] * _Size;
	}

	template<size_t _Size, typename... _Components>
	inline uint16_t archetype_storage<_Size, _Components...>::component_offset(size_t index) const
	{
		return componentOffsets[index] * _Size;
	}

	template<size_t _Size, typename... _Components>
	inline uint32_t archetype_storage<_Size, _Components...>::emplace(entity entity)
	{
		size_t size = entityCount++;
		size_t index = size % _Size, bucketIndex = size / _Size;

		bucket* _bucket = bucketIndex < buckets.size()
			? (bucket*)buckets[bucketIndex]
			: (bucket*)buckets.emplace_back(new bucket());

		_bucket->toEntity[index] = entity;

		((_bucket->get<_Components>()[index] = {}), ...);

		return uint32_t(bucketIndex * _Size + index);
	}

	template<size_t _Size, typename... _Components>
	template<typename>
	inline uint32_t archetype_storage<_Size, _Components...>::emplace(entity entity, _Components&&... move)
	{
		size_t size = entityCount++;
		size_t index = size % _Size, bucketIndex = size / _Size;

		bucket* _bucket = bucketIndex < buckets.size()
			? (bucket*)buckets[bucketIndex]
			: (bucket*)buckets.emplace_back(new bucket());

		_bucket->toEntity[index] = entity;

		((_bucket->get<_Components>()[index] = std::move(move)), ...);

		return uint32_t(bucketIndex * _Size + index);
	}

	template<size_t _Size, typename... _Components>
	inline std::pair<uint32_t, uint32_t> archetype_storage<_Size, _Components...>::erase(size_t index)
	{
		if (index < entityCount)
		{
			--entityCount;
			
			return removeOperation(*this, index);
		}
	}

	/*template<size_t _Size, typename... _Components>
	inline void archetype_storage<_Size, _Components...>::reserve(size_t size)
	{
		size_t index = size % _Size, bucketSize = size / _Size;

		buckets.reserve(bucketSize + 1);

		auto it = buckets.data(), end = it + (buckets.capacity() - buckets.size());
		for (; it != end; ++it)
			*it = new bucket();
	}*/
}
