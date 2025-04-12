#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opsoup.h"

int test_image(void) {
    int failures = 0;
    opsoup_t ctx = {0};
    o = &ctx;

    printf("Testing image module...\n");

    // Test 1: Basic image loading
    if (image_load() != 0) {
        printf("FAIL: Basic image loading failed\n");
        failures++;
    } else {
        printf("PASS: Basic image loading succeeded\n");
    }

    // Test 2: Segment finding
    segment_t *seg = image_seg_find(o->image.core);
    if (seg == NULL) {
        printf("FAIL: Segment finding failed\n");
        failures++;
    } else {
        printf("PASS: Segment finding succeeded\n");
    }

    // Test 3: Invalid memory address
    seg = image_seg_find((uint8_t *)0xFFFFFFFF);
    if (seg != NULL) {
        printf("FAIL: Invalid memory address test failed\n");
        failures++;
    } else {
        printf("PASS: Invalid memory address test succeeded\n");
    }

    return failures;
} 