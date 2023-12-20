#pragma once

#include <type_traits>
#include <stdexcept>

namespace ecs::details
{
	template <typename _T, size_t _BucketCapacity, typename _Alloc = std::allocator<_T>>
	class bucket_vector
	{
	private:
		struct bucket_t
		{
			union // disable RAII
			{
				_T _data[_BucketCapacity];
			};

			bucket_t* _next = nullptr;

			bucket_t() { };
			~bucket_t() { };
		};

		size_t _size = 0;
		size_t _capacity = 0;

		bucket_t* _first = nullptr;
		bucket_t* _last = nullptr;

		_Alloc _alloc;

		bucket_t* get_bucket(size_t index);

		_T& get_element(size_t index);

		void destruct(_T& element);

	public:
		class const_iterator
		{
			friend class bucket_vector;

		private:
			bucket_t* _bucket;
			size_t _index;

			const_iterator(bucket_t* bucket, size_t index)
				: _bucket(bucket)
				, _index(index)
			{
			}

		public:
			const_iterator& operator++()
			{
				if (++_index == _BucketCapacity)
				{
					_bucket = _bucket->_next;
					_index = 0;
				}

				return *this;
			}

			bool operator==(const const_iterator& other) const
			{
				return _bucket == other._bucket && _index == other._index;
			}

			bool operator!=(const const_iterator& other) const
			{
				return _bucket != other._bucket || _index != other._index;
			}

			const _T& operator*() const
			{
				return _bucket->_data[_index];
			}

			const _T* operator->() const
			{
				return _bucket->_data + _index;
			}
		};

		class iterator : private const_iterator
		{
			friend class bucket_vector;

		private:
			bucket_t* _bucket;
			size_t _index;

			iterator(bucket_t* bucket, size_t index)
				: const_iterator(bucket, index)
			{
			}

		public:
			iterator& operator++()
			{
				const_iterator::operator++();
				return *this;
			}

			bool operator==(const iterator& other) const
			{
				return const_iterator::operator==(other);
			}

			bool operator!=(const iterator& other) const
			{
				return const_iterator::operator!=(other);
			}

			_T& operator*() const
			{
				return const_cast<_T&>(const_iterator::operator*());
			}

			_T* operator->() const
			{
				return const_cast<_T*>(const_iterator::operator->());
			}
		};

		bucket_vector();
		~bucket_vector();

		bool empty();
		size_t size();
		size_t capacity();

		_T& operator[] (size_t index);
		const _T& operator[] (size_t index) const;

		template <typename... _Args>
		_T& emplace_back(_Args&&... arguments);

		_T& push_back(const _T& copy);

		iterator begin();
		iterator end();

		const_iterator begin() const;
		const_iterator end() const;

		void clear();
	};

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline bucket_vector<_T, _BucketCapacity, _Alloc>::bucket_vector()
	{

	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline bucket_vector<_T, _BucketCapacity, _Alloc>::~bucket_vector()
	{
		clear();
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline bool bucket_vector<_T, _BucketCapacity, _Alloc>::empty()
	{
		return _size == 0;
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline size_t bucket_vector<_T, _BucketCapacity, _Alloc>::size()
	{
		return _size;
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline size_t bucket_vector<_T, _BucketCapacity, _Alloc>::capacity()
	{
		return _capacity;
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline auto bucket_vector<_T, _BucketCapacity, _Alloc>::get_bucket(size_t index) -> bucket_t*
	{
		bucket_t* bucket = _first;

		for (size_t i = 0; i < index; ++i)
			bucket = bucket->_next;

		return bucket;
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline _T& bucket_vector<_T, _BucketCapacity, _Alloc>::get_element(size_t index)
	{
		bucket_t* bucket = _first;

		for (; index > _BucketCapacity; index -= _BucketCapacity)
			bucket = bucket->_next;

		return bucket->_data[index];
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline void bucket_vector<_T, _BucketCapacity, _Alloc>::destruct(_T& element)
	{
		std::allocator_traits<_Alloc>::destroy(_alloc, &element);
		//_data[i].~_T();
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline _T& bucket_vector<_T, _BucketCapacity, _Alloc>::operator[] (size_t index)
	{
		return get_element(index);
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline const _T& bucket_vector<_T, _BucketCapacity, _Alloc>::operator[] (size_t index) const
	{
		return get_element(index);
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	template <typename... _Args>
	inline _T& bucket_vector<_T, _BucketCapacity, _Alloc>::emplace_back(_Args&&... arguments)
	{
		size_t elementIndex = _size++ % _BucketCapacity;
		bucket_t* bucket = _last;

		if (bucket == nullptr)
			bucket = _first = _last = new bucket_t();
		else if (elementIndex == 0)
			bucket = _last = _last->_next = new bucket_t();
				
		_T& element = bucket->_data[elementIndex];
		std::allocator_traits<_Alloc>::construct(_alloc, &element, std::forward<_Args>(arguments)...);
		//new (&element) _T(std::forward<_Args>(arguments)...);

		return element;
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline _T& bucket_vector<_T, _BucketCapacity, _Alloc>::push_back(const _T& copy)
	{
		size_t elementIndex = _size++ % _BucketCapacity;
		bucket_t* bucket = _last;

		if (bucket == nullptr)
			bucket = _first = _last = new bucket_t();
		else if (elementIndex == 0)
			bucket = _last = _last->_next = new bucket_t();

		_T& element = bucket->_data[elementIndex];
		element = copy;

		return element;
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline void bucket_vector<_T, _BucketCapacity, _Alloc>::clear()
	{
		bucket_t* bucket = _first, * endBucket = _last;
		size_t size = _size;

		for (; size >= _BucketCapacity; size -= _BucketCapacity, bucket = bucket->_next)
		{
			for (size_t i = 0; i < _BucketCapacity; ++i)
				destruct(bucket->_data[i]);

			delete bucket;
		}
		
		if (size > 0)
		{
			destruct(bucket->_data[0]);

			for (size_t i = 1; i <= size; ++i)
				destruct(bucket->_data[i]);

			delete bucket;
		}

		_size = 0;
		_first = _last = nullptr;
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline auto bucket_vector<_T, _BucketCapacity, _Alloc>::begin() -> iterator
	{
		return iterator(_first, 0);
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline auto bucket_vector<_T, _BucketCapacity, _Alloc>::end() -> iterator
	{
		return iterator(_last, _size % _BucketCapacity);
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline auto bucket_vector<_T, _BucketCapacity, _Alloc>::begin() const -> const_iterator
	{
		return const_iterator(_first, 0);
	}

	template <typename _T, size_t _BucketCapacity, typename _Alloc>
	inline auto bucket_vector<_T, _BucketCapacity, _Alloc>::end() const -> const_iterator
	{
		return const_iterator(_last, (_size % _BucketCapacity) + 1);
	}
}
