#include <iostream>
#include <tuple>
#include <chrono>
#include <functional>
#include <deque>
#include <iomanip>

#include <ecs/world.h>

#include "EngineRegistry.h"

using namespace Esteem;

const __m128 a = _mm_set1_ps(4.f);
const __m128 b = _mm_set1_ps(8.f);

inline void Test2(One& one, Two& two)
{
	two.data = _mm_mul_ps(two.data, a);
	two.data = _mm_mul_ps(two.data, two.data);

	one.data = _mm_mul_ps(one.data, a);
	one.data = _mm_mul_ps(one.data, one.data);
}

#define BEGIN_TEST(V) std::cout << V; \
[&]() __declspec(noinline) \
{ \
	double delta = .0; \
	double totalTime = .0; \
	for (size_t i = 0; i < 8; ++i) \
	{ \
		processed = 0; \
		start = std::chrono::high_resolution_clock::now();

#define END_TEST(COUNT) \
		end = std::chrono::high_resolution_clock::now(); \
		totalTime += delta = std::chrono::duration<double, std::milli>(end - start).count(); \
		std::cout << ' ' << std::setw(10) << std::fixed << std::setprecision(4) << delta; \
	} \
	\
	std::cout << ' ' << std::setw(6) << (COUNT / 1'000'000) << "M " << \
		std::setw(10) << ((double(totalTime) / double(8 * COUNT)) * 1'000'000) << "ns/op\n"; \
}();

int main()
{
	ecs::world<EngineRegistry<Six, Seven, Eight, Nine>> world;
	//FillWorld(world);

	std::vector<std::array<Esteem::Two, 64>*> base1;
	std::vector<ecs::details::archetype_storage<64, Esteem::Two>::bucket*> base2;
	std::vector<ecs::details::archetype_storage<64, Esteem::Zero>::bucket*> base0;
	std::vector<void*> base0void_;
	auto& base0void = reinterpret_cast<std::vector<ecs::details::archetype_storage<64, Zero>::bucket*>&>(base0void_);

	//world.reserve_entities<Zero, One, Two, Three>(10'000'000);
	//world.reserve_entities<Three, Four, Five>(10'000'000);
	//world.reserve_entities<Two>(10'000'000);
	base1.resize(20'000'000 / 64);
	base2.resize(30'000'000 / 64);
	base0.resize(10'000'000 / 64);
	base0void.resize(10'000'000 / 64);

	for (auto*& bucket : base1)
		bucket = new std::array<Two, 64>();

	for (auto*& bucket : base2)
		bucket = new ecs::details::archetype_storage<64, Two>::bucket();

	for (auto*& bucket : base0)
		bucket = new ecs::details::archetype_storage<64, Zero>::bucket();

	for (auto*& bucket : base0void)
		bucket = new ecs::details::archetype_storage<64, Zero>::bucket();

	for (size_t i = 0; i < 10'000'000; ++i)
		world.emplace_entity<One, Two, Three>();

	for (size_t i = 0; i < 10'000'000; ++i)
		world.emplace_entity<Two>();

	for (size_t i = 0; i < 10'000'000; ++i)
		world.emplace_entity<Two, Four, Five>();

	for (size_t i = 0; i < 10'000'000; ++i)
		world.emplace_entity<Three, Six>();

	for (size_t i = 0; i < 10'000'000; ++i)
		world.emplace_entity<Zero>();

	std::chrono::high_resolution_clock::time_point start, end;
	size_t processed;

	std::cout << "== non ECS tests for comparison ==\n";
	{
		BEGIN_TEST("raw<2*>'      ")
			for (auto& bucket : base1)
			{
				for (Two& two : *bucket)
				{
					two.data = _mm_mul_ps(two.data, a);
					two.data = _mm_mul_ps(two.data, two.data);
				}
			}
		END_TEST(base1.size() * std::size(**base1.data()));


		BEGIN_TEST("bucket<0*>'   ")
			for (auto& bucket : base0)
			{
				for (Zero& zero : bucket->get<Zero>())
				{
					zero.data *= zero.data;
				}
			}
		END_TEST(base0.size() * std::size(**base0.data()));


		BEGIN_TEST("bucket<0*>'v  ")
			for (auto& bucket : base0void)
			{
				for (Zero& zero : bucket->get<Zero>())
				{
					zero.data *= zero.data;
				}
			}
		END_TEST(base0void.size() * std::size(**base0void.data()));


		BEGIN_TEST("bucket<2*>'   ")
			for (auto& bucket : base2)
			{
				for (Two& two : bucket->get<Two>())
				{
					two.data = _mm_mul_ps(two.data, a);
					two.data = _mm_mul_ps(two.data, two.data);
				}
			}
		END_TEST(base2.size()* std::size(**base2.data()));
	}

	std::cout << "\n== queries ==\n";
	{
		BEGIN_TEST("query<0*>_    ")
			world.query([](Zero& zero) -> void
				{
					zero.data *= zero.data;
				});
		END_TEST(world.count<Zero>());


		BEGIN_TEST("query<1=>_    ")
			world.query([](One& one) -> void
				{
					one.data = _mm_set1_ps(4.f);
				});
		END_TEST(world.count<One>());


		BEGIN_TEST("query<1, 2*>_ ")
			world.query([](One& one, Two& two) -> void
				{
					two.data = _mm_mul_ps(two.data, a);
					two.data = _mm_mul_ps(two.data, two.data);
				});
		END_TEST((world.count<One, Two>()));

		BEGIN_TEST("query<2*>_    ")
			world.query([](Two& two) -> void
				{
					two.data = _mm_mul_ps(two.data, a);
					two.data = _mm_mul_ps(two.data, two.data);
				});
		END_TEST(world.count<Two>());


		BEGIN_TEST("query<1*, 2*> ")
			world.query(Test2);
		END_TEST((world.count<One, Two>()));


		BEGIN_TEST("query<1*, 2*>_")
			world.query([](One& one, Two& two) -> void
				{
					two.data = _mm_mul_ps(two.data, a);
					two.data = _mm_mul_ps(two.data, two.data);

					one.data = _mm_mul_ps(one.data, a);
					one.data = _mm_mul_ps(one.data, one.data);
				});
		END_TEST((world.count<One, Two>()));
	}

	std::cout << "\n== counting ==\n";
	BEGIN_TEST("query<1, 2>_  ")
		world.query([&processed](One& one, Two& two) -> void
			{
				processed++;
			});
	END_TEST(processed);


	BEGIN_TEST("count<1, 2>   ")
		processed = world.count<One, Two>();
	END_TEST(processed);


	BEGIN_TEST("query<3>_     ")
		world.query([&processed](Three& three) -> void
			{
				processed++;
			});
	END_TEST(processed);


	BEGIN_TEST("count<3, !1>  ")
		processed = world.count<Three, ecs::exclude<One>>();
	END_TEST(processed);

	BEGIN_TEST("query<3, !1>_ ")
		world.query<ecs::exclude<One>>([&processed](Three& three) -> void
			{
				processed++;
			});
	END_TEST(processed);
	
	BEGIN_TEST("query<E, !1>_ ")
		world.query<ecs::exclude<One>>([&processed](Entity entity) -> void
			{
				processed++;
			});
	END_TEST(processed);
}
