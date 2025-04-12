#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opsoup.h"

int test_image(void);
int test_disasm(void);
int test_elf(void);
int test_label(void);
int test_data(void);

int main(int argc, char **argv) {
    int failures = 0;

    if (argc < 2) {
        printf("Usage: %s [--test-image|--test-disasm|--test-elf|--test-label|--test-data]\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test-image") == 0) {
            failures += test_image();
        } else if (strcmp(argv[i], "--test-disasm") == 0) {
            failures += test_disasm();
        } else if (strcmp(argv[i], "--test-elf") == 0) {
            failures += test_elf();
        } else if (strcmp(argv[i], "--test-label") == 0) {
            failures += test_label();
        } else if (strcmp(argv[i], "--test-data") == 0) {
            failures += test_data();
        }
    }

    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
} 