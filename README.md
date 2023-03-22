# Entity-Component-System
Heavily templated ECS system, focused on bringing the least amount of overhead, where components are known by compile time.
Alpha state, still a WIP.

# Query examples

Notes:
* `<Type>.data` is just a field of the component.

Call lambda on all entities that have the `One` and `Two` components and do operations on both
```cpp
world.query([](One& one, Two& two) -> void
{
	two.data = _mm_mul_ps(two.data, a);
	two.data = _mm_mul_ps(two.data, two.data);

	one.data = _mm_mul_ps(one.data, a);
	one.data = _mm_mul_ps(one.data, one.data);
});
```

Call lambda on all entities that have the `One` and `Four`, but not `Three` components and do operations on `Four`
Go through all entities that don't have the component `One` and do something with the entity id
```cpp
world.query<One, ecs::exclude<Three>>([](Four& four) -> void
{
	four.data = _mm_mul_ps(four.data, a);
	four.data = _mm_mul_ps(four.data, four.data);
});
```

Go through all entities that don't have the component `One` and do something with the entity id
```cpp
world.query<ecs::exclude<One>>([](Entity entity) -> void
{
	// do something with `entity`
});
```

# Benchmark
Notes:
* Check the [benchmark/Main.cpp](https://github.com/Fortahr/ecs/blob/main/benchmark/Main.cpp) file for the all the code that's being benchmarked,
* I mostly compare `bucket<0*>'` and `bucket<0*>'v` with `query<0*>_` as they actually do some work (4x4 matrix multiplications),
* Results fluctuate, like the winner in above comparison can change each run, but they are always around the same numbers,
* Archetypes with only 1 component, run around the same speed as its raw counterpart,
* Simple, single-ish instructions seem to come out as a bit slower than its raw counterpart (needs confirmation).
```
Test name           run 1      run 2      run 3      run 4      run 5      run 6      run 7      run 8   ent.#     avg.time/op

== non ECS tests for comparison ==
raw<2*>'          65.0758    34.4843    36.0136    41.4713    34.6519    33.7337    33.8108    34.2884     20M     1.9596ns/op
bucket<0*>'      271.3245   240.5398   249.0630   247.2940   249.7214   237.0558   238.8882   241.0861     10M    24.6872ns/op
bucket<0*>'v     273.5992   245.7274   239.7482   242.3971   241.3381   244.3304   243.8623   242.2596     10M    24.6658ns/op
bucket<2*>'       93.6958    68.0411    62.4222    61.3711    67.5850    60.9035    59.3761    62.3626     30M     2.2323ns/op

== queries ==
query<0*>_       264.7137   238.9366   239.1758   238.7612   243.8389   237.8649   235.6786   237.0654     10M    24.2004ns/op
query<1=>_        41.9882    18.1537    22.2429    18.8166    18.4037    18.3167    18.2307    18.1903     10M     2.1793ns/op
query<1, 2*>_     23.9972    24.3273    24.0846    27.9223    25.4841    24.6972    25.6880    25.1450     10M     2.5168ns/op
query<2*>_       106.9369    66.6049    66.9318    70.4740    68.5526    65.7817    68.9608    67.5692     30M     2.4242ns/op
query<1*, 2*>     34.9617    34.9027    35.7537    37.6024    35.2290    38.8559    34.8807    34.7602     10M     3.5868ns/op
query<1*, 2*>_    35.4229    31.7984    31.4284    31.6545    31.5571    31.5085    31.6658    34.2357     10M     3.2409ns/op

== counting ==
query<1, 2>_       1.8508     1.8498     1.8405     1.3252     1.8372     1.8582     1.8311     1.8337     10M     0.1778ns/op
count<1, 2>        0.0001     0.0001     0.0001     0.0000     0.0001     0.0001     0.0001     0.0001     10M     0.0000ns/op
query<3>_          2.7914     3.4180     3.3665     3.5695     3.4005     3.6382     3.5112     3.4071     20M     0.1694ns/op
count<3, !1>       0.0001     0.0001     0.0000     0.0001     0.0001     0.0001     0.0000     0.0000     10M     0.0000ns/op
query<3, !1>_      1.8200     2.0571     2.0501     1.8957     1.7368     1.5088     1.5050     1.8284     10M     0.1800ns/op
query<E, !1>_      5.3309     6.6640     7.2202     7.4838     7.4611     7.0713     5.0262     7.2896     40M     0.1673ns/op

== explanation ==
0-9  Component to find (0: 4x4 matrix, others are single __m128 packed floats)
E    Entity (parameter input)

=    Assignment operation on component
*    Multiplication operation on component

!    Exclude component
'    Manual inlined code (no function/lambda)
_    Lambda
v    Obsolete, evaluates reinterpret overhead due to potential optimization misses.
```

# WIP
1. Entity id system still needs testing,
2. Requires production environment testing.
