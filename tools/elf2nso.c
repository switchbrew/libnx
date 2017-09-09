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
    u32 DstOff;
    u32 DecompSz;
    u32 Align;
} NsoSegment;

typedef u8 Sha2Hash[0x20];

typedef struct {
    u8  Magic[4];
    u32 Unk1;
    u32 Unk2;
    u32 Unk3;
    NsoSegment Segments[3];
    u8  BuildId[0x20];
    u32 CompSz[3];
    u8  Padding[0x24];
    u64 Unk4;
    u64 Unk5;
    Sha2Hash Hashes[3];    
} NsoHeader;

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
        printf("%s <elf-file> <nso-file>\n", argv[0]);
        return 1;
    }

    NsoHeader nso_hdr;
    memset(&nso_hdr, 0, sizeof(nso_hdr));
    memcpy(nso_hdr.Magic, "NSO0", 4);
    nso_hdr.Unk3 = 0x3f;

    if (sizeof(NsoHeader) != 0x100) {
        printf("Bad compile environment!\n");
        return 1;
    }

    size_t elf_len;
    uint8_t* elf = ReadEntireFile(argv[1], &elf_len);
    if (elf == NULL) {
        printf("Failed to open input!\n");
        return 1;
    }

    if (elf_len < sizeof(Elf64_Ehdr)) {
        printf("Input file doesn't fit ELF header!\n");
        return 1;
    }

    Elf64_Ehdr* hdr = (Elf64_Ehdr*) elf;
    if (hdr->e_machine != EM_AARCH64) {
        printf("Invalid ELF: expected AArch64!\n");
        return 1;
    }

    Elf64_Off ph_end = hdr->e_phoff + 3 * sizeof(Elf64_Phdr);

    if (ph_end < hdr->e_phoff || ph_end > elf_len) {
        printf("Invalid ELF: phdrs outside file!\n");
        return 1;
    }

    if (hdr->e_phnum != 3) {
        printf("Invalid ELF: expected 3 phdrs!\n");
        return 1;
    }

    Elf64_Phdr* phdr = (Elf64_Phdr*) &elf[hdr->e_phoff];
    size_t i;
    size_t file_off = sizeof(NsoHeader);

    uint8_t* comp_buf[3];
    size_t comp_sz[3];

    for (i=0; i<hdr->e_phnum; i++, phdr++) {
        nso_hdr.Segments[i].FileOff = file_off;
        nso_hdr.Segments[i].DstOff = phdr->p_vaddr;
        nso_hdr.Segments[i].DecompSz = phdr->p_filesz;
        nso_hdr.Segments[i].Align = 1;

        SHA256_CTX ctx;
        sha256_init(&ctx);
        sha256_update(&ctx, &elf[phdr->p_offset], phdr->p_filesz);
        sha256_final(&ctx, (u8*) &nso_hdr.Hashes[i]);

        size_t comp_max = LZ4_compressBound(phdr->p_filesz);
        comp_buf[i] = malloc(comp_max);

        if (comp_buf[i] == NULL) {
            printf("Compressing: Out of memory!\n");
            return 1;
        }

        // TODO check p_offset
        comp_sz[i] = LZ4_compress_default(&elf[phdr->p_offset], comp_buf[i], phdr->p_filesz, comp_max);

        if (comp_sz[i] < 0) {
            printf("Failed to compress!\n");
            return 1;
        }

        nso_hdr.CompSz[i] = comp_sz[i];
        file_off += comp_sz[i];
    }

    FILE* out = fopen(argv[2], "wb");

    if (out == NULL) {
        printf("Failed to open output file!\n");
        return 1;
    }

    // TODO check retvals
    fwrite(&nso_hdr, sizeof(nso_hdr), 1, out);

    for (i=0; i<3; i++)
        fwrite(comp_buf[i], comp_sz[i], 1, out);

    return 0;
}
