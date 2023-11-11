#ifndef OBJDUMP_H
#define OBJDUMP_H
#include <stdio.h>

struct OPTIONS {
    int print_header;
    int print_reloc;
    int print_instr;
    int print_data;
};

int objdump_main(FILE* in, FILE* out, struct OPTIONS opt);

#endif
