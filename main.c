#include "opsoup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define OUTPUT_FILE "ffe.asm"

opsoup_t *o;

int main(int argc, char **argv) {
    opsoup_t ctx = {0};
    int round = 1;
    FILE *f = NULL;

    o = &ctx;

    o->verbose = (argc == 2 && strcmp(argv[1], "-v") == 0);

    if (image_load() != 0) {
        fprintf(stderr, "Error: Image load failed!\n");
        return EXIT_FAILURE;
    }

    init_sync();
    dis_pass1();

    while (dis_pass2(round++)) {
        o->nref = 0;
    }

    label_reloc_upgrade();
    label_gen_names();
    label_sort();

    if ((f = fopen(OUTPUT_FILE, "w")) == NULL) {
        fprintf(stderr, "main: couldn't open '%s' for writing: %s\n", OUTPUT_FILE, strerror(errno));
        return EXIT_FAILURE;
    }

    label_extern_output(f);
    dis_pass3(f);
    data_output(f);
    data_bss_output(f);

    fclose(f);
    return EXIT_SUCCESS;
}
