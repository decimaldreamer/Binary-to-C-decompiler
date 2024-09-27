#include "opsoup.h"

int _rm_disp32(uint8_t *mem, uint8_t *reg) {
    uint8_t mod = mem[0] >> 6;
    uint8_t rm = mem[0] & 0x7;

    if (mod == 3)
        return 0;

    if (reg != NULL)
        *reg = (mem[0] & 0x38) >> 3;

    if (mod == 2 || (mod == 0 && rm == 5) || (mod == 0 && rm == 4 && (mem[1] & 0x7) == 5))
        return 1;

    return 0;
}

static label_type_t _vtable_access(uint8_t *mem) {
    uint8_t b = mem[0];
    uint8_t reg;

    // Segment override prefix
    if (b == 0x26 || b == 0x2e || b == 0x36 || b == 0x3e || b == 0x64 || b == 0x65) {
        mem++;
        b = mem[0];
    }

    // jmp r/m32 (0xff /4) or call r/m32 (0xff /2)
    if (b == 0xff && _rm_disp32(mem + 1, &reg)) {
        return (reg == 4) ? label_CODE_JUMP : (reg == 2) ? label_CODE_CALL : 0;
    }

    // mov reg32,r/m32 (0x8b /r)
    if (b == 0x8b && _rm_disp32(mem + 1, NULL)) {
        return label_DATA;
    }

    // movzx reg32,r/m8 (0xf 0xb6 /r)
    if (b == 0xf && mem[1] == 0xb6 && _rm_disp32(mem + 2, NULL)) {
        return label_DATA;
    }

    // mov reg8,r/m8 (0x8a /r) or test r/m32,reg32 (0x85 /r)
    if ((b == 0x8a || b == 0x85) && _rm_disp32(mem + 1, NULL)) {
        return label_DATA;
    }

    return 0;
}

static int _mem_access(uint8_t *mem) {
    uint8_t b = mem[0];
    uint8_t reg;

    // Segment override prefix
    if (b == 0x26 || b == 0x2e || b == 0x36 || b == 0x3e || b == 0x64 || b == 0x65) {
        mem++;
        b = mem[0];
    }

    // mov r/m32,mem32 (0xc7 /0)
    if (b == 0xc7 && _rm_disp32(mem + 1, &reg) && reg == 0) {
        return 1;
    }

    // cmp r/m32,mem32 (0x81 /0)
    if (b == 0x81 && _rm_disp32(mem + 1, &reg) && reg == 7) {
        return 1;
    }

    // push imm32 (0x68) or mov reg32,imm32 (0xb8+r)
    if (b == 0x68 || (b >= 0xb8 && b <= 0xbf)) {
        return 1;
    }

    return 0;
}

static void _target_extract(uint8_t *mem, uint8_t **target, label_type_t *type) {
    uint8_t b1 = mem[0];
    uint8_t b2 = mem[1];

    // Segment override or operand-size override prefix
    if (b1 == 0x26 || b1 == 0x2e || b1 == 0x36 || b1 == 0x3e || b1 == 0x64 || b1 == 0x65 || b1 == 0x66 || b1 == 0x67) {
        mem++;
        b1 = mem[0];
    }

    // Handle jumps and calls
    if (b1 >= 0xe0 && b1 <= 0xe3) {
        *type = label_CODE_JUMP;
        *target = mem + 2 + *(int8_t *)(mem + 1);
    } else if (b1 == 0xe8) {
        *type = label_CODE_CALL;
        *target = mem + 5 + *(int32_t *)(mem + 1);
    } else if (b1 == 0xeb || (b1 >= 0x70 && b1 <= 0x7f)) {
        *type = label_CODE_JUMP;
        *target = mem + 2 + *(int8_t *)(mem + 1);
    } else if (b1 == 0xe9) {
        *type = label_CODE_JUMP;
        *target = mem + 5 + *(int32_t *)(mem + 1);
    } else if (b1 == 0x0f && b2 >= 0x80 && b2 <= 0x8f) {
        *type = label_CODE_JUMP;
        *target = mem + 6 + *(int32_t *)(mem + 2);
    } else {
        *type = label_NONE;
        *target = NULL;
    }
}

void dis_pass1(void) {
    int i, len, ir = 0;
    uint8_t *mem, *target, *vtable;
    label_type_t type, vtype;
    char line[256];
    segment_t *s;

    printf("dis1: disassembly, pass 1 - finding obvious labels\n");

    for (i = 0; o->image.segment[i].name != NULL; i++) {
        if (o->image.segment[i].type != seg_CODE)
            continue;

        printf("dis1: processing segment '%s' (size 0x%x)\n", o->image.segment[i].name, o->image.segment[i].size);
        
        // Find relocations within the current segment
        while (ir < o->nreloc && o->reloc[ir].mem < o->image.segment[i].start)
            ir++;

        mem = o->image.segment[i].start;

        while (mem < o->image.segment[i].start + o->image.segment[i].size) {
            len = disasm(mem, line, sizeof(line), 32, 0, 1, 0);
            if (len == 0)
                len = eatbyte(mem, line, sizeof(line));

            if (ir < o->nreloc && mem + len > o->reloc[ir].mem &&
                o->reloc[ir].mem >= o->image.segment[i].start &&
                o->reloc[ir].mem < o->image.segment[i].end) {

                target = o->reloc[ir].target;
                s = image_seg_find(target);
                
                if (!s) {
                    if (o->verbose)
                        printf("    target %p is not in a segment!\n", target);
                    mem += len;
                    continue;
                }

                type = (s->type == seg_BSS) ? label_BSS : (s->type == seg_DATA) ? label_DATA : label_NONE;
                
                // Handle virtual tables
                if ((vtype = _vtable_access(mem))) {
                    vtable = target;
                    type = (type & label_BSS) ? label_BSS_VTABLE : label_DATA_VTABLE;
                }

                _target_extract(mem, &target, &type);

                // Process labels
                if (target) {
                    label_insert(target, type, s);
                    ref_insert(mem, target);
                }

                if (vtable && o->verbose)
                    printf("    vector table found at %p\n", vtable);

                mem += len;
            }
        }
    }

    label_print_upgraded("dis1");
}

int dis_pass2(int n) {
    label_t *l = malloc(sizeof(label_t) * o->nlabel);
    int i, ir = 0, len;
    uint8_t *mem, *target;
    label_type_t type;
    segment_t *s;

    memcpy(l, o->label, sizeof(label_t) * o->nlabel);
    printf("dis2: disassembly, pass 2, round %d - finding missed labels\n", n);

    for (i = 0; i < o->nlabel; i++) {
        if (!(l[i].type & label_CODE)) 
            continue;

        mem = l[i].target;

        while (mem < l[i].seg->end) {
            len = disasm(mem, line, sizeof(line), 32, 0, 1, 0);
            if (len == 0)
                len = eatbyte(mem, line, sizeof(line));

            _target_extract(mem, &target, &type);
            if (target)
                label_insert(target, type, image_seg_find(target));

            mem += len;
        }
    }

    free(l);
    return label_print_upgraded("dis2");
}
