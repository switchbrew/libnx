#include <string.h>
#include <stdlib.h>
#include <arm_neon.h>

#include "result.h"
#include "crypto/aes.h"

/* Helper macros to setup for inline AES asm */
#define AES_ENC_DEC_SETUP_VARS() \
uint8x16_t tmp = vld1q_u8((const uint8_t *)src); \
uint8x16_t tmp2

#define AES_ENC_DEC_OUTPUT_VARS() \
[tmp]"+w"(tmp), [tmp2]"=w"(tmp2)

#define AES_ENC_DEC_STORE_RESULT() \
vst1q_u8((uint8_t *)dst, tmp)

/* Helper macros to do AES encryption, via inline asm. */
#define AES_ENC_ROUND(n) \
"ldr %q[tmp2], %[round_key_" #n "]\n" \
"aese %[tmp].16b, %[tmp2].16b\n" \
"aesmc %[tmp].16b, %[tmp].16b\n"

#define AES_ENC_FINAL_ROUND() \
"ldr %q[tmp2], %[round_key_second_last]\n" \
"aese %[tmp].16b, %[tmp2].16b\n" \
"ldr %q[tmp2], %[round_key_last]\n" \
"eor %[tmp].16b, %[tmp].16b, %[tmp2].16b"

#define AES_ENC_INPUT_ROUND_KEY(num_rounds, n) \
[round_key_##n]"m"(ctx->round_keys[(n-1)])

#define AES_ENC_INPUT_LAST_ROUND_KEYS(num_rounds) \
[round_key_second_last]"m"(ctx->round_keys[(num_rounds - 1)]), \
[round_key_last]"m"(ctx->round_keys[(num_rounds)])

/* Helper macros to do AES decryption, via inline asm. */
#define AES_DEC_ROUND(n) \
"ldr %q[tmp2], %[round_key_" #n "]\n" \
"aesd %[tmp].16b, %[tmp2].16b\n" \
"aesimc %[tmp].16b, %[tmp].16b\n"

#define AES_DEC_FINAL_ROUND() \
"ldr %q[tmp2], %[round_key_second_last]\n" \
"aesd %[tmp].16b, %[tmp2].16b\n" \
"ldr %q[tmp2], %[round_key_last]\n" \
"eor %[tmp].16b, %[tmp].16b, %[tmp2].16b"

#define AES_DEC_INPUT_ROUND_KEY(num_rounds, n) \
[round_key_##n]"m"(ctx->round_keys[(num_rounds + 1 - n)])

#define AES_DEC_INPUT_LAST_ROUND_KEYS(num_rounds) \
[round_key_second_last]"m"(ctx->round_keys[1]), \
[round_key_last]"m"(ctx->round_keys[0])

/* Lookup tables for key scheduling. */
static const u8 s_subBytesTable[0x100] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const u8 s_rconTable[16] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f
};

static inline u32 _subBytes(u32 tmp) {
    return (s_subBytesTable[(tmp >> 0x00) & 0xFF] << 0x00) |
           (s_subBytesTable[(tmp >> 0x08) & 0xFF] << 0x08) |
           (s_subBytesTable[(tmp >> 0x10) & 0xFF] << 0x10) |
           (s_subBytesTable[(tmp >> 0x18) & 0xFF] << 0x18);
}

static inline u32 _rotateBytes(u32 tmp) {
    return (((tmp >> 0x00) & 0xFF) << 0x18) |
           (((tmp >> 0x08) & 0xFF) << 0x00) |
           (((tmp >> 0x10) & 0xFF) << 0x08) |
           (((tmp >> 0x18) & 0xFF) << 0x10);
}

void aes128ContextCreate(Aes128Context *out, const void *key, bool is_encryptor) {
    u32 *round_keys_u32 = (u32 *)out->round_keys;

    /* Copy in first round key. */
    memcpy(round_keys_u32, key, AES_128_KEY_SIZE);

    u32 tmp = round_keys_u32[AES_128_U32_PER_KEY - 1];

    /* Do AES key scheduling. */
    for (size_t i = AES_128_U32_PER_KEY; i < sizeof(out->round_keys) / sizeof(u32); i++) {
        /* First word of key needs special handling. */
        if (i % AES_128_U32_PER_KEY == 0) {
            tmp = _rotateBytes(_subBytes(tmp)) ^ (u32)s_rconTable[(i / AES_128_U32_PER_KEY) - 1];
        }

        tmp ^= round_keys_u32[i - AES_128_U32_PER_KEY];
        round_keys_u32[i] = tmp;
    }

    /* If decryption, calculate inverse mix columns on round keys ahead of time to speed up decryption. */
    if (!is_encryptor) {
        for (size_t i = 1; i < AES_128_NUM_ROUNDS; i++) {
            uint8x16_t tmp_key = vld1q_u8(out->round_keys[i]);
            tmp_key = vaesimcq_u8(tmp_key);
            vst1q_u8(out->round_keys[i], tmp_key);
        }
    }
}

void aes192ContextCreate(Aes192Context *out, const void *key, bool is_encryptor) {
    u32 *round_keys_u32 = (u32 *)out->round_keys;

    /* Copy in first round key. */
    memcpy(round_keys_u32, key, AES_192_KEY_SIZE);

    u32 tmp = round_keys_u32[AES_192_U32_PER_KEY - 1];

    /* Do AES key scheduling. */
    for (size_t i = AES_192_U32_PER_KEY; i < sizeof(out->round_keys) / sizeof(u32); i++) {
        /* First word of key needs special handling. */
        if (i % AES_192_U32_PER_KEY == 0) {
            tmp = _rotateBytes(_subBytes(tmp)) ^ (u32)s_rconTable[(i / AES_192_U32_PER_KEY) - 1];
        }

        tmp ^= round_keys_u32[i - AES_192_U32_PER_KEY];
        round_keys_u32[i] = tmp;
    }

    /* If decryption, calculate inverse mix columns on round keys ahead of time to speed up decryption. */
    if (!is_encryptor) {
        for (size_t i = 1; i < AES_192_NUM_ROUNDS; i++) {
            uint8x16_t tmp_key = vld1q_u8(out->round_keys[i]);
            tmp_key = vaesimcq_u8(tmp_key);
            vst1q_u8(out->round_keys[i], tmp_key);
        }
    }
}

void aes256ContextCreate(Aes256Context *out, const void *key, bool is_encryptor) {
    u32 *round_keys_u32 = (u32 *)out->round_keys;

    /* Copy in first round key. */
    memcpy(round_keys_u32, key, AES_256_KEY_SIZE);

    u32 tmp = round_keys_u32[AES_256_U32_PER_KEY - 1];

    /* Do AES key scheduling. */
    for (size_t i = AES_256_U32_PER_KEY; i < sizeof(out->round_keys) / sizeof(u32); i++) {
        /* First word of key needs special handling. */
        if (i % AES_256_U32_PER_KEY == 0) {
            tmp = _rotateBytes(_subBytes(tmp)) ^ (u32)s_rconTable[(i / AES_256_U32_PER_KEY) - 1];
        } else if (i % AES_256_U32_PER_KEY == (AES_256_U32_PER_KEY / 2)) {
            /* AES-256 does sub bytes on first word of second key */
            tmp = _subBytes(tmp);
        }

        tmp ^= round_keys_u32[i - AES_256_U32_PER_KEY];
        round_keys_u32[i] = tmp;
    }

    /* If decryption, calculate inverse mix columns on round keys ahead of time to speed up decryption. */
    if (!is_encryptor) {
        for (size_t i = 1; i < AES_256_NUM_ROUNDS; i++) {
            uint8x16_t tmp_key = vld1q_u8(out->round_keys[i]);
            tmp_key = vaesimcq_u8(tmp_key);
            vst1q_u8(out->round_keys[i], tmp_key);
        }
    }
}

void aes128EncryptBlock(const Aes128Context *ctx, void *dst, const void *src) {
    /* Setup for asm */
    AES_ENC_DEC_SETUP_VARS();

    /* Use optimized assembly to do all rounds. */
    __asm__ __volatile__ (
        AES_ENC_ROUND(1)
        AES_ENC_ROUND(2)
        AES_ENC_ROUND(3)
        AES_ENC_ROUND(4)
        AES_ENC_ROUND(5)
        AES_ENC_ROUND(6)
        AES_ENC_ROUND(7)
        AES_ENC_ROUND(8)
        AES_ENC_ROUND(9)
        AES_ENC_FINAL_ROUND()
        : AES_ENC_DEC_OUTPUT_VARS()
        : AES_ENC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 1),
          AES_ENC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 2),
          AES_ENC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 3),
          AES_ENC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 4),
          AES_ENC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 5),
          AES_ENC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 6),
          AES_ENC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 7),
          AES_ENC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 8),
          AES_ENC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 9),
          AES_ENC_INPUT_LAST_ROUND_KEYS(AES_128_NUM_ROUNDS)
    );

    /* Store result. */
    AES_ENC_DEC_STORE_RESULT();
}

void aes192EncryptBlock(const Aes192Context *ctx, void *dst, const void *src) {
    /* Setup for asm */
    AES_ENC_DEC_SETUP_VARS();

    /* Use optimized assembly to do all rounds. */
    __asm__ __volatile__ (
        AES_ENC_ROUND(1)
        AES_ENC_ROUND(2)
        AES_ENC_ROUND(3)
        AES_ENC_ROUND(4)
        AES_ENC_ROUND(5)
        AES_ENC_ROUND(6)
        AES_ENC_ROUND(7)
        AES_ENC_ROUND(8)
        AES_ENC_ROUND(9)
        AES_ENC_ROUND(10)
        AES_ENC_ROUND(11)
        AES_ENC_FINAL_ROUND()
        : AES_ENC_DEC_OUTPUT_VARS()
        : AES_ENC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 1),
          AES_ENC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 2),
          AES_ENC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 3),
          AES_ENC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 4),
          AES_ENC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 5),
          AES_ENC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 6),
          AES_ENC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 7),
          AES_ENC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 8),
          AES_ENC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 9),
          AES_ENC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 10),
          AES_ENC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 11),
          AES_ENC_INPUT_LAST_ROUND_KEYS(AES_192_NUM_ROUNDS)
    );

    /* Store result. */
    AES_ENC_DEC_STORE_RESULT();
}

void aes256EncryptBlock(const Aes256Context *ctx, void *dst, const void *src) {
    /* Setup for asm */
    AES_ENC_DEC_SETUP_VARS();

    /* Use optimized assembly to do all rounds. */
    __asm__ __volatile__ (
        AES_ENC_ROUND(1)
        AES_ENC_ROUND(2)
        AES_ENC_ROUND(3)
        AES_ENC_ROUND(4)
        AES_ENC_ROUND(5)
        AES_ENC_ROUND(6)
        AES_ENC_ROUND(7)
        AES_ENC_ROUND(8)
        AES_ENC_ROUND(9)
        AES_ENC_ROUND(10)
        AES_ENC_ROUND(11)
        AES_ENC_ROUND(12)
        AES_ENC_ROUND(13)
        AES_ENC_FINAL_ROUND()
        : AES_ENC_DEC_OUTPUT_VARS()
        : AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 1),
          AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 2),
          AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 3),
          AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 4),
          AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 5),
          AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 6),
          AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 7),
          AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 8),
          AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 9),
          AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 10),
          AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 11),
          AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 12),
          AES_ENC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 13),
          AES_ENC_INPUT_LAST_ROUND_KEYS(AES_256_NUM_ROUNDS)
    );

    /* Store result. */
    AES_ENC_DEC_STORE_RESULT();
}

void aes128DecryptBlock(const Aes128Context *ctx, void *dst, const void *src) {
    /* Setup for asm */
    AES_ENC_DEC_SETUP_VARS();

    /* Use optimized assembly to do all rounds. */
    __asm__ __volatile__ (
        AES_DEC_ROUND(1)
        AES_DEC_ROUND(2)
        AES_DEC_ROUND(3)
        AES_DEC_ROUND(4)
        AES_DEC_ROUND(5)
        AES_DEC_ROUND(6)
        AES_DEC_ROUND(7)
        AES_DEC_ROUND(8)
        AES_DEC_ROUND(9)
        AES_DEC_FINAL_ROUND()
        : AES_ENC_DEC_OUTPUT_VARS()
        : AES_DEC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 1),
          AES_DEC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 2),
          AES_DEC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 3),
          AES_DEC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 4),
          AES_DEC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 5),
          AES_DEC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 6),
          AES_DEC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 7),
          AES_DEC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 8),
          AES_DEC_INPUT_ROUND_KEY(AES_128_NUM_ROUNDS, 9),
          AES_DEC_INPUT_LAST_ROUND_KEYS(AES_128_NUM_ROUNDS)
    );

    /* Store result. */
    AES_ENC_DEC_STORE_RESULT();
}

void aes192DecryptBlock(const Aes192Context *ctx, void *dst, const void *src) {
    /* Setup for asm */
    AES_ENC_DEC_SETUP_VARS();

    /* Use optimized assembly to do all rounds. */
    __asm__ __volatile__ (
        AES_DEC_ROUND(1)
        AES_DEC_ROUND(2)
        AES_DEC_ROUND(3)
        AES_DEC_ROUND(4)
        AES_DEC_ROUND(5)
        AES_DEC_ROUND(6)
        AES_DEC_ROUND(7)
        AES_DEC_ROUND(8)
        AES_DEC_ROUND(9)
        AES_DEC_ROUND(10)
        AES_DEC_ROUND(11)
        AES_DEC_FINAL_ROUND()
        : AES_ENC_DEC_OUTPUT_VARS()
        : AES_DEC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 1),
          AES_DEC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 2),
          AES_DEC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 3),
          AES_DEC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 4),
          AES_DEC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 5),
          AES_DEC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 6),
          AES_DEC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 7),
          AES_DEC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 8),
          AES_DEC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 9),
          AES_DEC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 10),
          AES_DEC_INPUT_ROUND_KEY(AES_192_NUM_ROUNDS, 11),
          AES_DEC_INPUT_LAST_ROUND_KEYS(AES_192_NUM_ROUNDS)
    );

    /* Store result. */
    AES_ENC_DEC_STORE_RESULT();
}

void aes256DecryptBlock(const Aes256Context *ctx, void *dst, const void *src) {
    /* Setup for asm */
    AES_ENC_DEC_SETUP_VARS();

    /* Use optimized assembly to do all rounds. */
    __asm__ __volatile__ (
        AES_DEC_ROUND(1)
        AES_DEC_ROUND(2)
        AES_DEC_ROUND(3)
        AES_DEC_ROUND(4)
        AES_DEC_ROUND(5)
        AES_DEC_ROUND(6)
        AES_DEC_ROUND(7)
        AES_DEC_ROUND(8)
        AES_DEC_ROUND(9)
        AES_DEC_ROUND(10)
        AES_DEC_ROUND(11)
        AES_DEC_ROUND(12)
        AES_DEC_ROUND(13)
        AES_DEC_FINAL_ROUND()
        : AES_ENC_DEC_OUTPUT_VARS()
        : AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 1),
          AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 2),
          AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 3),
          AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 4),
          AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 5),
          AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 6),
          AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 7),
          AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 8),
          AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 9),
          AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 10),
          AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 11),
          AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 12),
          AES_DEC_INPUT_ROUND_KEY(AES_256_NUM_ROUNDS, 13),
          AES_DEC_INPUT_LAST_ROUND_KEYS(AES_256_NUM_ROUNDS)
    );

    /* Store result. */
    AES_ENC_DEC_STORE_RESULT();
}