#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opsoup.h"

int test_data(void) {
    int failures = 0;
    opsoup_t ctx = {0};
    o = &ctx;

    printf("Testing data module...\n");

    // Test 1: Data output
    FILE *f = tmpfile();
    if (f == NULL) {
        printf("FAIL: Could not create temporary file\n");
        failures++;
    } else {
        data_output(f);
        printf("PASS: Data output completed\n");
        fclose(f);
    }

    // Test 2: BSS output
    f = tmpfile();
    if (f == NULL) {
        printf("FAIL: Could not create temporary file\n");
        failures++;
    } else {
        data_bss_output(f);
        printf("PASS: BSS output completed\n");
        fclose(f);
    }

    return failures;
} 