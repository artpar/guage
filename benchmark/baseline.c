/*
 * Guage Benchmark Baselines - C Implementations
 *
 * These provide native C performance baselines for comparison with:
 *   - Guage interpreter
 *   - Guage JIT compiler
 *
 * Compile: cc -O2 -o baseline baseline.c -lm
 * Run: ./baseline [benchmark] [iterations] [param]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

/* High-resolution timer */
static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/* ============================================================================
 * Benchmark 1: Sum of Squares
 * Guage: (define sum-squares (lambda (n acc)
 *          (if (< n 1) acc (sum-squares (- n 1) (+ acc (* n n))))))
 * ============================================================================ */

static double sum_squares_recursive(double n, double acc) {
    if (n < 1.0) return acc;
    return sum_squares_recursive(n - 1.0, acc + n * n);
}

static double sum_squares_loop(double n) {
    double acc = 0.0;
    while (n >= 1.0) {
        acc += n * n;
        n -= 1.0;
    }
    return acc;
}

/* ============================================================================
 * Benchmark 2: Fibonacci (naive recursive)
 * Guage: (define fib (lambda (n)
 *          (if (< n 2) n (+ (fib (- n 1)) (fib (- n 2))))))
 * ============================================================================ */

static double fib_recursive(double n) {
    if (n < 2.0) return n;
    return fib_recursive(n - 1.0) + fib_recursive(n - 2.0);
}

/* ============================================================================
 * Benchmark 3: Ackermann function
 * Guage: (define ack (lambda (m n)
 *          (if (= m 0) (+ n 1)
 *              (if (= n 0) (ack (- m 1) 1)
 *                  (ack (- m 1) (ack m (- n 1)))))))
 * ============================================================================ */

static double ack(double m, double n) {
    if (m == 0.0) return n + 1.0;
    if (n == 0.0) return ack(m - 1.0, 1.0);
    return ack(m - 1.0, ack(m, n - 1.0));
}

/* ============================================================================
 * Benchmark 4: Tak function (Takeuchi)
 * Guage: (define tak (lambda (x y z)
 *          (if (>= y x) z
 *              (tak (tak (- x 1) y z)
 *                   (tak (- y 1) z x)
 *                   (tak (- z 1) x y)))))
 * ============================================================================ */

static double tak(double x, double y, double z) {
    if (y >= x) return z;
    return tak(tak(x - 1.0, y, z),
               tak(y - 1.0, z, x),
               tak(z - 1.0, x, y));
}

/* ============================================================================
 * Benchmark 5: Sum to N (simple tail recursion)
 * Guage: (define sum-to (lambda (n acc)
 *          (if (< n 1) acc (sum-to (- n 1) (+ acc n)))))
 * ============================================================================ */

static double sum_to_loop(double n) {
    double acc = 0.0;
    while (n >= 1.0) {
        acc += n;
        n -= 1.0;
    }
    return acc;
}

/* ============================================================================
 * Benchmark Runner
 * ============================================================================ */

typedef struct {
    const char* name;
    double (*fn)(double);
    double (*fn2)(double, double);
    double (*fn3)(double, double, double);
    int arity;
    double default_param;
    double default_param2;
    double default_param3;
} Benchmark;

/* Wrapper for sum_squares_loop to match arity-1 signature */
static double sum_squares_wrapper(double n) {
    return sum_squares_loop(n);
}

static Benchmark benchmarks[] = {
    {"sum-squares", sum_squares_wrapper, NULL, NULL, 1, 50000.0, 0, 0},
    {"sum-to", sum_to_loop, NULL, NULL, 1, 100000.0, 0, 0},
    {"fib", fib_recursive, NULL, NULL, 1, 30.0, 0, 0},
    {"ack", NULL, ack, NULL, 2, 3.0, 9.0, 0},
    {"tak", NULL, NULL, tak, 3, 18.0, 12.0, 6.0},
    {NULL, NULL, NULL, NULL, 0, 0, 0, 0}
};

static void run_benchmark(Benchmark* b, int iterations, double p1, double p2, double p3) {
    uint64_t total_ns = 0;
    uint64_t min_ns = UINT64_MAX;
    uint64_t max_ns = 0;
    double result = 0;

    /* Warmup */
    for (int i = 0; i < 3; i++) {
        if (b->arity == 1 && b->fn) result = b->fn(p1);
        else if (b->arity == 1 && b->fn2) result = b->fn2(p1, 0);
        else if (b->arity == 2) result = b->fn2(p1, p2);
        else if (b->arity == 3) result = b->fn3(p1, p2, p3);
    }

    /* Timed runs */
    for (int i = 0; i < iterations; i++) {
        uint64_t start = now_ns();

        if (b->arity == 1 && b->fn) result = b->fn(p1);
        else if (b->arity == 1 && b->fn2) result = b->fn2(p1, 0);
        else if (b->arity == 2) result = b->fn2(p1, p2);
        else if (b->arity == 3) result = b->fn3(p1, p2, p3);

        uint64_t elapsed = now_ns() - start;
        total_ns += elapsed;
        if (elapsed < min_ns) min_ns = elapsed;
        if (elapsed > max_ns) max_ns = elapsed;
    }

    double mean_us = (double)total_ns / iterations / 1000.0;
    double min_us = (double)min_ns / 1000.0;
    double max_us = (double)max_ns / 1000.0;

    /* JSON output for parsing */
    printf("{\"benchmark\":\"%s\",\"lang\":\"c\",\"iterations\":%d,", b->name, iterations);
    printf("\"param1\":%.0f,\"param2\":%.0f,\"param3\":%.0f,", p1, p2, p3);
    printf("\"mean_us\":%.2f,\"min_us\":%.2f,\"max_us\":%.2f,", mean_us, min_us, max_us);
    printf("\"result\":%.6g}\n", result);
}

static void print_usage(const char* prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -b NAME    Run specific benchmark (default: all)\n");
    fprintf(stderr, "  -n ITER    Number of iterations (default: 10)\n");
    fprintf(stderr, "  -p1 VAL    First parameter\n");
    fprintf(stderr, "  -p2 VAL    Second parameter\n");
    fprintf(stderr, "  -p3 VAL    Third parameter\n");
    fprintf(stderr, "  -l         List available benchmarks\n");
    fprintf(stderr, "\nAvailable benchmarks:\n");
    for (Benchmark* b = benchmarks; b->name; b++) {
        fprintf(stderr, "  %-15s (arity=%d, default=%.0f", b->name, b->arity, b->default_param);
        if (b->arity >= 2) fprintf(stderr, ",%.0f", b->default_param2);
        if (b->arity >= 3) fprintf(stderr, ",%.0f", b->default_param3);
        fprintf(stderr, ")\n");
    }
}

int main(int argc, char** argv) {
    const char* bench_name = NULL;
    int iterations = 10;
    double p1 = -1, p2 = -1, p3 = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            bench_name = argv[++i];
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            iterations = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-p1") == 0 && i + 1 < argc) {
            p1 = atof(argv[++i]);
        } else if (strcmp(argv[i], "-p2") == 0 && i + 1 < argc) {
            p2 = atof(argv[++i]);
        } else if (strcmp(argv[i], "-p3") == 0 && i + 1 < argc) {
            p3 = atof(argv[++i]);
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    for (Benchmark* b = benchmarks; b->name; b++) {
        if (bench_name && strcmp(bench_name, b->name) != 0) continue;

        double param1 = (p1 >= 0) ? p1 : b->default_param;
        double param2 = (p2 >= 0) ? p2 : b->default_param2;
        double param3 = (p3 >= 0) ? p3 : b->default_param3;

        run_benchmark(b, iterations, param1, param2, param3);
    }

    return 0;
}
