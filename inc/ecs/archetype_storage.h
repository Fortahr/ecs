#pragma once

#include <array>
#include <vector>
#include <tuple>
#include <utility>
#include <cstdlib>

#include "utils.h"

namespace ecs
{
	namespace details
	{
		template <size_t _Size, typename... _Components>
		class archetype_storage;

		struct entity_target
		{
			uint32_t _version;
			uint32_t _index;
			archetype_storage<64>* _archetype;

			static constexpr archetype_storage<64>* npos = nullptr;

			void set(archetype_storage<64>* archetype, uint32_t index)
			{
				this->_archetype = archetype;
				this->_index = index;
			}

			void move(uint32_t index)
			{
				this->_index = index;
			}

			void invalidate()
			{
				_version++;
				_index = 0;
				_archetype = nullptr;
			}
		};

		template <size_t _Size, typename... _Components>
		class archetype_storage
		{
		public:
			class bucket
			{
				friend class archetype_storage<_Size, _Components...>;
			public:
				typedef std::tuple<std::array<_Components, _Size>...> ComponentData;

			private:
				std::array<entity, _Size> toEntity;
				alignas(8) ComponentData componentData;

			public:
				static constexpr size_t size();

				constexpr ComponentData& components();

				constexpr const ComponentData& components() const;

				constexpr const std::array<entity, _Size>& entities() const;

				template<typename _T>
				constexpr std::array<_T, _Size>& get();

				template<typename _T>
				constexpr const std::array<_T, _Size>& get() const;

				template<size_t _Index>
				constexpr std::array<std::tuple_element_t<_Index, std::tuple<_Components...>>, _Size>& get();

				template<size_t _Index>
				constexpr const std::array<std::tuple_element_t<_Index, std::tuple<_Components...>>, _Size>& get() const;

				/*void* operator new(size_t size)
				{
#if _WIN32
					return _aligned_malloc(size, _Size);
#else
					return aligned_alloc(_Size, size);
#endif
				}

				void operator delete(void* ptr)
				{
#if _WIN32
					_aligned_free(ptr);
#else
					free(ptr);
#endif
				}*/
			};

		private:
			size_t componentMask = 0;

			// stored as `offset / _Size` removing unused precision
			uint16_t componentOffsets[std::numeric_limits<size_t>::digits] = { uint16_t(~0) };

			size_t entityCount = 0;
			std::vector<bucket*> buckets;

			std::pair<uint32_t, uint32_t>(*removeOperation)(archetype_storage& storage, size_t index) = &remove;

			static std::pair<uint32_t, uint32_t> remove(archetype_storage& storage, size_t index);

		public:
			template<typename... _Cs, typename = std::enable_if_t<(sizeof...(_Components) == 0)>>
			static std::pair<uint32_t, uint32_t> remove_generic(archetype_storage& storage, size_t index, ecs::registry<_Cs...>);

		public:
			archetype_storage() = default;

			template<typename _Registry, typename... _Cs>
			archetype_storage<_Size, _Cs...>& initialize();

			size_t size() const;

			size_t component_mask() const;

			template<size_t _Index>
			uint16_t component_offset() const;

			uint16_t component_offset(size_t index) const;

			uint32_t emplace(entity entity);

			template<typename = std::enable_if_t<(sizeof...(_Components) > 0)>>
			uint32_t emplace(entity entity, _Components&&... move);

			// erases entity at given index, returns the entity that took its place
			std::pair<uint32_t, uint32_t> erase(size_t index);

			//void reserve(size_t size);
			const std::vector<bucket*>& get_buckets() const;
		};
	}
}

#include "archetype_storage.inl"
