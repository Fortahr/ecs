#pragma once

#include <array>
#include <vector>
#include <tuple>
#include <utility>
#include <cstdlib>

#include "../utils.h"
#include "component_matrix.h"

namespace ecs
{
	namespace details
	{
		template <typename... _Components>
		class archetype_storage;

		struct entity_target
		{
			uint32_t _version;
			uint32_t _index;
			archetype_storage<>* _archetype;

			static constexpr archetype_storage<>* npos = nullptr;

			void set(archetype_storage<>* archetype, uint32_t index)
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

		template <typename... _Components>
		class archetype_storage
		{
		public:
			class bucket
			{
				friend class archetype_storage<_Components...>;
			public:
				typedef component_matrix<_Components...> ComponentData;

			private:
				std::array<entity, config::component_row_count> _to_entity;
				alignas(8) ComponentData _component_data;

			public:
				static constexpr size_t size();

				constexpr ComponentData& components();

				constexpr const ComponentData& components() const;

				constexpr const std::array<entity, config::component_row_count>& entities() const;

				template<typename _T>
				constexpr component_row<_T>& get();

				template<typename _T>
				constexpr const component_row<_T>& get() const;

				template<size_t _Index>
				constexpr component_row<std::tuple_element_t<_Index, std::tuple<_Components...>>>& get();

				template<size_t _Index>
				constexpr const component_row<std::tuple_element_t<_Index, std::tuple<_Components...>>>& get() const;

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
			size_t _component_mask = 0;

			// stored as `offset / _Size` removing unused precision
			uint16_t _component_offsets[std::numeric_limits<size_t>::digits];

			size_t _entity_count = 0;
			std::vector<bucket*> _buckets;

			std::pair<uint32_t, uint32_t>(*removeOperation)(archetype_storage& storage, size_t index) = &remove;

			static std::pair<uint32_t, uint32_t> remove(archetype_storage& storage, size_t index);

			template<typename _T, typename _ComponentMatrix>
			void initialize_component_offset();

		public:
			template<typename... _Cs, typename = std::enable_if_t<(sizeof...(_Components) == 0)>>
			static std::pair<uint32_t, uint32_t> remove_generic(archetype_storage& storage, size_t index, ecs::registry<_Cs...>);

			archetype_storage();

			template<typename... _Cs>
			archetype_storage<_Cs...>& initialize();

			size_t size() const;

			size_t component_mask() const;

			template<size_t _Index>
			size_t component_offset() const;

			size_t component_offset(size_t index) const;

			template<typename _T>
			_T* get_component(entity_target entity) const;

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
