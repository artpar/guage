/* Takeuchi function tak(21,14,7) - compiled with gcc -O2 */
#include <stdio.h>

static double tak(double x, double y, double z) {
    if (x <= y) return z;
    return tak(tak(x - 1.0, y, z),
               tak(y - 1.0, z, x),
               tak(z - 1.0, x, y));
}

int main(void) {
    printf("%.0f\n", tak(21.0, 14.0, 7.0));
    return 0;
}
