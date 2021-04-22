#include <stdint.h>
#include <stdio.h>

int8_t test;
#define A 0x01

int main() {
    test = 0;
    printf("0x%X\n", test);
    test |= A;
    printf("0x%X\n", test);
    test &= ~A;
    printf("0x%X\n", test);
    return 0;
}
