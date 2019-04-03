#include <string.h>
#include <stdlib.h>
#include <arm_neon.h>

#include "result.h"
#include "crypto/aes_cbc.h"

/* Variable management macros. */
#define DECLARE_ROUND_KEY_VAR(n) \
const uint8x16_t round_key_##n = vld1q_u8(ctx->aes_ctx.round_keys[n])

#define AES_ENC_DEC_OUTPUT_THREE_BLOCKS() \
[tmp0]"+w"(tmp0), [tmp1]"+w"(tmp1), [tmp2]"+w"(tmp2)

#define AES_ENC_DEC_OUTPUT_ONE_BLOCK() \
[tmp0]"+w"(tmp0)

#define AES_ENC_DEC_INPUT_ROUND_KEY(n) \
[round_key_##n]"w"(round_key_##n)

/* AES Encryption macros. */
#define AES_ENC_ROUND(n, i) \
"aese %[tmp" #i "].16b, %[round_key_" #n "].16b\n" \
"aesmc %[tmp" #i "].16b, %[tmp" #i "].16b\n"

#define AES_ENC_SECOND_LAST_ROUND(n, i) \
"aese %[tmp" #i "].16b, %[round_key_" #n "].16b\n"

#define AES_ENC_LAST_ROUND(n, i) \
"eor %[tmp" #i "].16b, %[tmp" #i "].16b, %[round_key_" #n "].16b\n"

/* AES Decryption macros. */
#define AES_DEC_ROUND(n, i) \
"aesd %[tmp" #i "].16b, %[round_key_" #n "].16b\n" \
"aesimc %[tmp" #i "].16b, %[tmp" #i "].16b\n"

#define AES_DEC_SECOND_LAST_ROUND(n, i) \
"aesd %[tmp" #i "].16b, %[round_key_" #n "].16b\n"

#define AES_DEC_LAST_ROUND(n, i) \
"eor %[tmp" #i "].16b, %[tmp" #i "].16b, %[round_key_" #n "].16b\n"


/* Macro for main body of crypt wrapper. */
#define CRYPT_FUNC_BODY(block_handler) \
do { \
    const u8 *cur_src = src; \
    u8 *cur_dst = dst; \
\
    /* Handle pre-buffered data. */ \
    if (ctx->num_buffered > 0) { \
        const size_t needed = AES_BLOCK_SIZE - ctx->num_buffered; \
        const size_t copyable = (size > needed ? needed : size); \
        memcpy(&ctx->buffer[ctx->num_buffered], cur_src, copyable); \
        cur_src += copyable; \
        ctx->num_buffered += copyable; \
        size -= copyable; \
\
        if (ctx->num_buffered == AES_BLOCK_SIZE) { \
            block_handler(ctx, cur_dst, ctx->buffer, 1); \
            cur_dst += AES_BLOCK_SIZE; \
            ctx->num_buffered = 0; \
        } \
    } \
\
    /* Handle complete blocks. */ \
    if (size >= AES_BLOCK_SIZE) { \
        const size_t num_blocks = size / AES_BLOCK_SIZE; \
        block_handler(ctx, cur_dst, cur_src, num_blocks); \
        size -= num_blocks * AES_BLOCK_SIZE; \
        cur_src += num_blocks * AES_BLOCK_SIZE; \
        cur_dst += num_blocks * AES_BLOCK_SIZE; \
    } \
\
    /* Buffer remaining data. */ \
    if (size > 0) { \
        memcpy(ctx->buffer, cur_src, size); \
        ctx->num_buffered = size; \
    } \
    return (size_t)((uintptr_t)cur_dst - (uintptr_t)dst); \
} while (0)


void aes128CbcContextCreate(Aes128CbcContext *out, const void *key, const void *iv, bool is_encryptor) {
    /* Initialize inner context. */
    aes128ContextCreate(&out->aes_ctx, key, is_encryptor);
    aes128CbcContextResetIv(out, iv);
}

void aes128CbcContextResetIv(Aes128CbcContext *ctx, const void *iv) {
    /* Set IV, nothing is buffered. */
    memcpy(ctx->iv, iv, sizeof(ctx->iv));
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
    ctx->num_buffered = 0;
}

static inline void _aes128CbcEncryptBlocks(Aes128CbcContext *ctx, u8 *dst_u8, const u8 *src_u8, size_t num_blocks) {
    /* Preload all round keys + iv into neon registers. */
    DECLARE_ROUND_KEY_VAR(0);
    DECLARE_ROUND_KEY_VAR(1);
    DECLARE_ROUND_KEY_VAR(2);
    DECLARE_ROUND_KEY_VAR(3);
    DECLARE_ROUND_KEY_VAR(4);
    DECLARE_ROUND_KEY_VAR(5);
    DECLARE_ROUND_KEY_VAR(6);
    DECLARE_ROUND_KEY_VAR(7);
    DECLARE_ROUND_KEY_VAR(8);
    DECLARE_ROUND_KEY_VAR(9);
    DECLARE_ROUND_KEY_VAR(10);
    uint8x16_t cur_iv = vld1q_u8(ctx->iv);

    /* Process last block or two individually. */
    while (num_blocks >= 1) {
        /* Read block in, xor with IV. */
        uint8x16_t tmp0 = veorq_u8(cur_iv, vld1q_u8(src_u8));
        src_u8 += AES_BLOCK_SIZE;

        /* Actually do encryption, use optimized asm. */
        __asm__ __volatile__ (
            AES_ENC_ROUND(0, 0)
            AES_ENC_ROUND(1, 0)
            AES_ENC_ROUND(2, 0)
            AES_ENC_ROUND(3, 0)
            AES_ENC_ROUND(4, 0)
            AES_ENC_ROUND(5, 0)
            AES_ENC_ROUND(6, 0)
            AES_ENC_ROUND(7, 0)
            AES_ENC_ROUND(8, 0)
            AES_ENC_SECOND_LAST_ROUND(9, 0)
            AES_ENC_LAST_ROUND(10, 0)
            : AES_ENC_DEC_OUTPUT_ONE_BLOCK()
            : AES_ENC_DEC_INPUT_ROUND_KEY(0),
              AES_ENC_DEC_INPUT_ROUND_KEY(1),
              AES_ENC_DEC_INPUT_ROUND_KEY(2),
              AES_ENC_DEC_INPUT_ROUND_KEY(3),
              AES_ENC_DEC_INPUT_ROUND_KEY(4),
              AES_ENC_DEC_INPUT_ROUND_KEY(5),
              AES_ENC_DEC_INPUT_ROUND_KEY(6),
              AES_ENC_DEC_INPUT_ROUND_KEY(7),
              AES_ENC_DEC_INPUT_ROUND_KEY(8),
              AES_ENC_DEC_INPUT_ROUND_KEY(9),
              AES_ENC_DEC_INPUT_ROUND_KEY(10)
        );

        /* Update IV. */
        cur_iv = tmp0;

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks--;
    }

    vst1q_u8(ctx->iv, cur_iv);
}

static inline void _aes128CbcDecryptBlocks(Aes128CbcContext *ctx, u8 *dst_u8, const u8 *src_u8, size_t num_blocks) {
    /* Preload all round keys + iv into neon registers. */
    DECLARE_ROUND_KEY_VAR(0);
    DECLARE_ROUND_KEY_VAR(1);
    DECLARE_ROUND_KEY_VAR(2);
    DECLARE_ROUND_KEY_VAR(3);
    DECLARE_ROUND_KEY_VAR(4);
    DECLARE_ROUND_KEY_VAR(5);
    DECLARE_ROUND_KEY_VAR(6);
    DECLARE_ROUND_KEY_VAR(7);
    DECLARE_ROUND_KEY_VAR(8);
    DECLARE_ROUND_KEY_VAR(9);
    DECLARE_ROUND_KEY_VAR(10);
    uint8x16_t cur_iv = vld1q_u8(ctx->iv);

    /* Process three blocks at a time, when possible. */
    while (num_blocks >= 3) {
        /* Read blocks in. Keep them in registers for XOR later. */
        const uint8x16_t block0 = vld1q_u8(src_u8);
        src_u8 += AES_BLOCK_SIZE;
        const uint8x16_t block1 = vld1q_u8(src_u8);
        src_u8 += AES_BLOCK_SIZE;
        const uint8x16_t block2 = vld1q_u8(src_u8);
        src_u8 += AES_BLOCK_SIZE;

        uint8x16_t tmp0 = block0, tmp1 = block1, tmp2 = block2;

        /* Actually do encryption, use optimized asm. */
        __asm__ __volatile__ (
            AES_DEC_ROUND(10, 0) AES_DEC_ROUND(10, 1) AES_DEC_ROUND(10, 2)
            AES_DEC_ROUND(9, 0) AES_DEC_ROUND(9, 1) AES_DEC_ROUND(9, 2)
            AES_DEC_ROUND(8, 0) AES_DEC_ROUND(8, 1) AES_DEC_ROUND(8, 2)
            AES_DEC_ROUND(7, 0) AES_DEC_ROUND(7, 1) AES_DEC_ROUND(7, 2)
            AES_DEC_ROUND(6, 0) AES_DEC_ROUND(6, 1) AES_DEC_ROUND(6, 2)
            AES_DEC_ROUND(5, 0) AES_DEC_ROUND(5, 1) AES_DEC_ROUND(5, 2)
            AES_DEC_ROUND(4, 0) AES_DEC_ROUND(4, 1) AES_DEC_ROUND(4, 2)
            AES_DEC_ROUND(3, 0) AES_DEC_ROUND(3, 1) AES_DEC_ROUND(3, 2)
            AES_DEC_ROUND(2, 0) AES_DEC_ROUND(2, 1) AES_DEC_ROUND(2, 2)
            AES_DEC_SECOND_LAST_ROUND(1, 0) AES_DEC_SECOND_LAST_ROUND(1, 1) AES_DEC_SECOND_LAST_ROUND(1, 2)
            AES_DEC_LAST_ROUND(0, 0) AES_DEC_LAST_ROUND(0, 1) AES_DEC_LAST_ROUND(0, 2)
            : AES_ENC_DEC_OUTPUT_THREE_BLOCKS()
            : AES_ENC_DEC_INPUT_ROUND_KEY(0),
              AES_ENC_DEC_INPUT_ROUND_KEY(1),
              AES_ENC_DEC_INPUT_ROUND_KEY(2),
              AES_ENC_DEC_INPUT_ROUND_KEY(3),
              AES_ENC_DEC_INPUT_ROUND_KEY(4),
              AES_ENC_DEC_INPUT_ROUND_KEY(5),
              AES_ENC_DEC_INPUT_ROUND_KEY(6),
              AES_ENC_DEC_INPUT_ROUND_KEY(7),
              AES_ENC_DEC_INPUT_ROUND_KEY(8),
              AES_ENC_DEC_INPUT_ROUND_KEY(9),
              AES_ENC_DEC_INPUT_ROUND_KEY(10)
        );

        /* Do XOR for CBC. */
        tmp0 = veorq_u8(tmp0, cur_iv);
        tmp1 = veorq_u8(tmp1, block0);
        tmp2 = veorq_u8(tmp2, block1);
        cur_iv = block2;

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;
        vst1q_u8(dst_u8, tmp1);
        dst_u8 += AES_BLOCK_SIZE;
        vst1q_u8(dst_u8, tmp2);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks -= 3;
    }

    /* Process last block or two individually. */
    while (num_blocks >= 1) {
        /* Read block in, keep in register for IV later. */
        const uint8x16_t block0 = vld1q_u8(src_u8);
        src_u8 += AES_BLOCK_SIZE;

        uint8x16_t tmp0 = block0;

        /* Actually do encryption, use optimized asm. */
        __asm__ __volatile__ (
            AES_DEC_ROUND(10, 0)
            AES_DEC_ROUND(9, 0)
            AES_DEC_ROUND(8, 0)
            AES_DEC_ROUND(7, 0)
            AES_DEC_ROUND(6, 0)
            AES_DEC_ROUND(5, 0)
            AES_DEC_ROUND(4, 0)
            AES_DEC_ROUND(3, 0)
            AES_DEC_ROUND(2, 0)
            AES_DEC_SECOND_LAST_ROUND(1, 0)
            AES_DEC_LAST_ROUND(0, 0)
            : AES_ENC_DEC_OUTPUT_ONE_BLOCK()
            : AES_ENC_DEC_INPUT_ROUND_KEY(0),
              AES_ENC_DEC_INPUT_ROUND_KEY(1),
              AES_ENC_DEC_INPUT_ROUND_KEY(2),
              AES_ENC_DEC_INPUT_ROUND_KEY(3),
              AES_ENC_DEC_INPUT_ROUND_KEY(4),
              AES_ENC_DEC_INPUT_ROUND_KEY(5),
              AES_ENC_DEC_INPUT_ROUND_KEY(6),
              AES_ENC_DEC_INPUT_ROUND_KEY(7),
              AES_ENC_DEC_INPUT_ROUND_KEY(8),
              AES_ENC_DEC_INPUT_ROUND_KEY(9),
              AES_ENC_DEC_INPUT_ROUND_KEY(10)
        );

        /* Update IV. */
        cur_iv = tmp0;

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks--;
    }

    vst1q_u8(ctx->iv, cur_iv);
}

size_t aes128CbcEncrypt(Aes128CbcContext *ctx, void *dst, const void *src, size_t size) {
    CRYPT_FUNC_BODY(_aes128CbcEncryptBlocks);
}

size_t aes128CbcDecrypt(Aes128CbcContext *ctx, void *dst, const void *src, size_t size) {
    CRYPT_FUNC_BODY(_aes128CbcDecryptBlocks);
}

void aes192CbcContextCreate(Aes192CbcContext *out, const void *key, const void *iv, bool is_encryptor) {
    /* Initialize inner context. */
    aes192ContextCreate(&out->aes_ctx, key, is_encryptor);
    aes192CbcContextResetIv(out, iv);
}

void aes192CbcContextResetIv(Aes192CbcContext *ctx, const void *iv) {
    /* Set IV, nothing is buffered. */
    memcpy(ctx->iv, iv, sizeof(ctx->iv));
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
    ctx->num_buffered = 0;
}

static inline void _aes192CbcEncryptBlocks(Aes192CbcContext *ctx, u8 *dst_u8, const u8 *src_u8, size_t num_blocks) {
    /* Preload all round keys + iv into neon registers. */
    DECLARE_ROUND_KEY_VAR(0);
    DECLARE_ROUND_KEY_VAR(1);
    DECLARE_ROUND_KEY_VAR(2);
    DECLARE_ROUND_KEY_VAR(3);
    DECLARE_ROUND_KEY_VAR(4);
    DECLARE_ROUND_KEY_VAR(5);
    DECLARE_ROUND_KEY_VAR(6);
    DECLARE_ROUND_KEY_VAR(7);
    DECLARE_ROUND_KEY_VAR(8);
    DECLARE_ROUND_KEY_VAR(9);
    DECLARE_ROUND_KEY_VAR(10);
    DECLARE_ROUND_KEY_VAR(11);
    DECLARE_ROUND_KEY_VAR(12);
    uint8x16_t cur_iv = vld1q_u8(ctx->iv);

    /* Process last block or two individually. */
    while (num_blocks >= 1) {
        /* Read block in, xor with IV. */
        uint8x16_t tmp0 = veorq_u8(cur_iv, vld1q_u8(src_u8));
        src_u8 += AES_BLOCK_SIZE;

        /* Actually do encryption, use optimized asm. */
        __asm__ __volatile__ (
            AES_ENC_ROUND(0, 0)
            AES_ENC_ROUND(1, 0)
            AES_ENC_ROUND(2, 0)
            AES_ENC_ROUND(3, 0)
            AES_ENC_ROUND(4, 0)
            AES_ENC_ROUND(5, 0)
            AES_ENC_ROUND(6, 0)
            AES_ENC_ROUND(7, 0)
            AES_ENC_ROUND(8, 0)
            AES_ENC_ROUND(9, 0)
            AES_ENC_ROUND(10, 0)
            AES_ENC_SECOND_LAST_ROUND(11, 0)
            AES_ENC_LAST_ROUND(12, 0)
            : AES_ENC_DEC_OUTPUT_ONE_BLOCK()
            : AES_ENC_DEC_INPUT_ROUND_KEY(0),
              AES_ENC_DEC_INPUT_ROUND_KEY(1),
              AES_ENC_DEC_INPUT_ROUND_KEY(2),
              AES_ENC_DEC_INPUT_ROUND_KEY(3),
              AES_ENC_DEC_INPUT_ROUND_KEY(4),
              AES_ENC_DEC_INPUT_ROUND_KEY(5),
              AES_ENC_DEC_INPUT_ROUND_KEY(6),
              AES_ENC_DEC_INPUT_ROUND_KEY(7),
              AES_ENC_DEC_INPUT_ROUND_KEY(8),
              AES_ENC_DEC_INPUT_ROUND_KEY(9),
              AES_ENC_DEC_INPUT_ROUND_KEY(10),
              AES_ENC_DEC_INPUT_ROUND_KEY(11),
              AES_ENC_DEC_INPUT_ROUND_KEY(12)
        );

        /* Update IV. */
        cur_iv = tmp0;

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks--;
    }

    vst1q_u8(ctx->iv, cur_iv);
}

static inline void _aes192CbcDecryptBlocks(Aes192CbcContext *ctx, u8 *dst_u8, const u8 *src_u8, size_t num_blocks) {
    /* Preload all round keys + iv into neon registers. */
    DECLARE_ROUND_KEY_VAR(0);
    DECLARE_ROUND_KEY_VAR(1);
    DECLARE_ROUND_KEY_VAR(2);
    DECLARE_ROUND_KEY_VAR(3);
    DECLARE_ROUND_KEY_VAR(4);
    DECLARE_ROUND_KEY_VAR(5);
    DECLARE_ROUND_KEY_VAR(6);
    DECLARE_ROUND_KEY_VAR(7);
    DECLARE_ROUND_KEY_VAR(8);
    DECLARE_ROUND_KEY_VAR(9);
    DECLARE_ROUND_KEY_VAR(10);
    DECLARE_ROUND_KEY_VAR(11);
    DECLARE_ROUND_KEY_VAR(12);
    uint8x16_t cur_iv = vld1q_u8(ctx->iv);

    /* Process three blocks at a time, when possible. */
    while (num_blocks >= 3) {
        /* Read blocks in. Keep them in registers for XOR later. */
        const uint8x16_t block0 = vld1q_u8(src_u8);
        src_u8 += AES_BLOCK_SIZE;
        const uint8x16_t block1 = vld1q_u8(src_u8);
        src_u8 += AES_BLOCK_SIZE;
        const uint8x16_t block2 = vld1q_u8(src_u8);
        src_u8 += AES_BLOCK_SIZE;

        uint8x16_t tmp0 = block0, tmp1 = block1, tmp2 = block2;

        /* Actually do encryption, use optimized asm. */
        __asm__ __volatile__ (
            AES_DEC_ROUND(12, 0) AES_DEC_ROUND(12, 1) AES_DEC_ROUND(12, 2)
            AES_DEC_ROUND(11, 0) AES_DEC_ROUND(11, 1) AES_DEC_ROUND(11, 2)
            AES_DEC_ROUND(10, 0) AES_DEC_ROUND(10, 1) AES_DEC_ROUND(10, 2)
            AES_DEC_ROUND(9, 0) AES_DEC_ROUND(9, 1) AES_DEC_ROUND(9, 2)
            AES_DEC_ROUND(8, 0) AES_DEC_ROUND(8, 1) AES_DEC_ROUND(8, 2)
            AES_DEC_ROUND(7, 0) AES_DEC_ROUND(7, 1) AES_DEC_ROUND(7, 2)
            AES_DEC_ROUND(6, 0) AES_DEC_ROUND(6, 1) AES_DEC_ROUND(6, 2)
            AES_DEC_ROUND(5, 0) AES_DEC_ROUND(5, 1) AES_DEC_ROUND(5, 2)
            AES_DEC_ROUND(4, 0) AES_DEC_ROUND(4, 1) AES_DEC_ROUND(4, 2)
            AES_DEC_ROUND(3, 0) AES_DEC_ROUND(3, 1) AES_DEC_ROUND(3, 2)
            AES_DEC_ROUND(2, 0) AES_DEC_ROUND(2, 1) AES_DEC_ROUND(2, 2)
            AES_DEC_SECOND_LAST_ROUND(1, 0) AES_DEC_SECOND_LAST_ROUND(1, 1) AES_DEC_SECOND_LAST_ROUND(1, 2)
            AES_DEC_LAST_ROUND(0, 0) AES_DEC_LAST_ROUND(0, 1) AES_DEC_LAST_ROUND(0, 2)
            : AES_ENC_DEC_OUTPUT_THREE_BLOCKS()
            : AES_ENC_DEC_INPUT_ROUND_KEY(0),
              AES_ENC_DEC_INPUT_ROUND_KEY(1),
              AES_ENC_DEC_INPUT_ROUND_KEY(2),
              AES_ENC_DEC_INPUT_ROUND_KEY(3),
              AES_ENC_DEC_INPUT_ROUND_KEY(4),
              AES_ENC_DEC_INPUT_ROUND_KEY(5),
              AES_ENC_DEC_INPUT_ROUND_KEY(6),
              AES_ENC_DEC_INPUT_ROUND_KEY(7),
              AES_ENC_DEC_INPUT_ROUND_KEY(8),
              AES_ENC_DEC_INPUT_ROUND_KEY(9),
              AES_ENC_DEC_INPUT_ROUND_KEY(10),
              AES_ENC_DEC_INPUT_ROUND_KEY(11),
              AES_ENC_DEC_INPUT_ROUND_KEY(12)
        );

        /* Do XOR for CBC. */
        tmp0 = veorq_u8(tmp0, cur_iv);
        tmp1 = veorq_u8(tmp1, block0);
        tmp2 = veorq_u8(tmp2, block1);
        cur_iv = block2;

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;
        vst1q_u8(dst_u8, tmp1);
        dst_u8 += AES_BLOCK_SIZE;
        vst1q_u8(dst_u8, tmp2);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks -= 3;
    }

    /* Process last block or two individually. */
    while (num_blocks >= 1) {
        /* Read block in, keep in register for IV later. */
        const uint8x16_t block0 = vld1q_u8(src_u8);
        src_u8 += AES_BLOCK_SIZE;

        uint8x16_t tmp0 = block0;

        /* Actually do encryption, use optimized asm. */
        __asm__ __volatile__ (
            AES_DEC_ROUND(12, 0)
            AES_DEC_ROUND(11, 0)
            AES_DEC_ROUND(10, 0)
            AES_DEC_ROUND(9, 0)
            AES_DEC_ROUND(8, 0)
            AES_DEC_ROUND(7, 0)
            AES_DEC_ROUND(6, 0)
            AES_DEC_ROUND(5, 0)
            AES_DEC_ROUND(4, 0)
            AES_DEC_ROUND(3, 0)
            AES_DEC_ROUND(2, 0)
            AES_DEC_SECOND_LAST_ROUND(1, 0)
            AES_DEC_LAST_ROUND(0, 0)
            : AES_ENC_DEC_OUTPUT_ONE_BLOCK()
            : AES_ENC_DEC_INPUT_ROUND_KEY(0),
              AES_ENC_DEC_INPUT_ROUND_KEY(1),
              AES_ENC_DEC_INPUT_ROUND_KEY(2),
              AES_ENC_DEC_INPUT_ROUND_KEY(3),
              AES_ENC_DEC_INPUT_ROUND_KEY(4),
              AES_ENC_DEC_INPUT_ROUND_KEY(5),
              AES_ENC_DEC_INPUT_ROUND_KEY(6),
              AES_ENC_DEC_INPUT_ROUND_KEY(7),
              AES_ENC_DEC_INPUT_ROUND_KEY(8),
              AES_ENC_DEC_INPUT_ROUND_KEY(9),
              AES_ENC_DEC_INPUT_ROUND_KEY(10),
              AES_ENC_DEC_INPUT_ROUND_KEY(11),
              AES_ENC_DEC_INPUT_ROUND_KEY(12)
        );

        /* Update IV. */
        cur_iv = tmp0;

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks--;
    }

    vst1q_u8(ctx->iv, cur_iv);
}

size_t aes192CbcEncrypt(Aes192CbcContext *ctx, void *dst, const void *src, size_t size) {
    CRYPT_FUNC_BODY(_aes192CbcEncryptBlocks);
}

size_t aes192CbcDecrypt(Aes192CbcContext *ctx, void *dst, const void *src, size_t size) {
    CRYPT_FUNC_BODY(_aes192CbcDecryptBlocks);
}

void aes256CbcContextCreate(Aes256CbcContext *out, const void *key, const void *iv, bool is_encryptor) {
    /* Initialize inner context. */
    aes256ContextCreate(&out->aes_ctx, key, is_encryptor);
    aes256CbcContextResetIv(out, iv);
}

void aes256CbcContextResetIv(Aes256CbcContext *ctx, const void *iv) {
    /* Set IV, nothing is buffered. */
    memcpy(ctx->iv, iv, sizeof(ctx->iv));
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
    ctx->num_buffered = 0;
}

static inline void _aes256CbcEncryptBlocks(Aes256CbcContext *ctx, u8 *dst_u8, const u8 *src_u8, size_t num_blocks) {
    /* Preload all round keys + iv into neon registers. */
    DECLARE_ROUND_KEY_VAR(0);
    DECLARE_ROUND_KEY_VAR(1);
    DECLARE_ROUND_KEY_VAR(2);
    DECLARE_ROUND_KEY_VAR(3);
    DECLARE_ROUND_KEY_VAR(4);
    DECLARE_ROUND_KEY_VAR(5);
    DECLARE_ROUND_KEY_VAR(6);
    DECLARE_ROUND_KEY_VAR(7);
    DECLARE_ROUND_KEY_VAR(8);
    DECLARE_ROUND_KEY_VAR(9);
    DECLARE_ROUND_KEY_VAR(10);
    DECLARE_ROUND_KEY_VAR(11);
    DECLARE_ROUND_KEY_VAR(12);
    DECLARE_ROUND_KEY_VAR(13);
    DECLARE_ROUND_KEY_VAR(14);
    uint8x16_t cur_iv = vld1q_u8(ctx->iv);

    /* Process last block or two individually. */
    while (num_blocks >= 1) {
        /* Read block in, xor with IV. */
        uint8x16_t tmp0 = veorq_u8(cur_iv, vld1q_u8(src_u8));
        src_u8 += AES_BLOCK_SIZE;

        /* Actually do encryption, use optimized asm. */
        __asm__ __volatile__ (
            AES_ENC_ROUND(0, 0)
            AES_ENC_ROUND(1, 0)
            AES_ENC_ROUND(2, 0)
            AES_ENC_ROUND(3, 0)
            AES_ENC_ROUND(4, 0)
            AES_ENC_ROUND(5, 0)
            AES_ENC_ROUND(6, 0)
            AES_ENC_ROUND(7, 0)
            AES_ENC_ROUND(8, 0)
            AES_ENC_ROUND(9, 0)
            AES_ENC_ROUND(10, 0)
            AES_ENC_ROUND(11, 0)
            AES_ENC_ROUND(12, 0)
            AES_ENC_SECOND_LAST_ROUND(13, 0)
            AES_ENC_LAST_ROUND(14, 0)
            : AES_ENC_DEC_OUTPUT_ONE_BLOCK()
            : AES_ENC_DEC_INPUT_ROUND_KEY(0),
              AES_ENC_DEC_INPUT_ROUND_KEY(1),
              AES_ENC_DEC_INPUT_ROUND_KEY(2),
              AES_ENC_DEC_INPUT_ROUND_KEY(3),
              AES_ENC_DEC_INPUT_ROUND_KEY(4),
              AES_ENC_DEC_INPUT_ROUND_KEY(5),
              AES_ENC_DEC_INPUT_ROUND_KEY(6),
              AES_ENC_DEC_INPUT_ROUND_KEY(7),
              AES_ENC_DEC_INPUT_ROUND_KEY(8),
              AES_ENC_DEC_INPUT_ROUND_KEY(9),
              AES_ENC_DEC_INPUT_ROUND_KEY(10),
              AES_ENC_DEC_INPUT_ROUND_KEY(11),
              AES_ENC_DEC_INPUT_ROUND_KEY(12),
              AES_ENC_DEC_INPUT_ROUND_KEY(13),
              AES_ENC_DEC_INPUT_ROUND_KEY(14)
        );

        /* Update IV. */
        cur_iv = tmp0;

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks--;
    }

    vst1q_u8(ctx->iv, cur_iv);
}

static inline void _aes256CbcDecryptBlocks(Aes256CbcContext *ctx, u8 *dst_u8, const u8 *src_u8, size_t num_blocks) {
    /* Preload all round keys + iv into neon registers. */
    DECLARE_ROUND_KEY_VAR(0);
    DECLARE_ROUND_KEY_VAR(1);
    DECLARE_ROUND_KEY_VAR(2);
    DECLARE_ROUND_KEY_VAR(3);
    DECLARE_ROUND_KEY_VAR(4);
    DECLARE_ROUND_KEY_VAR(5);
    DECLARE_ROUND_KEY_VAR(6);
    DECLARE_ROUND_KEY_VAR(7);
    DECLARE_ROUND_KEY_VAR(8);
    DECLARE_ROUND_KEY_VAR(9);
    DECLARE_ROUND_KEY_VAR(10);
    DECLARE_ROUND_KEY_VAR(11);
    DECLARE_ROUND_KEY_VAR(12);
    DECLARE_ROUND_KEY_VAR(13);
    DECLARE_ROUND_KEY_VAR(14);
    uint8x16_t cur_iv = vld1q_u8(ctx->iv);

    /* Process three blocks at a time, when possible. */
    while (num_blocks >= 3) {
        /* Read blocks in. Keep them in registers for XOR later. */
        const uint8x16_t block0 = vld1q_u8(src_u8);
        src_u8 += AES_BLOCK_SIZE;
        const uint8x16_t block1 = vld1q_u8(src_u8);
        src_u8 += AES_BLOCK_SIZE;
        const uint8x16_t block2 = vld1q_u8(src_u8);
        src_u8 += AES_BLOCK_SIZE;

        uint8x16_t tmp0 = block0, tmp1 = block1, tmp2 = block2;

        /* Actually do encryption, use optimized asm. */
        __asm__ __volatile__ (
            AES_DEC_ROUND(14, 0) AES_DEC_ROUND(14, 1) AES_DEC_ROUND(14, 2)
            AES_DEC_ROUND(13, 0) AES_DEC_ROUND(13, 1) AES_DEC_ROUND(13, 2)
            AES_DEC_ROUND(12, 0) AES_DEC_ROUND(12, 1) AES_DEC_ROUND(12, 2)
            AES_DEC_ROUND(11, 0) AES_DEC_ROUND(11, 1) AES_DEC_ROUND(11, 2)
            AES_DEC_ROUND(10, 0) AES_DEC_ROUND(10, 1) AES_DEC_ROUND(10, 2)
            AES_DEC_ROUND(9, 0) AES_DEC_ROUND(9, 1) AES_DEC_ROUND(9, 2)
            AES_DEC_ROUND(8, 0) AES_DEC_ROUND(8, 1) AES_DEC_ROUND(8, 2)
            AES_DEC_ROUND(7, 0) AES_DEC_ROUND(7, 1) AES_DEC_ROUND(7, 2)
            AES_DEC_ROUND(6, 0) AES_DEC_ROUND(6, 1) AES_DEC_ROUND(6, 2)
            AES_DEC_ROUND(5, 0) AES_DEC_ROUND(5, 1) AES_DEC_ROUND(5, 2)
            AES_DEC_ROUND(4, 0) AES_DEC_ROUND(4, 1) AES_DEC_ROUND(4, 2)
            AES_DEC_ROUND(3, 0) AES_DEC_ROUND(3, 1) AES_DEC_ROUND(3, 2)
            AES_DEC_ROUND(2, 0) AES_DEC_ROUND(2, 1) AES_DEC_ROUND(2, 2)
            AES_DEC_SECOND_LAST_ROUND(1, 0) AES_DEC_SECOND_LAST_ROUND(1, 1) AES_DEC_SECOND_LAST_ROUND(1, 2)
            AES_DEC_LAST_ROUND(0, 0) AES_DEC_LAST_ROUND(0, 1) AES_DEC_LAST_ROUND(0, 2)
            : AES_ENC_DEC_OUTPUT_THREE_BLOCKS()
            : AES_ENC_DEC_INPUT_ROUND_KEY(0),
              AES_ENC_DEC_INPUT_ROUND_KEY(1),
              AES_ENC_DEC_INPUT_ROUND_KEY(2),
              AES_ENC_DEC_INPUT_ROUND_KEY(3),
              AES_ENC_DEC_INPUT_ROUND_KEY(4),
              AES_ENC_DEC_INPUT_ROUND_KEY(5),
              AES_ENC_DEC_INPUT_ROUND_KEY(6),
              AES_ENC_DEC_INPUT_ROUND_KEY(7),
              AES_ENC_DEC_INPUT_ROUND_KEY(8),
              AES_ENC_DEC_INPUT_ROUND_KEY(9),
              AES_ENC_DEC_INPUT_ROUND_KEY(10),
              AES_ENC_DEC_INPUT_ROUND_KEY(11),
              AES_ENC_DEC_INPUT_ROUND_KEY(12),
              AES_ENC_DEC_INPUT_ROUND_KEY(13),
              AES_ENC_DEC_INPUT_ROUND_KEY(14)
        );

        /* Do XOR for CBC. */
        tmp0 = veorq_u8(tmp0, cur_iv);
        tmp1 = veorq_u8(tmp1, block0);
        tmp2 = veorq_u8(tmp2, block1);
        cur_iv = block2;

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;
        vst1q_u8(dst_u8, tmp1);
        dst_u8 += AES_BLOCK_SIZE;
        vst1q_u8(dst_u8, tmp2);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks -= 3;
    }

    /* Process last block or two individually. */
    while (num_blocks >= 1) {
        /* Read block in, keep in register for IV later. */
        const uint8x16_t block0 = vld1q_u8(src_u8);
        src_u8 += AES_BLOCK_SIZE;

        uint8x16_t tmp0 = block0;

        /* Actually do encryption, use optimized asm. */
        __asm__ __volatile__ (
            AES_DEC_ROUND(14, 0)
            AES_DEC_ROUND(13, 0)
            AES_DEC_ROUND(12, 0)
            AES_DEC_ROUND(11, 0)
            AES_DEC_ROUND(10, 0)
            AES_DEC_ROUND(9, 0)
            AES_DEC_ROUND(8, 0)
            AES_DEC_ROUND(7, 0)
            AES_DEC_ROUND(6, 0)
            AES_DEC_ROUND(5, 0)
            AES_DEC_ROUND(4, 0)
            AES_DEC_ROUND(3, 0)
            AES_DEC_ROUND(2, 0)
            AES_DEC_SECOND_LAST_ROUND(1, 0)
            AES_DEC_LAST_ROUND(0, 0)
            : AES_ENC_DEC_OUTPUT_ONE_BLOCK()
            : AES_ENC_DEC_INPUT_ROUND_KEY(0),
              AES_ENC_DEC_INPUT_ROUND_KEY(1),
              AES_ENC_DEC_INPUT_ROUND_KEY(2),
              AES_ENC_DEC_INPUT_ROUND_KEY(3),
              AES_ENC_DEC_INPUT_ROUND_KEY(4),
              AES_ENC_DEC_INPUT_ROUND_KEY(5),
              AES_ENC_DEC_INPUT_ROUND_KEY(6),
              AES_ENC_DEC_INPUT_ROUND_KEY(7),
              AES_ENC_DEC_INPUT_ROUND_KEY(8),
              AES_ENC_DEC_INPUT_ROUND_KEY(9),
              AES_ENC_DEC_INPUT_ROUND_KEY(10),
              AES_ENC_DEC_INPUT_ROUND_KEY(11),
              AES_ENC_DEC_INPUT_ROUND_KEY(12),
              AES_ENC_DEC_INPUT_ROUND_KEY(13),
              AES_ENC_DEC_INPUT_ROUND_KEY(14)
        );

        /* Update IV. */
        cur_iv = tmp0;

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks--;
    }

    vst1q_u8(ctx->iv, cur_iv);
}

size_t aes256CbcEncrypt(Aes256CbcContext *ctx, void *dst, const void *src, size_t size) {
    CRYPT_FUNC_BODY(_aes256CbcEncryptBlocks);
}

size_t aes256CbcDecrypt(Aes256CbcContext *ctx, void *dst, const void *src, size_t size) {
    CRYPT_FUNC_BODY(_aes256CbcDecryptBlocks);
}
