#include "objdump.h"
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <zlib.h>

#ifdef USE_INET_NTOHL
#include <arpa/inet.h>
#endif
#ifdef USE_WINSOCK_NTOHL
#include <Winsock2.h>
#endif
#ifdef USE_CUSTOM_NTOHL

#define ntohl(i) ( \
    ((i) & 0x000000ff) << 24 | \
    ((i) & 0x0000ff00) << 8 | \
    ((i) & 0x00ff0000) >> 8 | \
    ((i) & 0xff000000) >> 24 \
    )

#endif

#define CHUNK  10240
#define RELOC_SIZE 4

#define FLAT_VERSION 0x4l
union bFLT_header {
    uint8_t header[64];
    struct {
        uint8_t bFLT[4];
        uint32_t rev;
        uint32_t entry;
        uint32_t data_start;
        uint32_t data_end;
        uint32_t bss_end;
        uint32_t stack_size;
        uint32_t reloc_start;
        uint32_t reloc_count;
        uint32_t flags;
        uint32_t build_date;
        uint32_t filler[5];
    };
};

int objdump_main(FILE* in, FILE* out, struct OPTIONS opt) {
    union bFLT_header hdr;
    if (fread(hdr.header, 1, 64, in) < 64) {
        return 1;
    }
    if (memcmp(hdr.bFLT, "bFLT", 4) != 0) {
        return 2;
    }
    hdr.rev = ntohl(hdr.rev);
    hdr.entry = ntohl(hdr.entry);
    hdr.data_start = ntohl(hdr.data_start);
    hdr.data_end = ntohl(hdr.data_end);
    hdr.bss_end = ntohl(hdr.bss_end);
    hdr.stack_size = ntohl(hdr.stack_size);
    hdr.reloc_start = ntohl(hdr.reloc_start);
    hdr.reloc_count = ntohl(hdr.reloc_count);
    hdr.flags = ntohl(hdr.flags);
    hdr.build_date = ntohl(hdr.build_date);
    if (hdr.rev != FLAT_VERSION) {
        return 3;
    }
    if (opt.print_header) {
        char timestr[26];
        ctime_r((time_t*) &hdr.build_date, timestr);
        timestr[24] = '\0';
        fprintf(out, "bFLT metadata:\n"
                "  revision:          %d\n"
                "  instruction start: 0x%x\n"
                "  data start:        0x%x\n"
                "  data end:          0x%x\n"
                "  bss end:           0x%x\n"
                "  stack size:        0x%x\n"
                "  reloc start:       0x%x\n"
                "  reloc count:       %d\n"
                "  flags:             0x%x\n"
                "  build date:        0x%d (%s)\n",
                hdr.rev, hdr.entry, hdr.data_start, hdr.data_end, hdr.bss_end,
                hdr.stack_size, hdr.reloc_start, hdr.reloc_count, hdr.flags, hdr.build_date,
                timestr
        );
        fprintf(out, "  flags: ");
        if (hdr.flags & 0x01) {
            fprintf(out, "FLAG_RAM");
            if (hdr.flags & ~0x01) {
                fprintf(out, " | ");
            } else {
                goto headerdone;
            }
        }
        if (hdr.flags & 0x02) {
            fprintf(out, "FLAG_GOTPIC");
            if (hdr.flags & ~0x03) {
                fprintf(out, " | ");
            } else {
                goto headerdone;
            }
        }
        if (hdr.flags & 0x04) {
            fprintf(out, "FLAG_GZIP");
            if (hdr.flags & ~0x07) {
                fprintf(out, " | ");
            } else {
                goto headerdone;
            }
        }
        if (hdr.flags & 0x08) {
            fprintf(out, "FLAG_GZDATA");
            if (hdr.flags & ~0x0f) {
                fprintf(out, " | ");
            } else {
                goto headerdone;
            }
        }
        if (hdr.flags & 0x10) {
            fprintf(out, "FLAG_KTRACE");
            if (hdr.flags & ~0x1f) {
                fprintf(out, " | ");
            } else {
                goto headerdone;
            }
        }
        if (hdr.flags & 0x20) {
            fprintf(out, "FLAG_GOTPIC");
            if (hdr.flags & ~0x3f) {
                fprintf(out, " | ");
            } else {
                goto headerdone;
            }
        }
    }
    headerdone:;
    fprintf(out, "\n");
    int contents_size = hdr.data_end + hdr.reloc_count * RELOC_SIZE;
    char* contents = malloc(contents_size);
    fseek(in, 0, SEEK_SET);
    fread(contents, 1, 64, in);
    if (hdr.flags & 0x04) { // All but header is compressed
        char* gzip_out = &contents[64];
        int gzip_out_size = 0;
        gzFile gf = gzdopen(dup(fileno(in)), "rb");
        int read;
        ftell(in);
        while ((read = gzread(gf, gzip_out + gzip_out_size, CHUNK)) > 0) {
            gzip_out_size += read;
        }
        if (read != 0) {
            fprintf(stderr, "Gzread error %d\n", read);
            return 1;
        }
        if (gzip_out_size < hdr.data_end - hdr.entry + hdr.reloc_count * RELOC_SIZE) {
            fprintf(stderr, "Not enough compressed data %d expected %d\n", gzip_out_size, contents_size);
            return 1;
        }
    } else if (hdr.flags & 0x08) { // GZDATA
        fread(&contents[64], 1, hdr.data_start - hdr.entry, in);
        gzFile gf = gzdopen(dup(fileno(in)), "rb");
        int read;
        int gzip_out_size = 0;
        char* gzip_out = &contents[hdr.data_start];
        while ((read = gzread(gf, gzip_out + gzip_out_size, CHUNK)) > 0) {
            gzip_out_size += read;
        }
        if (read != 0) {
            fprintf(stderr, "Gzread error %s\n", gzerror(gf, NULL));
            return 1;
        }
        if (gzip_out_size < hdr.data_end - hdr.data_start + hdr.reloc_count * RELOC_SIZE) {
            fprintf(stderr, "Not enough compressed data %d expected %d\n",
                gzip_out_size, hdr.data_end - hdr.data_start + hdr.reloc_count * RELOC_SIZE);
            return 1;
        }
    } else {
        int expected_size = hdr.data_end - hdr.entry + hdr.reloc_count * RELOC_SIZE;
        int read = fread(&contents[64], 1, expected_size, in);
        if (read < expected_size) {
            fprintf(stderr, "Not enough data %d expected %d\n", read, expected_size);
            return 1;
        }
    }
    if (opt.print_instr) {
        fprintf(out, ".text\n");
        int i = 0;
        for (; i < hdr.data_start - 64; i += 4) {
            fprintf(out, "0x%4x:  %02hhx %02hhx %02hhx %02hhx (%c%c%c%c)\n", i, contents[i], contents[i + 1], contents[i + 2], contents[i + 3], contents[i], contents[i + 1], contents[i + 2], contents[i + 3]);
        }
        fprintf(out, "0x%4x: ", i);
        for (; i < hdr.data_start - 64; i++) {
            fprintf(out, " %02hhx", contents[i]);
        }
        fprintf(out, "\n");
    }
    if (opt.print_instr) {
        fprintf(out, ".data\n");
        int i = hdr.data_start;
        for (; i < hdr.data_end; i += 4) {
            fprintf(out, "0x%4x:  %02hhx %02hhx %02hhx %02hhx (%c%c%c%c)\n", i, contents[i], contents[i + 1], contents[i + 2], contents[i + 3], contents[i], contents[i + 1], contents[i + 2], contents[i + 3]);
        }
        fprintf(out, "0x%4x: ", i);
        for (; i < hdr.data_end; i++) {
            fprintf(out, " %02hhx", contents[i]);
        }
        fprintf(out, "\n");
    }
    if (opt.print_reloc) {
        fprintf(out, "reloc:\n");
        int i = hdr.reloc_start - 64;
        for (; i < hdr.reloc_start + hdr.reloc_count * RELOC_SIZE; i += RELOC_SIZE) {
            fprintf(out, "0x%4x:  %02hhx %02hhx %02hhx %02hhx (%08x)\n", i, contents[i], contents[i + 1], contents[i + 2], contents[i + 3], ntohl(*((int32_t*)&contents[i])));
        }
        fprintf(out, "\n");
    }
    return 0;
}
