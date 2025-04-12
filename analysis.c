#include "opsoup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Type analysis structures
typedef struct {
    uint8_t *start;
    uint8_t *end;
    size_t size;
    char *name;
    bool is_array;
    size_t array_size;
    bool is_pointer;
    bool is_struct;
    bool is_union;
    bool is_enum;
} type_info_t;

typedef struct {
    type_info_t *types;
    size_t count;
    size_t capacity;
} type_analysis_t;

static type_analysis_t analysis = {0};

// Initialize type analysis
void analysis_init(void) {
    analysis.capacity = 1024;
    analysis.types = calloc(analysis.capacity, sizeof(type_info_t));
    if (analysis.types == NULL) {
        fprintf(stderr, "Failed to initialize type analysis\n");
        return;
    }
}

// Add new type
void analysis_add_type(type_info_t type) {
    if (analysis.count >= analysis.capacity) {
        analysis.capacity *= 2;
        type_info_t *new_types = realloc(analysis.types, analysis.capacity * sizeof(type_info_t));
        if (new_types == NULL) {
            fprintf(stderr, "Failed to expand type analysis\n");
            return;
        }
        analysis.types = new_types;
    }

    analysis.types[analysis.count++] = type;
}

// Analyze memory region for types
void analysis_analyze_region(uint8_t *start, uint8_t *end) {
    type_info_t type = {0};
    type.start = start;
    type.end = end;
    type.size = end - start;

    // Basic type detection
    if (type.size == 1) {
        type.name = strdup("char");
    } else if (type.size == 2) {
        type.name = strdup("short");
    } else if (type.size == 4) {
        type.name = strdup("int");
    } else if (type.size == 8) {
        type.name = strdup("long long");
    }

    // Array detection
    if (type.size > 8 && type.size % 4 == 0) {
        type.is_array = true;
        type.array_size = type.size / 4;
        free(type.name);
        type.name = strdup("int[]");
    }

    // Pointer detection
    if (type.size == sizeof(void *)) {
        type.is_pointer = true;
        free(type.name);
        type.name = strdup("void*");
    }

    analysis_add_type(type);
}

// Detect structures
void analysis_detect_structures(opsoup_t *o) {
    for (int i = 0; i < o->nlabel; i++) {
        label_t *label = &o->label[i];
        if (label->type & label_DATA) {
            // Analyze data region for potential structures
            analysis_analyze_region(label->target, label->target + 64); // Sample size
        }
    }
}

// Generate C code for detected types
void analysis_generate_code(FILE *f) {
    fprintf(f, "// Detected types\n\n");
    
    for (size_t i = 0; i < analysis.count; i++) {
        type_info_t *type = &analysis.types[i];
        
        if (type->is_struct) {
            fprintf(f, "typedef struct {\n");
            // TODO: Add struct members
            fprintf(f, "} %s;\n\n", type->name);
        } else if (type->is_union) {
            fprintf(f, "typedef union {\n");
            // TODO: Add union members
            fprintf(f, "} %s;\n\n", type->name);
        } else if (type->is_enum) {
            fprintf(f, "typedef enum {\n");
            // TODO: Add enum values
            fprintf(f, "} %s;\n\n", type->name);
        } else {
            fprintf(f, "typedef %s %s_t;\n\n", type->name, type->name);
        }
    }
}

// Cleanup
void analysis_cleanup(void) {
    for (size_t i = 0; i < analysis.count; i++) {
        free(analysis.types[i].name);
    }
    free(analysis.types);
    analysis.types = NULL;
    analysis.count = 0;
    analysis.capacity = 0;
} 