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
* See [benchmark/Main.cpp](https://github.com/Fortahr/ecs/blob/main/benchmark/Main.cpp) for the the benchmark code,
* I mostly compare `bucket<0*>'` and `bucket<0*>'v` with `query<0*>_` as they actually do work (4x4 matrix multiplications),
* Results fluctuate, like the winner of above comparison changes each run, but are always close,
* Archetypes with only 1 component run around the same speed as its raw counterpart,
* Archetypes with more than 1 component run slower than raw with only 1 component,
* Simple, single-ish instructions seem to come out as a bit slower than its raw counterpart (needs confirmation).

<details>
<summary>MSVC with AVX2 support</summary>

```
Test name           run 1      run 2      run 3      run 4      run 5      run 6      run 7      run 8   ent.#     avg.time/op

== non ECS tests for comparison ==
raw<2*>'          32.3157    32.4407    32.3123    37.8498    32.2302    32.0441    32.3974    37.6322     20M     1.6826ns/op
bucket<0*>'      186.5257   186.8717   187.1009   187.6867   187.8803   187.5602   189.4021   187.4570     10M    18.7561ns/op
bucket<0*>'v     186.1202   185.8411   191.5940   194.3817   186.4195   187.0899   186.8392   185.2295     10M    18.7939ns/op
bucket<2*>'       58.3769    61.8686    59.8934    60.5815    62.9482    63.5913    60.1971    58.3236     30M     2.0241ns/op

== queries ==
query<0*>_       184.5911   186.2881   184.3497   185.2179   185.5310   185.6525   188.6758   184.5053     10M    18.5601ns/op
query<1=>_        20.4201    19.3007    19.8114    20.7901    19.0269    18.9761    19.1509    20.1484     10M     1.9703ns/op
query<1, 2*>_     25.6831    26.0038    25.6350    25.4649    24.5713    24.6802    24.5947    28.3017     10M     2.5617ns/op
query<2*>_        68.2834    67.4573    71.0837    67.2431    66.9042    71.0874    68.6399    67.0663     30M     2.2824ns/op
query<1*, 2*>     54.3241    52.0642    53.7581    53.0656    52.8810    51.6022    52.9664    52.5022     10M     5.2895ns/op
query<1*, 2*>_    42.2156    39.4935    41.5285    39.3814    40.0748    42.9833    40.3733    39.7693     10M     4.0727ns/op

== counting ==
query<1, 2>_       1.2393     1.4638     1.7770     1.6639     1.3500     1.4309     1.5769     1.2824     10M     0.1473ns/op
count<1, 2>        0.0001     0.0002     0.0001     0.0001     0.0001     0.0001     0.0001     0.0001     10M     0.0000ns/op
query<3>_          2.5724     2.6407     2.5421     2.5750     2.5456     2.6920     2.5160     2.5106     20M     0.1287ns/op
count<3, !1>       0.0002     0.0002     0.0001     0.0003     0.0001     0.0001     0.0002     0.0001     10M     0.0000ns/op
query<3, !1>_      1.7682     1.6491     1.5490     1.7812     1.3696     1.7111     1.8046     1.8607     10M     0.1687ns/op
query<E, !1>_      5.8915     7.2330     5.0008     5.3596     6.1713     5.9104     5.2711     5.0480     40M     0.1434ns/op
```
</details>

<details>
<summary>GCC no SIMD instruction explicitly set (AVX2 was slower)</summary>

```
Test name           run 1      run 2      run 3      run 4      run 5      run 6      run 7      run 8   ent.#     avg.time/op

== non ECS tests for comparison ==
raw<2*>'          33.4490    33.1730    33.4590    33.8040    33.7900    34.3510    33.1200    33.2880     20M     1.6777ns/op
bucket<0*>'       80.5050    80.4330    79.8490    79.5640    79.9360    80.0220    80.3830    79.8170     10M     8.0064ns/op
bucket<0*>'v      81.5530    79.6690    80.2050    80.2150    79.7810    80.1250    79.3320    80.5830     10M     8.0183ns/op
bucket<2*>'       60.2140    59.7490    60.5200    61.0490    60.1580    60.0860    60.7190    64.7950     30M     2.0304ns/op

== queries ==
query<0*>_        80.5390    81.8830    81.6590    81.6370    81.1730    80.7600    81.5300    81.2360     10M     8.1302ns/op
query<1=>_        20.0220    19.2790    19.1490    19.2400    19.0360    18.9360    19.5970    18.9820     10M     1.9280ns/op
query<1, 2*>_     25.3180    26.0180    25.4640    25.2730    26.5000    25.3530    26.7290    25.4340     10M     2.5761ns/op
query<2*>_        70.7140    71.3480    70.8380    70.9630    70.6390    70.7920    70.4010    71.3300     30M     2.3626ns/op
query<1*, 2*>     42.8010    42.2400    41.9550    42.7440    42.6830    42.2840    42.3580    42.4760     10M     4.2443ns/op
query<1*, 2*>_    42.7110    42.9580    42.2230    42.5610    42.9310    42.6730    42.0860    42.4870     10M     4.2579ns/op

== counting ==
query<1, 2>_       0.0000     0.0010     0.0000     0.0000     0.0000     0.0010     0.0000     0.0000     10M     0.0000ns/op
count<1, 2>        0.0000     0.0000     0.0000     0.0000     0.0010     0.0000     0.0000     0.0010     10M     0.0000ns/op
query<3>_          0.0000     0.0000     0.0000     0.0000     0.0000     0.0000     0.0000     0.0000     20M     0.0000ns/op
count<3, !1>       0.0000     0.0010     0.0010     0.0010     0.0010     0.0000     0.0010     0.0010     10M     0.0001ns/op
query<3, !1>_      0.0000     0.0010     0.0000     0.0010     0.0000     0.0010     0.0000     0.0000     10M     0.0000ns/op
query<E, !1>_      0.0010     0.0000     0.0000     0.0000     0.0000     0.0010     0.0000     0.0000     40M     0.0000ns/op
```
</details>

<details>
<summary>Explanation</summary>

```
0-9  Component to find, component 0 = 4x4 matrix (glm::mat4), other components are single __m128 packed floats
E    Entity (parameter input)

=    Assignment operation on component
*    Multiplication operation on component

!    Exclude component
'    Manual inlined code (no function/lambda)
_    Lambda
v    Obsolete, evaluates reinterpret overhead due to potential optimization misses.
```
</details>

# WIP

2. Entity id system needs testing,
3. Production environment testing,
4. Task Scheduler,
4. R&D for omission of global entity id reservations, reducing memory footprint, some ideas;
	1. grouping, 1 id for a whole group (e.g.: static geometry),
	2. entity is never targeted, removed with world,
	3. only reserve id when actually targeted.
