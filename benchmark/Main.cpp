#include <iostream>
#include <array>
#include <tuple>
#include <chrono>
#include <functional>
#include <deque>
#include <sstream>

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
	size_t processed = 0;

	Benchmarker::benchmark(name,
		Benchmarker::sub_run{ "ECS query", [&]
			{
				processed = 0;

				world.query<Components...>([&processed]() -> void
				{
					processed++;
				});
			},
			processed
		},
		Benchmarker::sub_run{ "ECS count", [&]
			{
				processed = world.count<Components...>();
			},
			processed
		}
	);
}

void test_entity_erasure(ecs::world& world)
{
	size_t p = 0, r = 0;

	Benchmarker::benchmark<1>("Removing entities", [&]
	{
		world.query_mutable([&](ecs::entity entity) -> void
		{
			if (!entity.valid() || p >= 50'000'000)
				std::cout << "wut\n";

			if (entity.get_id() % 2 && world.erase_entity(entity))
				++r;

			++p;
		});
	}, p);

	std::cout << " > Removed " << Benchmarker::format_count(r) << ", leaving " << Benchmarker::format_count(p - r);
	std::cout << '\n';
}

void test_entity_add_component(ecs::world& world)
{
	std::pair<uint32_t, uint32_t> testEntity = { 0, 30'000'002 };

	std::cout << "Adding component Two to entity " << testEntity.second << ' ';
	world.add_entity_component<Two>((ecs::entity&)testEntity);
	std::cout << "Done\n";
}

int main()
{
	setlocale(LC_CTYPE, "");

	std::cout << "Creating our world... ";
	decltype(ecs::world::create_world()) world = ecs::world::create_world();
	std::cout << "Done\n";

	//world.reserve_entities<Zero, One, Two, Three>(10'000'000);
	//world.reserve_entities<Three, Four, Five>(10'000'000);
	//world.reserve_entities<Two>(10'000'000);

	std::cout << "Filling comparison vectors with test components... ";
	auto baseRaw0 = create_vector<std::array<Esteem::Zero, ecs::config::bucket_size>>(10'000'000 / ecs::config::bucket_size);
	auto baseRaw2 = create_vector<std::array<Esteem::Two, ecs::config::bucket_size>>(30'000'000 / ecs::config::bucket_size);
	auto baseBucket2 = create_vector<ecs::details::archetype_storage<Esteem::Two>::bucket>(30'000'000 / ecs::config::bucket_size);
	auto baseBucket0 = create_vector<ecs::details::archetype_storage<Esteem::Zero>::bucket>(10'000'000 / ecs::config::bucket_size);
	std::cout << "Done\n";

	std::cout << "Filling our world with entities and their components... ";
	emplace_entities<One, Two, Three>(world, 10'000'000);
	emplace_entities<Two>(world, 10'000'000);
	emplace_entities<Two, Four, Five>(world, 10'000'000);
	emplace_entities<Three, Six>(world, 10'000'000);
	emplace_entities<Zero>(world, 10'000'000);
	std::cout << "Done\n";

	std::chrono::high_resolution_clock::time_point start, end;

	std::cout << "Executing benchmarks... (this may take a while)\n";

	Benchmarker::benchmark("(Two&) multiplication",
		Benchmarker::sub_run{ "Raw buckets", [&]
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
		},
		Benchmarker::sub_run{ "ECS buckets", [&]
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
		},
		Benchmarker::sub_run{ "ECS query", [&]
			{
				world.query([](Two& two) -> void
					{
						two.data = _mm_mul_ps(two.data, a);
						two.data = _mm_mul_ps(two.data, two.data);
					});
			},
			world.count<Two>()
		}
	);

	Benchmarker::benchmark("(Zero&) matrix 4x4 multiplication",
		Benchmarker::sub_run{ "Raw buckets", [&]
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
		},
		Benchmarker::sub_run{ "ECS buckets", [&]
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
		},
		Benchmarker::sub_run{ "ECS query", [&]
			{
				world.query([](Zero& zero) -> void
					{
						zero.data *= zero.data;
					});
			},
			world.count<Zero>()
		},
		Benchmarker::sub_run{ "ECS query entity", [&]
			{
				world.query([](Zero& zero, ecs::entity) -> void
					{
						zero.data *= zero.data;
					});
			},
			world.count<Zero>()
		},
		Benchmarker::sub_run{ "ECS query_mut entity", [&]
			{
				world.query_mutable([](Zero& zero, ecs::entity) -> void
					{
						zero.data *= zero.data;
					});
			},
			world.count<Zero>()
		}
	);

	Benchmarker::benchmark("(One&)",
		Benchmarker::sub_run{ "One = 4.f", [&]
			{
				world.query([](One& one) -> void
					{
						one.data = _mm_set1_ps(4.f);
					});
			},
			world.count<One>()
		}
	);

	Benchmarker::benchmark("(One&, Two&)",
		Benchmarker::sub_run{ "Two*=a²", [&]
			{
				world.query([](One& one, Two& two) -> void
					{
						two.data = _mm_mul_ps(two.data, a);
						two.data = _mm_mul_ps(two.data, two.data);
					});
			},
			world.count<One, Two>()
		}
	);

	Benchmarker::benchmark("(One&, Two&) multiplication",
		Benchmarker::sub_run{ "Function pointer", [&]
			{
				world.query(Test2);
			},
			world.count<One, Two>()
		},
		Benchmarker::sub_run{ "Lambda", [&]
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
		}
	);

	benchmark_counting<One, Two>(world, "(One, Two) counting");
	benchmark_counting<Three>(world, "(Three) counting");
	benchmark_counting<Three, ecs::ex<One>>(world, "(Three, !One) counting");
	benchmark_counting<ecs::entity>(world, "(entity) counting");

	test_entity_erasure(world);
	test_entity_add_component(world);

	std::cout << '\n';
	Benchmarker::print_results();
}
