/* Ackermann function ack(3,7) - compiled with gcc -O2 */
#include <stdio.h>

static double ack(double m, double n) {
    if (m == 0.0) return n + 1.0;
    if (n == 0.0) return ack(m - 1.0, 1.0);
    return ack(m - 1.0, ack(m, n - 1.0));
}

int main(void) {
    printf("%.0f\n", ack(3.0, 7.0));
    return 0;
}
