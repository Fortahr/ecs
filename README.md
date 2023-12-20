# Entity-Component-System

Heavily templated ECS system, focused on bringing the least amount of overhead, where components are known at compile time.

* Alpha state, still a WIP,
* Archetype ECS Model,
* Header only,
* C++17,
* Pre-registered components,
* Makes use of STL containers, e.g.: `vector` and `tuple`,
* Bitmask component lookups, currently up to 64 components,
* Query components by function/lambda parameters,
* Exclude components in queries,
* Up to 256 worlds supported, keeping the `entity` type at 8 bytes of size,


# Query examples

Call lambda on all entities that have the `One` and `Two` components and do operations on both
```cpp
world.query([](One& one, Two& two) -> void
{
	// do something with `one` and/or `two`
});
```

Call lambda on all entities that have the `One` and `Four`, but not `Three` components and do operations on `Four`
```cpp
world.query<One, ecs::exclude<Three>>([](Four& four) -> void
{
	// do something with `four`
});
```

Call lambda on all entities that don't have the component `One` and do something with the entity id
```cpp
world.query<ecs::exclude<One>>([](Entity entity) -> void
{
	// do something with `entity`
});
```

Count all entities that have the `One`, `Six`, and `Seven`, but not `Three` components.
```cpp
size_t count = world.count<One, Six, Seven, ecs::exclude<Three>>();
```

# Benchmark

Notes:
* See [benchmark/Main.cpp](https://github.com/Fortahr/ecs/blob/main/benchmark/Main.cpp) and [benchmark/Benchmarker.h](https://github.com/Fortahr/ecs/blob/main/benchmark/Benchmarker.h) for the the benchmark code,
* Results fluctuate, like the winner of above comparison changes each run, but are always close,
* Archetypes with only 1 component run around the same speed as its raw counterpart,
* Archetypes with more than 1 component run slower than raw with only 1 component,
* Simple, single-ish instructions seem to come out as a bit slower than its raw counterpart (needs confirmation).

## MSVC with AVX2 support
```
Test                        Count       Median         Mean       StdDev       CI / 2
(Two&) multiplication:
  Raw buckets                 30M      1.61341      1.63667      0.07968      0.02290
  ECS buckets                 30M      1.93362      1.96762      0.05667      0.01628
  ECS query                   30M      2.24501      2.28106      0.08270      0.02376

(Zero&) matrix 4x4 multiplication:
  Raw buckets                 10M     17.81684     17.80991      0.13152      0.03779
  ECS buckets                 10M     18.07440     18.08647      0.16882      0.04851
  ECS query                   10M     18.03297     18.06221      0.18721      0.05379

(One&):
  One=4.f                     10M      1.87868      1.94202      0.14241      0.04092

(One&, Two&):
  Two*=a²                     10M      2.44400      2.53864      0.21919      0.06298

(One&, Two&) multiplication:
  Function pointer            10M      5.03163      5.26202      0.37485      0.10771
  Lambda                      10M      3.83392      3.97265      0.22968      0.06600

(One, Two) counting
  ECS query                   10M      0.12764      0.13304      0.01627      0.00468
  ECS count                   10M      0.00000      0.00000      0.00001      0.00000

(Three) counting
  ECS query                   20M      0.12879      0.13271      0.01097      0.00315
  ECS count                   20M      0.00000      0.00000      0.00000      0.00000

(Three, !One) counting
  ECS query                   10M      0.12692      0.13377      0.01321      0.00380
  ECS count                   10M      0.00000      0.00000      0.00001      0.00000

(Entity) counting
  ECS query                   50M      0.13133      0.13142      0.00375      0.00108
  ECS count                   50M      0.00000      0.00000      0.00000      0.00000

* values are given in nanoseconds, calculated as: totalTime / Count.
```

# WIP

2. Entity id system needs testing,
3. Production environment testing,
4. Task Scheduler,
4. R&D for omission of global entity id reservations, reducing memory footprint, some ideas;
	1. grouping, 1 id for a whole group (e.g.: static geometry),
	2. entity is never targeted, removed with world,
	3. only reserve id when actually targeted.
