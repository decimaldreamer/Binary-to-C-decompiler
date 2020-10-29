#include "opsoup.h"

static int _ref_find_ll(uint8_t *mem) {
    ref_t *rf = o->ref;
    int nrf = o->nref, abs = 0, i = 1;

    if(rf == NULL)
        return -1;

    while(!(i == 0 && (nrf == 0 || nrf == 1))) {
        i = nrf >> 1;
        if(rf[i].mem == mem)
            return abs + i;
        if(rf[i].mem > mem)
            nrf = i;
        else {
            rf = &(rf[i]);
            nrf -= i;
            abs += i;
        }
    }

    return - (abs + i + 1);
}
ref_t *ref_insert(uint8_t *mem, uint8_t *target) {
    int i, ti;

    i = _ref_find_ll(mem);
    if(i < 0) {
        i = -i - 1;
        for(; i < o->nref && o->ref[i].mem < mem; i++);
        if(o->nref == o->sref) {
            o->sref += 1024;
            o->ref = (ref_t *) realloc(o->ref, sizeof(ref_t) * o->sref);
        }

        if(i < o->nref)
            memmove(&(o->ref[i + 1]), &(o->ref[i]), sizeof(ref_t) * (o->nref - i));
		o->nref++;
        o->ref[i].mem = mem;
        o->ref[i].ntarget = 0;
    }
    for(ti = 0; ti < o->ref[i].ntarget; ti++)
        if(o->ref[i].target[ti] == target)
            return &(o->ref[i]);
    if(o->ref[i].ntarget == MAX_REF_TARGET) {
        if(o->verbose)
            printf("  more than %d targets for label ref %p\n", MAX_REF_TARGET, mem);
        return &(o->ref[i]);
    }
    o->ref[i].target[o->ref[i].ntarget] = target;
    o->ref[i].ntarget++;
    return &(o->ref[i]);
}
