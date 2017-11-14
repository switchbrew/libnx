// Copyright 2017 plutoo
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <lz4.h>
#include "sha256.h"
#include "elf64.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

typedef struct {
    u32 FileOff;
    u32 Size;
} NsoSegment;

typedef struct {
    u32 unused;
    u32 modOffset;
    u8 Padding[8];
} NroStart;

typedef struct {
    u8  Magic[4];
    u32 Unk1;
    u32 size;
    u32 Unk2;
    NsoSegment Segments[3];
    u32 bssSize;
    u32 Unk3;
    u8  BuildId[0x20];
    u8  Padding[0x20];
} NroHeader;

uint8_t* ReadEntireFile(const char* fn, size_t* len_out) {
    FILE* fd = fopen(fn, "rb");
    if (fd == NULL)
        return NULL;

    fseek(fd, 0, SEEK_END);
    size_t len = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    uint8_t* buf = malloc(len);
    if (buf == NULL) {
        fclose(fd);
        return NULL;
    }

    size_t rc = fread(buf, 1, len, fd);
    if (rc != len) {
        fclose(fd);
        free(buf);
        return NULL;
    }

    *len_out = len;
    return buf;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "%s <elf-file> <nro-file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    NroStart nro_start;
    memset(&nro_start, 0, sizeof(nro_start));

    NroHeader nro_hdr;
    memset(&nro_hdr, 0, sizeof(nro_hdr));
    memcpy(nro_hdr.Magic, "NRO0", 4);

    if (sizeof(NroHeader) != 0x70) {
        fprintf(stderr, "Bad compile environment!\n");
        return EXIT_FAILURE;
    }

    size_t elf_len;
    uint8_t* elf = ReadEntireFile(argv[1], &elf_len);
    if (elf == NULL) {
        fprintf(stderr, "Failed to open input!\n");
        return EXIT_FAILURE;
    }

    if (elf_len < sizeof(Elf64_Ehdr)) {
        fprintf(stderr, "Input file doesn't fit ELF header!\n");
        return EXIT_FAILURE;
    }

    Elf64_Ehdr* hdr = (Elf64_Ehdr*) elf;
    if (hdr->e_machine != EM_AARCH64) {
        fprintf(stderr, "Invalid ELF: expected AArch64!\n");
        return EXIT_FAILURE;
    }

    Elf64_Off ph_end = hdr->e_phoff + hdr->e_phnum * sizeof(Elf64_Phdr);

    if (ph_end < hdr->e_phoff || ph_end > elf_len) {
        fprintf(stderr, "Invalid ELF: phdrs outside file!\n");
        return EXIT_FAILURE;
    }

    Elf64_Phdr* phdrs = (Elf64_Phdr*) &elf[hdr->e_phoff];
    size_t i, j = 0;
    size_t file_off = 0;

    uint8_t* buf[3];

    for (i=0; i<4; i++) {
        Elf64_Phdr* phdr = NULL;
        while (j < hdr->e_phnum) {
            Elf64_Phdr* cur = &phdrs[j++];
            if (cur->p_type == PT_LOAD || i == 3) {
                phdr = cur;
                break;
            }
        }

        if (phdr == NULL) {
            fprintf(stderr, "Invalid ELF: expected 3 loadable phdrs and a bss!\n");
            return EXIT_FAILURE;
        }
        
        // .bss is special
        if (i == 3) {
            nro_hdr.bssSize = (phdr->p_memsz + 0xFFF) & ~0xFFF;
            break;
        }

        nro_hdr.Segments[i].FileOff = phdr->p_vaddr;
        nro_hdr.Segments[i].Size = (phdr->p_memsz + 0xFFF) & ~0xFFF;
        buf[i] = malloc(nro_hdr.Segments[i].Size);
        memset(buf[i], 0, nro_hdr.Segments[i].Size);

        if (buf[i] == NULL) {
            fprintf(stderr, "Out of memory!\n");
            return EXIT_FAILURE;
        }
        
        memcpy(buf[i], &elf[phdr->p_offset], phdr->p_filesz);

        file_off += nro_hdr.Segments[i].Size;
        file_off = (file_off + 0xFFF) & ~0xFFF;
    }

    FILE* out = fopen(argv[2], "wb");

    if (out == NULL) {
        fprintf(stderr, "Failed to open output file!\n");
        return EXIT_FAILURE;
    }
    
    nro_hdr.size = file_off;

    // TODO check retvals

    for (i=0; i<3; i++)
    {
        fseek(out, nro_hdr.Segments[i].FileOff, SEEK_SET);
        fwrite(buf[i], nro_hdr.Segments[i].Size, 1, out);
    }

    fseek(out, 0, SEEK_SET);
    fwrite(&nro_start, sizeof(nro_start), 1, out);
    fwrite(&nro_hdr, sizeof(nro_hdr), 1, out);

    return EXIT_SUCCESS;
}
