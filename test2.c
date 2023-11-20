#include <stdint.h>
#include <stdio.h>

typedef int int32_t;

int32_t parseData(uint32_t input) {
    int32_t result;

    // Check the sign bit (the most significant bit)
    if (input & 0x80000000) {
        // The input is in two's complement form (negative)
        // Calculate the value in two's complement
        printf("negative\n");
        result = (~input) + 1;
    } else {
        // The input is in regular form (positive)
        printf("positive\n");
        result = input;
    }

    return result;
}

int main() {
    uint32_t inputData = 0xffffff9c;  // Example: -1 represented as uint32_t

    int32_t result = parseData(inputData);
    printf("Parsed Value: %d\n", result);  // Should print -1

    return 0;
}
