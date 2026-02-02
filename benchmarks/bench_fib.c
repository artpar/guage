/* Naive recursive Fibonacci(28) - compiled with gcc -O2 */
/* Uses double to match Guage's internal number representation */
#include <stdio.h>

static double fib(double n) {
    if (n < 2.0) return n;
    return fib(n - 1.0) + fib(n - 2.0);
}

int main(void) {
    printf("%.0f\n", fib(28.0));
    return 0;
}
