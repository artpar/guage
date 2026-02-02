/* Sum accumulator to 500000 (loop = TCO equivalent) - compiled with gcc -O2 */
#include <stdio.h>

int main(void) {
    double n = 500000.0;
    double acc = 0.0;
    while (n > 0.0) {
        acc += n;
        n -= 1.0;
    }
    printf("%.0f\n", acc);
    return 0;
}
