# Task II — Lock-free SPSC ring buffer

This folder contains my Task II attempt. It implements a fixed-size queue using
an array of eight `uint64_t` slots, one producer thread, and one consumer thread.

## What SPSC means

- **Single producer:** only the producer thread adds values and advances `head`.
- **Single consumer:** only the consumer thread removes values and advances
  `tail`.

The queue is a ring: after slot 7, the next slot is 0. One slot stays unused,
so `head == tail` always means empty and `next(head) == tail` means full.

## Why it is lock-free

There is no mutex. The producer is the only writer of `head`; the consumer is
the only writer of `tail`. Both are C atomics from `stdatomic.h`.

When producer writes an item, it writes the array slot first and then
release-stores the new `head`. Consumer acquire-loads `head` before reading a
slot. This makes the producer's completed write visible before consumer reads
the announced item. Consumer similarly release-stores its new `tail` after it
has read an item, letting producer safely reuse that slot.

## Build and run

```sh
make
./spsc_ring 20
```

The optional number is how many values to send. The default is 20. The program
prints every add/remove operation and checks that every value was received once,
in FIFO order.

## Expected ending

```text
Verified: produced 20 values, consumed 20 values, FIFO order preserved.
```

## Linux verification

The program was built and run in the repository's Alpine Linux Docker image:

```sh
docker build -t parallel-sum-linux .
docker run --rm parallel-sum-linux ./task2/spsc_ring 20
docker run --rm parallel-sum-linux ./task2/spsc_ring 1000
```

The 20-item run printed every producer add and consumer removal. The 1,000-item
run ended with:

```text
Verified: produced 1000 values, consumed 1000 values, FIFO order preserved.
```
