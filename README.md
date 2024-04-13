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
Test                            Count       Median         Mean       StdDev       CI / 2

(Two&) multiplication
  Raw buckets                     30M      1.55202      1.55644      0.01204      0.00346
  ECS buckets                     30M      1.95631      1.96062      0.02030      0.00583
  ECS query                       30M      2.21243      2.21480      0.02185      0.00628

(Zero&) matrix 4x4 multiplication
  Raw buckets                     10M     16.95202     16.94638      0.07077      0.02033
  ECS buckets                     10M     17.49242     17.50374      0.07741      0.02224
  ECS query                       10M     17.51966     17.50934      0.05898      0.01695
  ECS query entity                10M     17.51722     17.55157      0.16768      0.04818
  ECS query_mut entity            10M     18.28640     18.29692      0.10230      0.02939

(One&)
  One = 4.f                       10M      1.78094      1.79027      0.02400      0.00690

(One&, Two&)
  Two*=a²                         10M      2.57193      2.56842      0.02116      0.00608

(One&, Two&) multiplication
  Function pointer                10M      4.85793      5.00983      0.34365      0.09874
  Lambda                          10M      3.90494      4.02753      0.26628      0.07651

(One, Two) counting
  ECS query                       10M      0.12087      0.12468      0.01237      0.00355
  ECS count                       10M      0.00000      0.00000      0.00000      0.00000

(Three) counting
  ECS query                       20M      0.11952      0.12033      0.00212      0.00061
  ECS count                       20M      0.00000      0.00000      0.00000      0.00000

(Three, !One) counting
  ECS query                       10M      0.12543      0.12912      0.01023      0.00294
  ECS count                       10M      0.00000      0.00000      0.00000      0.00000

(entity) counting
  ECS query                       50M      0.12636      0.12624      0.00145      0.00042
  ECS count                       50M      0.00000      0.00000      0.00000      0.00000

Removing entities                 50M     17.81741     17.81741      0.00000      0.00000

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
