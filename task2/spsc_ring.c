#include <errno.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

/* Capacity must be a power of two. One slot remains unused, making the
 * full and empty states unambiguous. */
#define RING_CAPACITY 8

typedef struct {
    uint64_t slots[RING_CAPACITY];
    atomic_size_t head; /* Written only by the producer. */
    atomic_size_t tail; /* Written only by the consumer. */
} SpscRing;

typedef struct {
    SpscRing *ring;
    size_t item_count;
    atomic_bool producer_done;
    size_t produced;
    size_t consumed;
    bool order_ok;
} ProgramState;

static size_t next_index(size_t index) {
    return (index + 1) & (RING_CAPACITY - 1);
}

/* Producer-only operation. The release store publishes a value only after it
 * has been written into its array slot. */
static bool ring_try_push(SpscRing *ring, uint64_t value, size_t *slot_used) {
    size_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    size_t next_head = next_index(head);
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (next_head == tail) {
        return false;
    }

    ring->slots[head] = value;
    /* Print before publishing head, so consumer cannot display a removal of
     * this value before the matching add line is visible. */
    printf("Producer -> added %llu at slot %zu\n", (unsigned long long)value, head);
    atomic_store_explicit(&ring->head, next_head, memory_order_release);
    *slot_used = head;
    return true;
}

/* Consumer-only operation. The acquire load guarantees that an observed head
 * refers to a slot whose value was already written by the producer. */
static bool ring_try_pop(SpscRing *ring, uint64_t *value, size_t *slot_used) {
    size_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);

    if (tail == head) {
        return false;
    }

    *value = ring->slots[tail];
    /* Print before freeing this slot for producer reuse. */
    printf("Consumer <- removed %llu from slot %zu\n",
           (unsigned long long)*value, tail);
    atomic_store_explicit(&ring->tail, next_index(tail), memory_order_release);
    *slot_used = tail;
    return true;
}

static int producer(void *argument) {
    ProgramState *state = argument;

    for (size_t value = 1; value <= state->item_count; value++) {
        size_t slot;
        while (!ring_try_push(state->ring, value, &slot)) {
            thrd_yield();
        }
        state->produced++;
    }

    atomic_store_explicit(&state->producer_done, true, memory_order_release);
    return 0;
}

static int consumer(void *argument) {
    ProgramState *state = argument;
    uint64_t expected = 1;

    for (;;) {
        uint64_t value;
        size_t slot;

        if (ring_try_pop(state->ring, &value, &slot)) {
            if (value != expected) {
                state->order_ok = false;
            }
            expected++;
            state->consumed++;
            continue;
        }

        if (atomic_load_explicit(&state->producer_done, memory_order_acquire)) {
            /* The queue is empty and producer will not add another item. */
            break;
        }
        thrd_yield();
    }

    return 0;
}

static size_t parse_count(const char *text) {
    char *end = NULL;
    errno = 0;
    unsigned long long value = strtoull(text, &end, 10);

    if (errno != 0 || end == text || *end != '\0' || value == 0 || value > SIZE_MAX) {
        fprintf(stderr, "Item count must be a positive integer.\n");
        exit(EXIT_FAILURE);
    }
    return (size_t)value;
}

int main(int argc, char **argv) {
    size_t item_count = argc > 1 ? parse_count(argv[1]) : 20;
    SpscRing ring = {
        .head = ATOMIC_VAR_INIT(0),
        .tail = ATOMIC_VAR_INIT(0)
    };
    ProgramState state = {
        .ring = &ring,
        .item_count = item_count,
        .producer_done = ATOMIC_VAR_INIT(false),
        .produced = 0,
        .consumed = 0,
        .order_ok = true
    };
    thrd_t producer_thread;
    thrd_t consumer_thread;

    printf("SPSC ring buffer: capacity %d, usable slots %d\n",
           RING_CAPACITY, RING_CAPACITY - 1);
    printf("Starting one producer and one consumer for %zu values.\n\n", item_count);

    if (thrd_create(&producer_thread, producer, &state) != thrd_success ||
        thrd_create(&consumer_thread, consumer, &state) != thrd_success) {
        fprintf(stderr, "Could not create producer/consumer threads.\n");
        return EXIT_FAILURE;
    }

    thrd_join(producer_thread, NULL);
    thrd_join(consumer_thread, NULL);

    if (state.produced != item_count || state.consumed != item_count || !state.order_ok) {
        fprintf(stderr,
                "Verification failed: produced=%zu consumed=%zu order_ok=%s\n",
                state.produced, state.consumed, state.order_ok ? "true" : "false");
        return EXIT_FAILURE;
    }

    printf("\nVerified: produced %zu values, consumed %zu values, FIFO order preserved.\n",
           state.produced, state.consumed);
    return EXIT_SUCCESS;
}
