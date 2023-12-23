#pragma once

#include <type_traits>

namespace ecs::details
{
	template<typename _T, typename... _Args>
	class query_func
	{
	private:
		_T _func;

	public:
		constexpr query_func(_T&& func)
			: _func(func)
		{};

		template<typename... _InvokeArgs>
		constexpr void operator() (_InvokeArgs&&... args) const { _func(std::forward<_InvokeArgs>(args)...); }
	};

	// TODO: enable non-lambda member functions
	template<typename _Func, typename _Ret, typename... _Args>
	constexpr auto to_query_func(_Func&& func, _Ret(_Func::*)(_Args...))
	{
		return query_func<_Func, std::decay_t<_Args>...>(std::forward<_Func>(func));
	}

	template<typename _Func, typename _Ret, typename... _Args>
	constexpr auto to_query_func(_Func&& func, _Ret(_Func::*)(_Args...) const)
	{
		return query_func<_Func, std::decay_t<_Args>...>(std::forward<_Func>(func));
	}

	template<typename _Ret, typename... _Args>
	constexpr auto to_query_func(_Ret(*func)(_Args...))
	{
		return query_func<_Ret(*)(_Args...), std::decay_t<_Args>...>(std::forward<_Ret(*)(_Args...)>(func));
	}

	template<typename _Func>
	constexpr auto to_query_func(_Func&& func)
	{
		return to_query_func(std::forward<_Func>(func), &_Func::operator());
	};
}