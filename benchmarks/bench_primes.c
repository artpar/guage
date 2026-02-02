/* Prime counting via trial division up to 10000 - compiled with gcc -O2 */
#include <stdio.h>
#include <math.h>

static int is_prime(double n) {
    for (double i = 2.0; i * i <= n; i += 1.0) {
        if (fmod(n, i) == 0.0) return 0;
    }
    return 1;
}

int main(void) {
    double count = 0.0;
    for (double n = 2.0; n <= 10000.0; n += 1.0) {
        if (is_prime(n)) count += 1.0;
    }
    printf("%.0f\n", count);
    return 0;
}
