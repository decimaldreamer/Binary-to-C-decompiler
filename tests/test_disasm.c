#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opsoup.h"

int test_disasm(void) {
    int failures = 0;
    opsoup_t ctx = {0};
    o = &ctx;

    printf("Testing disassembly module...\n");

    // Test 1: Basic disassembly pass
    dis_pass1();
    printf("PASS: Basic disassembly pass completed\n");

    // Test 2: Multiple analysis passes
    int round = 1;
    while (dis_pass2(round++)) {
        o->nref = 0;
    }
    printf("PASS: Multiple analysis passes completed\n");

    // Test 3: Output generation
    FILE *f = tmpfile();
    if (f == NULL) {
        printf("FAIL: Could not create temporary file\n");
        failures++;
    } else {
        dis_pass3(f);
        printf("PASS: Output generation completed\n");
        fclose(f);
    }

    return failures;
} 