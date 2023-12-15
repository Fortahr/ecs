#pragma once

#include <type_traits>
#include <stdexcept>

namespace ecs::details
{
	template <typename _T, size_t _Capacity, typename _Alloc = std::allocator<_T>>
	class fixed_vector
	{
	private:
		union // disable RAII
		{
			_T _data[_Capacity];
		};
		
		size_t _size = 0;

		_Alloc _alloc;

	public:
		fixed_vector();
		~fixed_vector();

		bool empty();
		size_t size();
		size_t capacity();

		_T& operator[] (size_t index);
		const _T& operator[] (size_t index) const;

		template <typename... _Args>
		_T& emplace_back(_Args&&... arguments);

		_T& push_back(const _T&& copy);

		_T* begin();
		_T* end();

		const _T* begin() const;
		const _T* end() const;

		void clear();
	};

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline fixed_vector<_T, _Capacity, _Alloc>::fixed_vector()
	{

	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline fixed_vector<_T, _Capacity, _Alloc>::~fixed_vector()
	{
		clear();
	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline bool fixed_vector<_T, _Capacity, _Alloc>::empty()
	{
		return _size == 0;
	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline size_t fixed_vector<_T, _Capacity, _Alloc>::size()
	{
		return _size;
	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline size_t fixed_vector<_T, _Capacity, _Alloc>::capacity()
	{
		return _Capacity;
	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline _T& fixed_vector<_T, _Capacity, _Alloc>::operator[] (size_t index)
	{
		assert(index < _size);
		return _data[index];
	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline const _T& fixed_vector<_T, _Capacity, _Alloc>::operator[] (size_t index) const
	{
		assert(index < _size);
		return _data[index];
	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	template <typename... _Args>
	inline _T& fixed_vector<_T, _Capacity, _Alloc>::emplace_back(_Args&&... arguments)
	{
		if (_size >= _Capacity)
			throw std::out_of_range("Can't emplace element, size >= capacity");

		_T& element = _data[_size++];
		std::allocator_traits<_Alloc>::construct(_alloc, &element, std::forward<_Args>(arguments)...);
		//new (&element) _T(std::forward<_Args>(arguments)...);

		return element;
	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline _T& fixed_vector<_T, _Capacity, _Alloc>::push_back(const _T&& copy)
	{
		if (_size >= _Capacity)
			throw std::out_of_range("Can't push element, size >= capacity");

		_T& element = _data[_size++];
		element = copy;

		return element;
	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline void fixed_vector<_T, _Capacity, _Alloc>::clear()
	{
		for (size_t i = 0; i < _size; ++i)
		{
			std::allocator_traits<_Alloc>::destroy(_alloc, _data + i);
			//_data[i].~_T();
		}
	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline _T* fixed_vector<_T, _Capacity, _Alloc>::begin()
	{
		return _data;
	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline _T* fixed_vector<_T, _Capacity, _Alloc>::end()
	{
		return _data + _size;
	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline const _T* fixed_vector<_T, _Capacity, _Alloc>::begin() const
	{
		return _data;
	}

	template <typename _T, size_t _Capacity, typename _Alloc>
	inline const _T* fixed_vector<_T, _Capacity, _Alloc>::end() const
	{
		return _data + _size;
	}
}