#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "objdump.h"

int print_help(char* argv0);
int main(int argc, char** argv) {
    FILE* out = NULL;
    FILE* in = NULL;
    struct OPTIONS opt = {0};
    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];
        if (arg[0] == '-') {
            switch (arg[1]) {
                case 'o': {
                    if (++i < argc) {
                        if (out != NULL) {
                            fprintf(stderr, "Cannot specify multiple output files\n");
                            return 1;
                        }
                        out = fopen(argv[i], "w");
                        if (out == NULL) {
                            fprintf(stderr, "File %s could not be opened for writing\n", argv[i]);
                            fprintf(stderr, "Error: %s\n", strerror(errno));
                            return 1;
                        }
                    } else {
                        fprintf(stderr, "Expected output file after -o\n");
                        return 1;
                    }
                } break;
                case 'h': {
                    opt.print_header = 1;
                } break;
                case 'r': {
                    opt.print_reloc = 1;
                } break;
                case 'd': {
                    opt.print_instr = 1;
                } break;
                case 'D': {
                    opt.print_instr = 1;
                    opt.print_data = 1;
                } break;
                case '-': {
                    if (strlen(arg) == 6 && memcmp(&arg[2], "help", 4) == 0) {
                        return print_help(argv[0]);
                    } else {
                        fprintf(stderr, "Unknown option %s\n", arg);
                        return 1;
                    }
                } break;
                default: {
                    fprintf(stderr, "Unknown option %s\n", arg);
                    return 1;
                }
            }
        } else {
            if (in != NULL) {
                fprintf(stderr, "Cannot specify multiple input files\n");
                return 1;
            }
            in = fopen(arg, "rb");
            if (in == NULL) {
                fprintf(stderr, "File %s could not be opened for reading\n", arg);
                fprintf(stderr, "Error: %s\n", strerror(errno));
                return 1;
            }
        }
    }
    if (in == NULL) {
        in = stdin;
    }
    if (out == NULL) {
        out = stdout;
    }
    objdump_main(in, out, opt);
    if (in != stdin) {
        fclose(in);
    }
    if (out != stdout) {
        fclose(out);
    }
}
int print_help(char* argv0) {
    printf( "Usage %s [-h] [-r] [-d] [-o output_file] input_file\n"
            "   -h - Print header\n"
            "   -r - Print reloc entries\n"
            "   -d - Print instructions\n"
            "   -D - Print instructions and data\n", argv0);
    return 0;
}
