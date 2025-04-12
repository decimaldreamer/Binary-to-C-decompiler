#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opsoup.h"

int test_elf(void) {
    int failures = 0;
    opsoup_t ctx = {0};
    o = &ctx;

    printf("Testing ELF module...\n");

    // Test 1: Segment table creation
    if (elf_make_segment_table(&o->image) != 0) {
        printf("FAIL: Segment table creation failed\n");
        failures++;
    } else {
        printf("PASS: Segment table creation succeeded\n");
    }

    // Test 2: Label loading
    elf_load_labels(o);
    printf("PASS: Label loading completed\n");

    // Test 3: Relocation
    if (elf_relocate(o) != 0) {
        printf("FAIL: Relocation failed\n");
        failures++;
    } else {
        printf("PASS: Relocation succeeded\n");
    }

    return failures;
} 