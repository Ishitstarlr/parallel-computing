# Task I — Array sum comparison

This program creates a random array of 64-bit unsigned integers and finds its
sum in three ways:

1. One normal sequential loop.
2. Four threads where thread `i` sums indexes for which `index % 4 == i`.
3. Four threads where thread `i` sums one continuous quarter of the array.

The same array is used for every method. The program checks that all three sums
match on every run before printing timing averages.

## Why two threaded strategies?

Both use exactly four threads as required. The first strategy spreads each
thread's reads throughout the whole array. The second gives each thread one
continuous piece of the array. Results are measured rather than assumed.

## Build and run

From this folder on Linux:

```sh
make
./parallel_sum 10000000 5
```

The first argument is array size `N` (minimum 1024). The second is the number
of timing runs. For example:

```sh
./parallel_sum 1000000 5
./parallel_sum 10000000 5
./parallel_sum 50000000 5
```

## Linux Docker results

The program was built and run in `alpine:3.21` through Docker Desktop on macOS.
Linux reported 8 online CPUs. Each measurement is the average of five runs;
thread creation is included in threaded times.

| Array size | Sequential | 4-thread strided | 4-thread chunked | Fastest |
| ---: | ---: | ---: | ---: | --- |
| 10,000,000 | 4.488 ms | 5.376 ms | 2.369 ms | Chunked |
| 50,000,000 | 22.879 ms | 18.007 ms | 9.238 ms | Chunked |

All three methods printed `sums match` on every run. The final sums are
computed with `uint64_t`, so addition wraps modulo `2^64` consistently for all
three strategies.

On this machine, continuous chunks were fastest. A chunked thread reads a
continuous part of the array, while the strided version jumps through the whole
array. Exact timing varies by CPU and system load, but every comparison used the
same input and environment.
