#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define THREAD_COUNT 4

typedef enum {
    STRIDED,
    CHUNKED
} WorkMode;

typedef struct {
    const uint64_t *array;
    size_t length;
    size_t thread_index;
    WorkMode mode;
    uint64_t partial_sum;
} ThreadWork;

static double elapsed_ms(struct timespec start, struct timespec end) {
    return (double)(end.tv_sec - start.tv_sec) * 1000.0 +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000.0;
}

/* A small deterministic generator is enough here: the task needs random input,
 * not cryptographic randomness. */
static uint64_t next_random(uint64_t *state) {
    *state ^= *state >> 12;
    *state ^= *state << 25;
    *state ^= *state >> 27;
    return *state * UINT64_C(2685821657736338717);
}

static void *sum_worker(void *argument) {
    ThreadWork *work = argument;
    uint64_t sum = 0;

    if (work->mode == STRIDED) {
        for (size_t index = work->thread_index; index < work->length; index += THREAD_COUNT) {
            sum += work->array[index];
        }
    } else {
        size_t start = work->thread_index * work->length / THREAD_COUNT;
        size_t end = (work->thread_index + 1) * work->length / THREAD_COUNT;
        for (size_t index = start; index < end; index++) {
            sum += work->array[index];
        }
    }

    work->partial_sum = sum;
    return NULL;
}

static uint64_t sequential_sum(const uint64_t *array, size_t length, double *time_ms) {
    struct timespec start;
    struct timespec end;
    uint64_t sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t index = 0; index < length; index++) {
        sum += array[index];
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    *time_ms = elapsed_ms(start, end);
    return sum;
}

static uint64_t threaded_sum(const uint64_t *array, size_t length, WorkMode mode, double *time_ms) {
    pthread_t threads[THREAD_COUNT];
    ThreadWork work[THREAD_COUNT];
    struct timespec start;
    struct timespec end;
    uint64_t total = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t index = 0; index < THREAD_COUNT; index++) {
        work[index] = (ThreadWork){
            .array = array,
            .length = length,
            .thread_index = index,
            .mode = mode,
            .partial_sum = 0
        };
        int result = pthread_create(&threads[index], NULL, sum_worker, &work[index]);
        if (result != 0) {
            fprintf(stderr, "pthread_create failed: %s\n", strerror(result));
            exit(EXIT_FAILURE);
        }
    }

    for (size_t index = 0; index < THREAD_COUNT; index++) {
        int result = pthread_join(threads[index], NULL);
        if (result != 0) {
            fprintf(stderr, "pthread_join failed: %s\n", strerror(result));
            exit(EXIT_FAILURE);
        }
        total += work[index].partial_sum;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    *time_ms = elapsed_ms(start, end);
    return total;
}

static size_t parse_positive_size(const char *text, const char *name, size_t minimum) {
    char *end = NULL;
    errno = 0;
    unsigned long long value = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value < minimum || value > SIZE_MAX) {
        fprintf(stderr, "%s must be an integer of at least %zu.\n", name, minimum);
        exit(EXIT_FAILURE);
    }
    return (size_t)value;
}

int main(int argc, char **argv) {
    size_t length = argc > 1 ? parse_positive_size(argv[1], "N", 1024) : 10000000;
    size_t runs = argc > 2 ? parse_positive_size(argv[2], "runs", 1) : 5;

    if (length > SIZE_MAX / sizeof(uint64_t)) {
        fprintf(stderr, "N is too large for this machine.\n");
        return EXIT_FAILURE;
    }

    uint64_t *array = malloc(length * sizeof(*array));
    if (array == NULL) {
        fprintf(stderr, "Could not allocate %zu bytes.\n", length * sizeof(*array));
        return EXIT_FAILURE;
    }

    uint64_t seed = UINT64_C(0x9e3779b97f4a7c15);
    for (size_t index = 0; index < length; index++) {
        array[index] = next_random(&seed);
    }

    printf("Array length: %zu unsigned 64-bit integers\n", length);
#ifdef _SC_NPROCESSORS_ONLN
    printf("Threads: %d | online CPUs reported by Linux: %ld\n",
           THREAD_COUNT, sysconf(_SC_NPROCESSORS_ONLN));
#else
    printf("Threads: %d\n", THREAD_COUNT);
#endif
    printf("Runs per strategy: %zu\n\n", runs);

    double sequential_total = 0;
    double strided_total = 0;
    double chunked_total = 0;
    uint64_t expected_sum = 0;

    for (size_t run = 1; run <= runs; run++) {
        double sequential_ms;
        double strided_ms;
        double chunked_ms;
        uint64_t sequential = sequential_sum(array, length, &sequential_ms);
        uint64_t strided = threaded_sum(array, length, STRIDED, &strided_ms);
        uint64_t chunked = threaded_sum(array, length, CHUNKED, &chunked_ms);

        if (run == 1) {
            expected_sum = sequential;
        }
        if (sequential != expected_sum || strided != expected_sum || chunked != expected_sum) {
            fprintf(stderr, "Sum mismatch on run %zu; stopping.\n", run);
            free(array);
            return EXIT_FAILURE;
        }

        sequential_total += sequential_ms;
        strided_total += strided_ms;
        chunked_total += chunked_ms;
        printf("Run %zu: sequential %.3f ms | strided %.3f ms | chunked %.3f ms | sums match\n",
               run, sequential_ms, strided_ms, chunked_ms);
    }

    printf("\nFinal sum (modulo 2^64): %" PRIu64 "\n", expected_sum);
    printf("Average sequential: %.3f ms\n", sequential_total / runs);
    printf("Average strided:    %.3f ms\n", strided_total / runs);
    printf("Average chunked:    %.3f ms\n", chunked_total / runs);

    free(array);
    return EXIT_SUCCESS;
}
