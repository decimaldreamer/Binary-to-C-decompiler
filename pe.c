#include "opsoup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

// PE Header structures
typedef struct {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER;

typedef struct {
    uint32_t VirtualAddress;
    uint32_t Size;
} IMAGE_DATA_DIRECTORY;

typedef struct {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;
    uint32_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[16];
} IMAGE_OPTIONAL_HEADER32;

typedef struct {
    char     Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER;

#define IMAGE_DOS_SIGNATURE 0x5A4D
#define IMAGE_NT_SIGNATURE 0x00004550

static int pe_validate_header(const uint8_t *data) {
    if (*(uint16_t *)data != IMAGE_DOS_SIGNATURE) {
        return -1;
    }
    return 0;
}

int pe_load(void) {
    if (pe_validate_header(o->image.core) != 0) {
        fprintf(stderr, "Invalid PE header\n");
        return -1;
    }

    // Parse DOS header
    IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)o->image.core;
    
    // Get NT headers
    IMAGE_NT_HEADERS *nt_headers = (IMAGE_NT_HEADERS *)(o->image.core + dos_header->e_lfanew);
    if (nt_headers->Signature != IMAGE_NT_SIGNATURE) {
        fprintf(stderr, "Invalid NT header signature\n");
        return -1;
    }

    // Initialize segments
    o->image.segment = calloc(nt_headers->FileHeader.NumberOfSections, sizeof(segment_t));
    if (o->image.segment == NULL) {
        fprintf(stderr, "Failed to allocate segment table\n");
        return -1;
    }

    return 0;
}

int pe_make_section_table(image_t *image) {
    IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)image->core;
    IMAGE_NT_HEADERS *nt_headers = (IMAGE_NT_HEADERS *)(image->core + dos_header->e_lfanew);
    IMAGE_SECTION_HEADER *sections = (IMAGE_SECTION_HEADER *)((uint8_t *)nt_headers + 
        sizeof(IMAGE_NT_HEADERS));

    for (int i = 0; i < nt_headers->FileHeader.NumberOfSections; i++) {
        segment_t *seg = &image->segment[i];
        seg->name = strndup(sections[i].Name, 8);
        seg->start = image->core + sections[i].PointerToRawData;
        seg->end = seg->start + sections[i].SizeOfRawData;
        seg->size = sections[i].SizeOfRawData;

        // Determine segment type
        if (sections[i].Characteristics & IMAGE_SCN_CNT_CODE) {
            seg->type = seg_CODE;
        } else if (sections[i].Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
            seg->type = seg_DATA;
        } else if (sections[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            seg->type = seg_BSS;
        }
    }

    return 0;
}

void pe_load_labels(opsoup_t *o) {
    IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)o->image.core;
    IMAGE_NT_HEADERS *nt_headers = (IMAGE_NT_HEADERS *)(o->image.core + dos_header->e_lfanew);
    IMAGE_SECTION_HEADER *sections = (IMAGE_SECTION_HEADER *)((uint8_t *)nt_headers + 
        sizeof(IMAGE_NT_HEADERS));

    // Add entry point label
    label_insert(o->image.core + nt_headers->OptionalHeader.AddressOfEntryPoint, 
                label_CODE, &o->image.segment[0]);

    // Add section labels
    for (int i = 0; i < nt_headers->FileHeader.NumberOfSections; i++) {
        label_insert(sections[i].PointerToRawData + o->image.core, 
                    label_DATA, &o->image.segment[i]);
    }
}

int pe_relocate(opsoup_t *o) {
    IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)o->image.core;
    IMAGE_NT_HEADERS *nt_headers = (IMAGE_NT_HEADERS *)(o->image.core + dos_header->e_lfanew);
    
    // Process relocation table if exists
    if (nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > 0) {
        IMAGE_BASE_RELOCATION *reloc = (IMAGE_BASE_RELOCATION *)(o->image.core + 
            nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
        
        while (reloc->SizeOfBlock > 0) {
            uint16_t *entries = (uint16_t *)((uint8_t *)reloc + sizeof(IMAGE_BASE_RELOCATION));
            int count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t);
            
            for (int i = 0; i < count; i++) {
                if (entries[i] & 0xF000) { // Valid relocation
                    uint32_t *addr = (uint32_t *)(o->image.core + reloc->VirtualAddress + (entries[i] & 0xFFF));
                    label_insert((uint8_t *)addr, label_RELOC, NULL);
                }
            }
            
            reloc = (IMAGE_BASE_RELOCATION *)((uint8_t *)reloc + reloc->SizeOfBlock);
        }
    }

    return 0;
} 