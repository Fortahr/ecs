#include <iostream>
#include <array>
#include <tuple>
#include <chrono>
#include <functional>
#include <deque>

#include <ecs/world.h>

#include "Components.h"
#include "Benchmarker.h"

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

template <typename T>
std::vector<T*> create_vector(size_t size)
{
	std::vector<T*> vector;
	vector.reserve(size);

	for (size_t i = 0; i < vector.capacity(); ++i)
		vector.push_back(new T());

	return vector;
}

template <typename... Components>
void emplace_entities(ecs::world& world, size_t size)
{
	for (size_t i = 0; i < size; ++i)
		world.emplace_entity<Components...>();
}

template <typename... Components>
void benchmark_counting(ecs::world& world, std::string_view name)
{
	std::cout << name << "\n";
	{
		size_t processed = 0;

		Benchmarker::benchmark("  ECS query", [&]
			{
				processed = 0;

				world.query<Components...>([&processed]() -> void
					{
						processed++;
					});
			},
			processed
		);

		Benchmarker::benchmark("  ECS count", [&]
			{
				processed = world.count<Components...>();
			},
			processed
		);
	}
	std::cout << '\n';
}

int main()
{
	setlocale(LC_CTYPE, "");

	Benchmarker::print_header();

	ecs::world world;
	//FillWorld(world);
	//world.reserve_entities<Zero, One, Two, Three>(10'000'000);
	//world.reserve_entities<Three, Four, Five>(10'000'000);
	//world.reserve_entities<Two>(10'000'000);

	auto baseRaw0 = create_vector<std::array<Esteem::Zero, ecs::config::bucket_size>>(10'000'000 / ecs::config::bucket_size);
	auto baseRaw2 = create_vector<std::array<Esteem::Two, ecs::config::bucket_size>>(30'000'000 / ecs::config::bucket_size);
	auto baseBucket2 = create_vector<ecs::details::archetype_storage<Esteem::Two>::bucket>(30'000'000 / ecs::config::bucket_size);
	auto baseBucket0 = create_vector<ecs::details::archetype_storage<Esteem::Zero>::bucket>(10'000'000 / ecs::config::bucket_size);

	emplace_entities<One, Two, Three>(world, 10'000'000);
	emplace_entities<Two>(world, 10'000'000);
	emplace_entities<Two, Four, Five>(world, 10'000'000);
	emplace_entities<Three, Six>(world, 10'000'000);
	emplace_entities<Zero>(world, 10'000'000);

	std::chrono::high_resolution_clock::time_point start, end;

	std::cout << "(Two&) multiplication:\n";
	{
		Benchmarker::benchmark("  Raw buckets", [&]
			{
				for (auto& bucket : baseRaw2)
				{
					for (Two& two : *bucket)
					{
						two.data = _mm_mul_ps(two.data, a);
						two.data = _mm_mul_ps(two.data, two.data);
					}
				}
			},
			baseRaw2.size() * std::size(**baseRaw2.data())
		);

		Benchmarker::benchmark("  ECS buckets", [&]
			{
				for (auto& bucket : baseBucket2)
				{
					for (Two& two : bucket->get<Two>()._elements)
					{
						two.data = _mm_mul_ps(two.data, a);
						two.data = _mm_mul_ps(two.data, two.data);
					}
				}
			},
			baseBucket2.size() * std::size(**baseBucket2.data())
		);

		Benchmarker::benchmark("  ECS query", [&]
			{
				world.query([](Two& two) -> void
					{
						two.data = _mm_mul_ps(two.data, a);
						two.data = _mm_mul_ps(two.data, two.data);
					});
			},
			world.count<Two>()
		);
	}
	std::cout << '\n';

	std::cout << "(Zero&) matrix 4x4 multiplication:\n";
	{
		Benchmarker::benchmark("  Raw buckets", [&]
			{
				for (auto& bucket : baseRaw0)
				{
					for (Zero& zero : *bucket)
					{
						zero.data *= zero.data;
					}
				}
			},
			baseBucket0.size() * std::size(**baseBucket0.data())
		);

		Benchmarker::benchmark("  ECS buckets", [&]
			{
				for (auto& bucket : baseBucket0)
				{
					for (Zero& zero : bucket->get<Zero>()._elements)
					{
						zero.data *= zero.data;
					}
				}
			},
			baseBucket0.size() * std::size(**baseBucket0.data())
		);

		Benchmarker::benchmark("  ECS query", [&]
			{
				world.query([](Zero& zero) -> void
					{
						zero.data *= zero.data;
					});
			},
			world.count<Zero>()
		);
	}
	std::cout << '\n';

	std::cout << "(One&):\n";
	{
		Benchmarker::benchmark("  One=4.f", [&]
			{
				world.query([](One& one) -> void
					{
						one.data = _mm_set1_ps(4.f);
					});
			},
			world.count<One>()
		);
	}
	std::cout << '\n';

	std::cout << "(One&, Two&):\n";
	{
		Benchmarker::benchmark("  Two*=a²", [&]
			{
				world.query([](One& one, Two& two) -> void
					{
						two.data = _mm_mul_ps(two.data, a);
						two.data = _mm_mul_ps(two.data, two.data);
					});
			},
			world.count<One, Two>()
		);
	}
	std::cout << '\n';

	std::cout << "(One&, Two&) multiplication:\n";
	{
		Benchmarker::benchmark("  Function pointer", [&]
			{
				world.query(Test2);
			},
			world.count<One, Two>()
		);


		Benchmarker::benchmark("  Lambda", [&]
			{
				world.query([](One& one, Two& two) -> void
					{
						two.data = _mm_mul_ps(two.data, a);
						two.data = _mm_mul_ps(two.data, two.data);

						one.data = _mm_mul_ps(one.data, a);
						one.data = _mm_mul_ps(one.data, one.data);
					});
				},
			world.count<One, Two>()
		);
	}
	std::cout << '\n';


	benchmark_counting<One, Two>(world, "(One, Two) counting");
	benchmark_counting<Three>(world, "(Three) counting");
	benchmark_counting<Three, ecs::exclude<One>>(world, "(Three, !One) counting");
	benchmark_counting<Entity>(world, "(Entity) counting");
}
