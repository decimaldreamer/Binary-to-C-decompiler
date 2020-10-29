#include "opsoup.h"
int _rm_disp32(uint8_t *mem, uint8_t *reg) {
    uint8_t mod, rm;

    mod = mem[0] >> 6;
    if(mod == 3)
        return 0;
    rm = mem[0] & 0x7;
    if(reg != NULL)
        *reg = (mem[0] & 0x38) >> 3;
    if(mod == 2 || (mod == 0 && rm == 5) || (mod == 0 && rm == 4 && (mem[1] & 0x7) == 5))
        return 1;

    return 0;
}
static label_type_t _vtable_access(uint8_t *mem) {
    uint8_t b, reg;
    b = mem[0];
    if(b == 0x26 || b == 0x2e || b == 0x36 || b == 0x3e || b == 0x64 || b == 0x65) {
        mem++;
        b = mem[0];
    }
    /*
     *  jmp r/m32 == 0xff /4
     * call r/m32 == 0xff /2
     */
    if(b == 0xff && _rm_disp32(mem+1, &reg)) {
        if(reg == 4)
            return label_CODE_JUMP;
        if(reg == 2)
            return label_CODE_CALL;
    }

    /*
     * mov reg32,r/m32 == 0x8b /r
     */
    if(b == 0x8b && _rm_disp32(mem+1, NULL))
        return label_DATA;

    /*
     * movzx reg32,r/m8 == 0xf 0xb6 /r
     */
    if(b == 0xf && mem[1] == 0xb6 && _rm_disp32(mem+2, NULL))
        return label_DATA;

    /*
     * mov reg8,r/m8 == 0x8a /r
     * test r/m32,reg32 == 0x85 /r
     */
    if((b == 0x8a || b == 0x85) && _rm_disp32(mem+1, NULL))
        return label_DATA;

    return 0;
}
static int _mem_access(uint8_t *mem) {
    uint8_t b, reg;
    b = mem[0];
    if(b == 0x26 || b == 0x2e || b == 0x36 || b == 0x3e || b == 0x64 || b == 0x65) {
        mem++;
        b = mem[0];
    }

    /*
     * mov r/m32,mem32 == 0xc7 /0
     */
    if(b == 0xc7) {
        _rm_disp32(mem+1, &reg);
        if(reg == 0)
            return 1;
    }

    /*
     * cmp r/m32,mem32 == 0x81 /0
     */
    if(b == 0x81) {
        _rm_disp32(mem+1, &reg);
        if(reg == 7)
            return 1;
    }

    /*
     * push imm32       == 0x68
     *  mov reg32,imm32 == 0xb8+r
     */
    if(b == 0x68 || (b >= 0xb8 && b <= 0xbf))
        return 1;

    return 0;
}

static void _target_extract(uint8_t *mem, uint8_t **target, label_type_t *type) {
    uint8_t b1, b2;
    b1 = mem[0];
    if(b1 == 0x26 || b1 == 0x2e || b1 == 0x36 || b1 == 0x3e || b1 == 0x64 || b1 == 0x65 || b1 == 0x66 || b1 == 0x67) {
        mem++;
        b1 = mem[0];
    }

    b2 = mem[1];

    /*   e0-e2: loop, loope, loopne
     *      e3: jexz, jecxz
     */
    if(b1 >= 0xe0 && b1 <= 0xe3) {
        *type = label_CODE_JUMP;
        *target = mem + 2 +  * (int8_t *) (mem+1);
    }
    else if(b1 == 0xe8) {
        *type = label_CODE_CALL;
        *target = mem + 5 + * (int32_t *) (mem+1);
    }
    else if(b1 == 0xeb) {
        *type = label_CODE_JUMP;
        *target = mem + 2 +  * (int8_t *) (mem+1);
    }
    else if(b1 == 0xe9) {
        *type = label_CODE_JUMP;
        *target = mem + 5 + * (int32_t *) (mem+1);
    }
    else if(b1 >= 0x70 && b1 <= 0x7f) {
        *type = label_CODE_JUMP;
        *target = mem + 2 +  * (int8_t *) (mem+1);
    }
    else if(b1 == 0x0f && b2 >= 0x80 && b2 <= 0x8f) {
        *type = label_CODE_JUMP;
        *target = mem + 6 + * (int32_t *) (mem+2);
    }

    else {
        *type = label_NONE;
        *target = NULL;
    }
}

void dis_pass1(void) {
    int i, len, ir = 0, vir;
    uint8_t *mem, *target, *vtable, *vmem;
    label_type_t type, vtype;
    char line[256];
    segment_t *s;

    printf("dis1: disassembly, pass 1 - finding obvious labels\n");

    for(i = 0; o->image.segment[i].name != NULL; i++) {
        if(o->image.segment[i].type != seg_CODE) continue;
        printf("dis1: processing segment '%s' (size 0x%x)\n", o->image.segment[i].name, o->image.segment[i].size);
        for(; ir < o->nreloc; ir++)
            if(o->reloc[ir].mem >= o->image.segment[i].start)
                break;
        mem = o->image.segment[i].start;
        while(mem < o->image.segment[i].start + o->image.segment[i].size) {
            len = disasm(mem, line, sizeof(line), 32, 0, 1, 0);
            if(len == 0)
                len = eatbyte(mem, line, sizeof(line));
            if(ir < o->nreloc && mem + len > o->reloc[ir].mem &&
               o->reloc[ir].mem >= o->image.segment[i].start &&
               o->reloc[ir].mem < o->image.segment[i].end) {

                target = o->reloc[ir].target;
                ir++;
                s = image_seg_find(target);
                if(s == NULL) {
                    if(o->verbose) {
                        printf("  %x: %s\n", mem - o->image.segment[i].start, line);
                        printf("    target %p (reloc at %p) is not in a segment!\n", target, o->reloc[ir].mem);
                    }
                    mem += len;
                    continue;
                }

                type = label_NONE;
                if(s->type == seg_BSS)
                    type = label_BSS;
                else if(s->type == seg_DATA)
                    type = label_DATA;
                if((vtype = _vtable_access(mem))) {
                    vtable = target;
                    if(type & label_BSS)
                        type = label_BSS_VTABLE;
                    else
                        type = label_DATA_VTABLE;
                }

                else if (type == label_NONE)
                    type = label_DATA;
            }
            else
                _target_extract(mem, &target, &type);

            if (target == NULL) {
                mem += len;
                continue;
            }

            if (o->verbose)
                printf("  %x: %s\n", mem - o->image.segment[i].start, line);
            s = image_seg_find(target);
            if(s == NULL) {
                if(o->verbose)
                    printf("    target %p (mem %p) is not in a segment!\n", target, mem);
            }

            else {
                while(type != label_NONE) {
                    label_insert(target, type, s);
                    ref_insert(mem, target);
                    type = label_NONE;
                    if(ir < o->nreloc && mem + len > o->reloc[ir].mem &&
                       o->reloc[ir].mem >= o->image.segment[i].start &&
                       o->reloc[ir].mem < o->image.segment[i].end) {
                        target = o->reloc[ir].target;
                        ir++;
                        s = image_seg_find(target);
                            
                        if(s != NULL) {
                            if(s->type == seg_BSS)
                                type = label_BSS;
                            else
                                type = label_DATA;
                        }

                        else if(o->verbose)
                            printf("    target %p (reloc at %p) is not in a segment!\n", target, o->reloc[ir].mem);
                    }
                }
            }
            if(vtable != NULL) {
                if(o->verbose)
                    printf("    vector table found at %p\n", vtable);

                for (vir = 0; vir < o->nreloc; vir++)
                    if (o->reloc[vir].mem == vtable)
                        break;

                if (vir == o->nreloc && o->verbose) 
                    printf("    couldn't find first relocation for vector table!\n");

                else {
                    vmem = vtable - 4;
                    while(o->reloc[vir].mem == vmem + 4) {
                        s = image_seg_find(o->reloc[vir].target);

                        if(s == NULL || s->type == seg_BSS)
                            type = label_BSS;

                        else if(vtype & label_CODE && s->type == seg_DATA)
                            type = label_DATA;

                        else
                            type = vtype;
                        label_insert(o->reloc[vir].target, type, s);

                        ref_insert(o->reloc[ir].mem, o->reloc[ir].target);
                        vir++;
                        vmem += 4;
                    }
                }

                vtable = NULL;
            }
            mem += len;
            if(o->verbose)
                if(((mem - o->image.segment[i].start) & 0xfffff000) != ((mem - o->image.segment[i].start + len) & 0xfffff000))
                    printf("  processed 0x%x bytes\n", mem + len - o->image.segment[i].start);
        }
    }

    label_print_upgraded("dis1");
}

int dis_pass2(int n) {
    label_t *l;
    int nl, i, ir = 0, vir, len;
    uint8_t *mem, *target, *vtable = NULL, *vmem;
    char line[256];
    label_type_t type, vtype;
    segment_t *s;
    printf("dis2: disassembly, pass 2, round %d - finding missed labels\n", n);
    n = 0;
    nl = o->nlabel;
    l = (label_t *) malloc(sizeof(label_t) * nl);
    memcpy(l, o->label, sizeof(label_t) * nl);

    for(i = 0; i < nl; i++) {
        if(!(l[i].type & label_CODE)) continue;
        mem = l[i].target;
        for(; ir < o->nreloc; ir++)
            if(o->reloc[ir].mem >= mem)
                break;
        while(1) {
            len = disasm(mem, line, sizeof(line), 32, 0, 1, 0);
            if(len == 0)
                len = eatbyte(mem, line, sizeof(line));
            if(mem + len > l[i].seg->end || (i < nl - 1 && mem + len > l[i + 1].target))
                break;
            if(ir < o->nreloc && mem + len > o->reloc[ir].mem &&
               o->reloc[ir].mem >= l[i].seg->start &&
               o->reloc[ir].mem < l[i].seg->end) {
                target = o->reloc[ir].target;
                ir++;
                s = image_seg_find(target);
                if(s == NULL) {
                    if(o->verbose) {
                        printf("  %x: %s\n", mem - l[i].seg->start, line);
                        printf("    target %p (reloc at %p) is not in a segment!\n", target, o->reloc[ir].mem);
                    }
                    mem += len;
                    continue;
                }

                type = label_NONE;
                if(s->type == seg_BSS)
                    type = label_BSS;
                else if(s->type == seg_DATA)
                    type = label_DATA;
                if((vtype = _vtable_access(mem))) {
                    vtable = target;
                    if(type & label_BSS)
                        type = label_BSS_VTABLE;
                    else
                        type = label_DATA_VTABLE;
                }
                else if (type == label_NONE)
                    type = label_DATA;
            }
            else
                _target_extract(mem, &target, &type);

            if (target == NULL) {
                mem += len;
                continue;
            }
            if (o->verbose)
                printf("  %x: %s\n", mem - l[i].seg->start, line);
            s = image_seg_find(target);
            if(s == NULL) {
                if(o->verbose)
                    printf("    target %p (offset %p) is not in a segment!\n", target, mem);
            }

            else {
                while(type != label_NONE) {
                    label_insert(target, type, s);
                    ref_insert(mem, target);
                    type = label_NONE;
                    if(ir < o->nreloc && mem + len > o->reloc[ir].mem &&
                       o->reloc[ir].mem >= l[i].seg->start &&
                       o->reloc[ir].mem < l[i].seg->end) {
                        target = o->reloc[ir].target;
                        ir++;
                        s = image_seg_find(target);
                            
                        if(s != NULL) {
                            if(s->type == seg_BSS)
                                type = label_BSS;
                            else
                                type = label_DATA;
                        }

                        else if(o->verbose)
                            printf("    target %p (reloc at %p) is not in a segment!\n", target, o->reloc[ir].mem);
                    }
                }
            }
            if(vtable != NULL) {
                if(o->verbose)
                    printf("    vector table found at %p\n", vtable);

                for (vir = 0; vir < o->nreloc; vir++)
                    if (o->reloc[vir].mem == vtable)
                        break;

                if (vir == o->nreloc && o->verbose) 
                    printf("    couldn't find first relocation for vector table!\n");

                else {
                    vmem = vtable - 4;
                    while(o->reloc[vir].mem == vmem + 4) {
                        s = image_seg_find(o->reloc[vir].target);

                        if(s == NULL || s->type == seg_BSS)
                            type = label_BSS;

                        else if(vtype & label_CODE && s->type == seg_DATA)
                            type = label_DATA;

                        else
                            type = vtype;
                        label_insert(o->reloc[vir].target, type, s);
                        ref_insert(o->reloc[ir].mem, o->reloc[ir].target);
                        vir++;
                        vmem += 4;
                    }
                }

                vtable = NULL;
            }
            mem += len;
        }
        if(o->verbose)
            if(i % 100 == 0)
                printf("  processed %d labels\n", i);
    }

    free(l);

    return label_print_upgraded("dis2");
}

void dis_pass3(FILE *f) {
    int i, j = 0, len, ti;
    uint8_t *mem;
    char line[256], line2[256], *pos, *num, *rest;
    label_t *l;
    printf("dis3: disassembly, pass 3 - full disassembly and output\n");
    fprintf(f, "\nSECTION .text\n");

    for(i = 0; i < o->nlabel; i++) {
        if(!(o->label[i].type & label_CODE)) continue;
        fprintf(f, "\n\n%s:                     ; %s %x \n\n", o->label[i].name, o->label[i].seg->name, (uint32_t) (o->label[i].target - o->label[i].seg->start));

        mem = o->label[i].target;

        while(1) {
            for(; j < o->nref; j++)
                if(o->ref[j].mem >= mem)
                    break;
            len = disasm(mem, line, sizeof(line), 32, 0, 1, 0);
            if(len == 0)
                len = eatbyte(mem, line, sizeof(line));
            if(mem + len > o->label[i].seg->end || (i < o->nlabel - 1 && mem + len > o->label[i + 1].target))
                break;
            pos = line;
            while((num = strstr(pos, "0x")) != NULL) {
                uint8_t *target;

                errno = 0;
                if (num[-1] == '-')
                    target = (uint8_t *) -strtoul(num, &rest, 16);
                else
                    target = (uint8_t *) strtoul(num, &rest, 16);
                if (errno) {
                    fprintf(stderr, "dis3: can't convert string starting '%s' to pointer: %s\n", num, strerror(errno));
                    abort();
                }

                l = label_find(target);
                if(l == NULL) {
                    label_type_t type;

                    _target_extract(mem, &target, &type);

                    if (type != label_NONE)
                        l = label_find(target);

                    if (l == NULL) {
                        pos = rest;
                        continue;
                    }
                }

                if (num[-1] == '-')
                    num[-1] = '+';

                for(ti = 0; ti < o->ref[j].ntarget; ti++)
                    if(l->target == o->ref[j].target[ti])
                        break;

                if(ti == o->ref[j].ntarget) {
                    pos = rest;
                    continue;
                }

                l->count++;

                *num = '\0';
                sprintf(line2, "%s%s", line, l->name);
                pos = strchr(line2, '\0');
                sprintf(pos, "%s", rest);

                strcpy(line, line2);

                pos = line + (pos - line2);
            }
            fprintf(f, "    %s\n", line);

            mem += len;
        }
        if(o->verbose)
            if(i % 100 == 0)
                printf("  processed %d labels\n", i);
    }
}
