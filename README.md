# Parallel Computing — Task I

This is an attempt at Task I from the WebClub NITK Systems & Security SIG
Parallel Computing recruitment task.

The program creates a random array of 64-bit unsigned integers and finds its
sum in three ways:

1. One normal sequential loop.
2. Four threads where thread `i` sums indexes for which `index % 4 == i`.
3. Four threads where thread `i` sums one continuous quarter of the array.

The same array is used for every method. The program checks that all three sums
match on every run before printing timing averages.

## Why two threaded strategies?

Both use exactly four threads as required. The first strategy spreads each
thread's reads throughout the whole array. The second gives each thread one
continuous piece of the array. I will compare measured results on Linux rather
than assuming one is always faster.

## Build and run on Linux

```sh
make
./parallel_sum 10000000 5
```

The first argument is the array size `N` (minimum 1024). The second is the
number of timing runs. For example:

```sh
./parallel_sum 1000000 5
./parallel_sum 10000000 5
./parallel_sum 50000000 5
```

## Running Linux on macOS with Docker Desktop

From this repository on a Mac with Docker Desktop:

```sh
docker build -t parallel-sum-linux .
docker run --rm parallel-sum-linux
```

To change the array size or number of runs:

```sh
docker run --rm parallel-sum-linux ./parallel_sum 50000000 5
```

This compiles and runs the program in a Linux container. The timings should be
recorded from the same Linux environment used for the final demonstration.

## Results

The program was built and run in the `alpine:3.21` Linux container through
Docker Desktop on macOS. Linux reported 8 online CPUs. Each measurement below
is the average of five runs; thread creation is included in the threaded times.

| Array size | Sequential | 4-thread strided | 4-thread chunked | Fastest |
| ---: | ---: | ---: | ---: | --- |
| 10,000,000 | 4.488 ms | 5.376 ms | 2.369 ms | Chunked |
| 50,000,000 | 22.879 ms | 18.007 ms | 9.238 ms | Chunked |

All three methods printed `sums match` on every run. The final sums were
computed using `uint64_t`, so addition wraps modulo `2^64` in the same way for
all strategies.

For the 10 million element run, the output was:

```text
Run 1: sequential 4.206 ms | strided 6.470 ms | chunked 2.345 ms | sums match
Run 2: sequential 4.553 ms | strided 4.344 ms | chunked 2.957 ms | sums match
Run 3: sequential 4.691 ms | strided 6.714 ms | chunked 2.322 ms | sums match
Run 4: sequential 4.559 ms | strided 4.178 ms | chunked 2.181 ms | sums match
Run 5: sequential 4.429 ms | strided 5.173 ms | chunked 2.041 ms | sums match
Average sequential: 4.488 ms
Average strided:    5.376 ms
Average chunked:    2.369 ms
```

On this machine, continuous chunks were fastest. A chunked thread reads a
continuous part of the array, while the strided version jumps through the whole
array. The exact numbers can vary between systems and Docker runs, but the
comparison was made with the same input and environment for all three methods.
