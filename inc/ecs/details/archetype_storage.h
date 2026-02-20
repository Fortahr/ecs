#pragma once

#include <vector>
#include <tuple>
#include <utility>
#include <cstdlib>

#include "../config_registry.h"
#include "../utils.h"
#include "component_matrix.h"

#ifndef ECS_ONLY_USE_RUNTIME_REMOVE_FUNC
#define ECS_ONLY_USE_RUNTIME_REMOVE_FUNC true
#endif

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

			void move(uint32_t index, archetype_storage<>& storage)
			{
				this->_index = index;
				this->_archetype = &storage;
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
			class alignas(config::bucket_size) bucket
			{
				friend class archetype_storage<_Components...>;
			public:
				typedef component_matrix<_Components...> ComponentData;

			private:
				entity _to_entity[config::bucket_size];
				alignas(8) ComponentData _component_data;

			public:
				static constexpr size_t max_size = config::bucket_size;

				// returns max size, not currently stored entities
				static constexpr size_t size();

				constexpr ComponentData& components();

				constexpr const ComponentData& components() const;

				constexpr const entity& get_entity(size_t index) const;

				template<typename _T>
				constexpr component_row<_T>& get();

				template<typename _T>
				constexpr const component_row<_T>& get() const;

				template<typename _T>
				constexpr _T& get(size_t index);

				template<typename _T>
				constexpr component_row<_T>& get_unsafe(size_t offset);

				template<typename _T>
				constexpr _T& get_unsafe(size_t offset, size_t index);

				template<size_t _Index>
				constexpr component_row<std::tuple_element_t<_Index, std::tuple<_Components...>>>& get();

				template<size_t _Index>
				constexpr const component_row<std::tuple_element_t<_Index, std::tuple<_Components...>>>& get() const;

				static void* operator new(size_t count);
				static void operator delete(void* ptr);

				static void* operator new(size_t count, size_t custom_size) { return operator new(custom_size); }
				static void operator delete(void* ptr, size_t custom_size) { operator delete(ptr); }
			};

		private:
			size_t _component_mask = 0;

			// stored as `offset / _Size` removing unused precision
			uint16_t _component_offsets[ecs::config::registry::count];

			size_t _entity_count = 0;
			std::vector<std::unique_ptr<bucket>> _buckets;

			size_t _bucket_size;
			// uint32_t _bucket_align; // we are already aligned by config::bucket_size

			uint32_t remove(size_t index);

#if !ECS_ONLY_USE_RUNTIME_REMOVE_FUNC
			decltype(&archetype_storage::remove) _removeOperation; // runtime version is faster in our test cases
#endif

			template<typename _T>
			static void destruct(_T& object);

			template<typename _T>			
			static void move_and_destruct(_T& to, _T&& from);

			template<typename... _Cs, typename _MoveFunc, typename _RemoveFunc>
			uint32_t remove_internal(size_t index, _MoveFunc move_func, _RemoveFunc remove_func);

			template<typename _T, typename _ComponentMatrix>
			void initialize_component_offset();

			template<typename... _Cs>
			uint32_t emplace_internal(entity entity, _Cs&&... move);

		public:
			archetype_storage();

			template<typename... _Cs>
			void initialize();

#pragma region runtime methods
			template<typename... _Cs>
			void runtime_initialize(size_t mask, ecs::pack<_Cs...>);

			template<typename... _Cs>
			std::tuple<uint32_t, bucket*, uint32_t> runtime_move(size_t index, archetype_storage<>& new_storage, ecs::pack<_Cs...>);
			
			template<typename... _Cs>
			uint32_t runtime_remove_internal(size_t index, ecs::pack<_Cs...>);

			uint32_t runtime_remove(size_t index);

			template<typename... _Cs>
			uint32_t runtime_emplace(entity entity, ecs::pack<_Cs...>);
#pragma endregion

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
			uint32_t erase(size_t index);

			//void reserve(size_t size);
			const std::vector<std::unique_ptr<bucket>>& get_buckets() const;

			constexpr explicit operator archetype_storage<>& ()
			{
				return reinterpret_cast<archetype_storage<>&>(*this);
			}
		};
	}
}

#include "archetype_storage.inl"
