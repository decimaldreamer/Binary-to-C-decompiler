#include "opsoup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

// Mach-O Header structures
typedef struct {
    uint32_t magic;
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
} mach_header_32;

typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
    char     segname[16];
    uint32_t vmaddr;
    uint32_t vmsize;
    uint32_t fileoff;
    uint32_t filesize;
    uint32_t maxprot;
    uint32_t initprot;
    uint32_t nsects;
    uint32_t flags;
} segment_command_32;

typedef struct {
    char     sectname[16];
    char     segname[16];
    uint32_t addr;
    uint32_t size;
    uint32_t offset;
    uint32_t align;
    uint32_t reloff;
    uint32_t nreloc;
    uint32_t flags;
    uint32_t reserved1;
    uint32_t reserved2;
} section_32;

#define MH_MAGIC 0xfeedface
#define MH_CIGAM 0xcefaedfe

static int macho_validate_header(const uint8_t *data) {
    uint32_t magic = *(uint32_t *)data;
    if (magic != MH_MAGIC && magic != MH_CIGAM) {
        return -1;
    }
    return 0;
}

int macho_load(void) {
    if (macho_validate_header(o->image.core) != 0) {
        fprintf(stderr, "Invalid Mach-O header\n");
        return -1;
    }

    mach_header_32 *header = (mach_header_32 *)o->image.core;
    
    // Initialize segments
    o->image.segment = calloc(header->ncmds, sizeof(segment_t));
    if (o->image.segment == NULL) {
        fprintf(stderr, "Failed to allocate segment table\n");
        return -1;
    }

    return 0;
}

int macho_make_segment_table(image_t *image) {
    mach_header_32 *header = (mach_header_32 *)image->core;
    uint8_t *cmd = (uint8_t *)header + sizeof(mach_header_32);
    
    for (uint32_t i = 0; i < header->ncmds; i++) {
        load_command *lc = (load_command *)cmd;
        
        if (lc->cmd == LC_SEGMENT) {
            segment_command_32 *seg_cmd = (segment_command_32 *)cmd;
            segment_t *seg = &image->segment[i];
            
            seg->name = strndup(seg_cmd->segname, 16);
            seg->start = image->core + seg_cmd->fileoff;
            seg->end = seg->start + seg_cmd->filesize;
            seg->size = seg_cmd->filesize;
            
            // Determine segment type
            if (seg_cmd->initprot & VM_PROT_EXECUTE) {
                seg->type = seg_CODE;
            } else if (seg_cmd->initprot & VM_PROT_READ) {
                seg->type = seg_DATA;
            }
        }
        
        cmd += lc->cmdsize;
    }

    return 0;
}

void macho_load_labels(opsoup_t *o) {
    mach_header_32 *header = (mach_header_32 *)o->image.core;
    uint8_t *cmd = (uint8_t *)header + sizeof(mach_header_32);
    
    for (uint32_t i = 0; i < header->ncmds; i++) {
        load_command *lc = (load_command *)cmd;
        
        if (lc->cmd == LC_SEGMENT) {
            segment_command_32 *seg_cmd = (segment_command_32 *)cmd;
            section_32 *sections = (section_32 *)(cmd + sizeof(segment_command_32));
            
            for (uint32_t j = 0; j < seg_cmd->nsects; j++) {
                label_insert(o->image.core + sections[j].offset, 
                           label_DATA, &o->image.segment[i]);
            }
        }
        
        cmd += lc->cmdsize;
    }
}

int macho_relocate(opsoup_t *o) {
    mach_header_32 *header = (mach_header_32 *)o->image.core;
    uint8_t *cmd = (uint8_t *)header + sizeof(mach_header_32);
    
    for (uint32_t i = 0; i < header->ncmds; i++) {
        load_command *lc = (load_command *)cmd;
        
        if (lc->cmd == LC_SEGMENT) {
            segment_command_32 *seg_cmd = (segment_command_32 *)cmd;
            section_32 *sections = (section_32 *)(cmd + sizeof(segment_command_32));
            
            for (uint32_t j = 0; j < seg_cmd->nsects; j++) {
                if (sections[j].nreloc > 0) {
                    relocation_info *relocs = (relocation_info *)(o->image.core + sections[j].reloff);
                    
                    for (uint32_t k = 0; k < sections[j].nreloc; k++) {
                        uint32_t *addr = (uint32_t *)(o->image.core + sections[j].offset + relocs[k].r_address);
                        label_insert((uint8_t *)addr, label_RELOC, NULL);
                    }
                }
            }
        }
        
        cmd += lc->cmdsize;
    }

    return 0;
} 