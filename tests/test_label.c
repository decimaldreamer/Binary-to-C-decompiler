#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opsoup.h"

int test_label(void) {
    int failures = 0;
    opsoup_t ctx = {0};
    o = &ctx;

    printf("Testing label module...\n");

    // Test 1: Label insertion and finding
    segment_t seg = {0};
    label_t *label = label_insert((uint8_t *)0x1000, label_CODE, &seg);
    if (label == NULL) {
        printf("FAIL: Label insertion failed\n");
        failures++;
    } else {
        printf("PASS: Label insertion succeeded\n");
    }

    label_t *found = label_find((uint8_t *)0x1000);
    if (found != label) {
        printf("FAIL: Label finding failed\n");
        failures++;
    } else {
        printf("PASS: Label finding succeeded\n");
    }

    // Test 2: Label removal
    label_remove((uint8_t *)0x1000);
    found = label_find((uint8_t *)0x1000);
    if (found != NULL) {
        printf("FAIL: Label removal failed\n");
        failures++;
    } else {
        printf("PASS: Label removal succeeded\n");
    }

    // Test 3: Label operations
    label = label_insert((uint8_t *)0x2000, label_CODE, &seg);
    label_reloc_upgrade();
    label_gen_names();
    label_sort();
    printf("PASS: Label operations completed\n");

    return failures;
} 