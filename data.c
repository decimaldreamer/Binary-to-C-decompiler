#include "opsoup.h"

static void print_label(FILE *f, label_t *label) {
    fprintf(f, "%s:              ; %s %x \n", label->name, label->seg->name, 
            (uint32_t)(label->target - label->seg->start));
}

static void print_data_bytes(FILE *f, uint8_t *mem, uint8_t *end) {
    int dbc = 0, nl = 1, is = 0;
    while (mem < end) {
        if (nl) fprintf(f, "    db ");
        
        if (*mem >= 0x20 && *mem <= 0x7e && *mem != 0x27) {  // Printable ASCII check
            if (nl) fputc(0x27, f);
            else if (!is) fprintf(f, ", '");

            fputc(*mem, f);
            nl = 0;
            is = 1;
        } else {
            if (is) {
                fprintf(f, "', ");
                is = 0;
            } else if (!nl) {
                fprintf(f, ", ");
            }
            fprintf(f, "0x%02x", *mem);
            nl = 0;
            dbc++;

            if (dbc == 8 || (*mem == 0xa || *mem == 0xd)) {
                nl = 1;
            }
        }
        if (nl || mem == end - 1) {
            if (is) fputc(0x27, f);
            fputc('\n', f);
            dbc = 0;
            is = 0;
        }
        mem++;
    }
}

static void write_vector_table(FILE *f, uint8_t *mem, uint8_t *end) {
    while (mem < end) {
        label_t *l = label_find((uint8_t *)*(uint32_t *)mem);
        if (l) {
            fprintf(f, "    dd %s\n", l->name);
        } else {
            fprintf(f, "    db 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", mem[0], mem[1], mem[2], mem[3]);
        }
        mem += 4;
    }
}

void data_output(FILE *f) {
    printf("data: writing data section\n");
    fprintf(f, "\n\nSECTION .data\n");

    for (int i = 0; i < o->nlabel; i++) {
        label_t *label = &o->label[i];
        if (!(label->type & label_DATA)) continue;
        
        fprintf(f, label->type & label_VTABLE ? "\n; vector table\n" : "\n");
        print_label(f, label);

        uint8_t *mem = label->target;
        uint8_t *end = (i == o->nlabel - 1 || label->seg != o->label[i + 1].seg) ? 
                        label->seg->end : o->label[i + 1].target;

        if (label->type & label_VTABLE) {
            write_vector_table(f, mem, end - ((end - mem) % 4));
        } else {
            print_data_bytes(f, mem, end);
        }

        if (o->verbose && i % 100 == 0) {
            printf("  processed %d labels\n", i);
        }
    }
}

void data_bss_output(FILE *f) {
    printf("data: writing bss section\n");
    fprintf(f, "\n\nSECTION .bss\n");

    for (int i = 0; i < o->nlabel; i++) {
        label_t *label = &o->label[i];
        if (!(label->type & label_BSS)) continue;

        fprintf(f, label->type & label_VTABLE ? "\n; vector table\n" : "\n");
        print_label(f, label);

        uint8_t *mem = label->target;
        uint8_t *end = (i == o->nlabel - 1 || label->seg != o->label[i + 1].seg) ? 
                        label->seg->start + label->seg->size : o->label[i + 1].target;
        fprintf(f, "    resb 0x%x\n", end - mem);

        if (o->verbose && i % 100 == 0) {
            printf("  processed %d labels\n", i);
        }
    }
}
