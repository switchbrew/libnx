#include <string.h>
#include <stdlib.h>
#include <arm_neon.h>

#include "result.h"
#include "crypto/cmac.h"

/* Variable management macros. */
#define DECLARE_ROUND_KEY_VAR(n) \
const uint8x16_t round_key_##n = vld1q_u8(ctx->ctx.round_keys[n])

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

/* Function body macros. */
#define CMAC_CONTEXT_CREATE(cipher) \
do { \
    cipher##ContextCreate(&out->ctx, key, true); \
    memset(&out->subkey, 0, sizeof(out->subkey)); \
    cipher##EncryptBlock(&out->ctx, out->subkey, out->subkey); \
    vst1q_u8(out->subkey, _galoisMultiply(vld1q_u8(out->subkey))); \
    memset(out->mac, 0, sizeof(out->mac)); \
    memset(out->buffer, 0, sizeof(out->buffer)); \
    out->num_buffered = 0; \
    out->finalized = false; \
} while (0)

#define CMAC_CONTEXT_UPDATE(cipher) \
do { \
    const u8 *cur_src = src; \
\
    /* Handle pre-buffered data. */ \
    if (ctx->num_buffered > 0) { \
        const size_t needed = sizeof(ctx->buffer) - ctx->num_buffered; \
        const size_t copyable = (size > needed ? needed : size); \
        memcpy(&ctx->buffer[ctx->num_buffered], cur_src, copyable); \
        cur_src += copyable; \
        ctx->num_buffered += copyable; \
        size -= copyable; \
\
        if (ctx->num_buffered == sizeof(ctx->buffer) && size > 0) { \
            _cmac##cipher##ProcessBlocks(ctx, ctx->buffer, 1); \
            ctx->num_buffered = 0; \
        } \
    } \
\
    /* Handle complete blocks. */ \
    if (size >= sizeof(ctx->buffer)) { \
        const size_t num_blocks = size / sizeof(ctx->buffer); \
        if (num_blocks > 1) { \
            _cmac##cipher##ProcessBlocks(ctx, cur_src, num_blocks - 1); \
            size -= (num_blocks - 1) * sizeof(ctx->buffer); \
            cur_src += (num_blocks - 1) * sizeof(ctx->buffer); \
        } \
        memcpy(ctx->buffer, cur_src, sizeof(ctx->buffer)); \
        size -= sizeof(ctx->buffer); \
        cur_src += sizeof(ctx->buffer); \
        ctx->num_buffered = sizeof(ctx->buffer); \
    } \
\
    /* Buffer remaining data. */ \
    if (size > 0) { \
        if (ctx->num_buffered == AES_BLOCK_SIZE) { \
            _cmac##cipher##ProcessBlocks(ctx, ctx->buffer, 1); \
            ctx->num_buffered = 0; \
        } \
        memcpy(ctx->buffer, cur_src, size); \
        ctx->num_buffered = size; \
    } \
} while (0)

#define CMAC_CONTEXT_GET_MAC(cipher) \
do { \
    if (!ctx->finalized) { \
        /* Handle unaligned buffered data. */ \
        if (ctx->num_buffered != sizeof(ctx->buffer)) { \
            static const u8 padding[sizeof(ctx->buffer)] = {0x80,}; \
            const size_t needed = sizeof(ctx->buffer) - ctx->num_buffered; \
            memcpy(ctx->buffer + ctx->num_buffered, padding, needed); \
            ctx->num_buffered = sizeof(ctx->buffer); \
            vst1q_u8(ctx->subkey, _galoisMultiply(vld1q_u8(ctx->subkey))); \
        } \
        /* Mask in subkey. */ \
        for (size_t i = 0; i < sizeof(ctx->buffer); i++) { \
            ctx->buffer[i] ^= ctx->subkey[i]; \
        } \
        _cmac##cipher##ProcessBlocks(ctx, ctx->buffer, 1); \
        ctx->num_buffered = 0; \
        ctx->finalized = true; \
    } \
\
    memcpy(dst, ctx->mac, sizeof(ctx->mac)); \
} while (0)

#define CMAC_AES_CALCULATE_MAC(cipher) \
do { \
    /* Make a new context, calculate hash, store to output, clear memory. */ \
    cipher##CmacContext ctx; \
    cmac##cipher##ContextCreate(&ctx, key); \
    cmac##cipher##ContextUpdate(&ctx, src, size); \
    cmac##cipher##ContextGetMac(&ctx, dst); \
    memset(&ctx, 0, sizeof(ctx)); \
} while (0)

/* Multiplies block in galois field. */
static inline uint8x16_t _galoisMultiply(const uint8x16_t val) {
    uint8x16_t mult;
    uint64_t high, low, mask;
    const uint64_t xor = 0x87ul;
    /* Use ASM. TODO: Better than using intrinsics? */
    __asm__ __volatile__ (
        "mov %[high], %[val].d[0]\n"
        "mov %[low], %[val].d[1]\n"
        "rev %[high], %[high]\n"
        "rev %[low], %[low]\n"
        "and %[mask], %[xor], %[high], asr 63\n"
        "extr %[high], %[high], %[low], 63\n"
        "eor %[low], %[mask], %[low], lsl 1\n"
        "rev %[high], %[high]\n"
        "rev %[low], %[low]\n"
        "mov %[mult].d[0], %[high]\n"
        "mov %[mult].d[1], %[low]\n"
        : [mult]"=w"(mult),
          [high]"=&r"(high), [low]"=&r"(low), [mask]"=&r"(mask)
        : [val]"w"(val),
          [xor]"r"(xor)
        : "cc"
    );
    return mult;
}

static void _cmacAes128ProcessBlocks(Aes128CmacContext *ctx, const u8 *src_u8, size_t num_blocks) {
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
    uint8x16_t cur_mac = vld1q_u8(ctx->mac);

    /* Process blocks one-by-one. */
    while (num_blocks >= 1) {
        /* Read block in, xor with MAC. */
        uint8x16_t tmp0 = veorq_u8(cur_mac, vld1q_u8(src_u8));
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

        /* Update MAC. */
        cur_mac = tmp0;

        num_blocks--;
    }

    vst1q_u8(ctx->mac, cur_mac);
}

static void _cmacAes192ProcessBlocks(Aes192CmacContext *ctx, const u8 *src_u8, size_t num_blocks) {
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
    uint8x16_t cur_mac = vld1q_u8(ctx->mac);

    /* Process blocks one-by-one. */
    while (num_blocks >= 1) {
        /* Read block in, xor with MAC. */
        uint8x16_t tmp0 = veorq_u8(cur_mac, vld1q_u8(src_u8));
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

        /* Update MAC. */
        cur_mac = tmp0;

        num_blocks--;
    }

    vst1q_u8(ctx->mac, cur_mac);
}

static void _cmacAes256ProcessBlocks(Aes256CmacContext *ctx, const u8 *src_u8, size_t num_blocks) {
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
    uint8x16_t cur_mac = vld1q_u8(ctx->mac);

    /* Process blocks one-by-one. */
    while (num_blocks >= 1) {
        /* Read block in, xor with MAC. */
        uint8x16_t tmp0 = veorq_u8(cur_mac, vld1q_u8(src_u8));
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

        /* Update MAC. */
        cur_mac = tmp0;

        num_blocks--;
    }

    vst1q_u8(ctx->mac, cur_mac);
}

void cmacAes128ContextCreate(Aes128CmacContext *out, const void *key) {
    CMAC_CONTEXT_CREATE(aes128);
}

void cmacAes128ContextUpdate(Aes128CmacContext *ctx, const void *src, size_t size) {
    CMAC_CONTEXT_UPDATE(Aes128);
}

void cmacAes128ContextGetMac(Aes128CmacContext *ctx, void *dst) {
    CMAC_CONTEXT_GET_MAC(Aes128);
}

void cmacAes128CalculateMac(void *dst, const void *key, const void *src, size_t size) {
    CMAC_AES_CALCULATE_MAC(Aes128);
}

void cmacAes192ContextCreate(Aes192CmacContext *out, const void *key) {
    CMAC_CONTEXT_CREATE(aes192);
}

void cmacAes192ContextUpdate(Aes192CmacContext *ctx, const void *src, size_t size) {
    CMAC_CONTEXT_UPDATE(Aes192);
}

void cmacAes192ContextGetMac(Aes192CmacContext *ctx, void *dst) {
    CMAC_CONTEXT_GET_MAC(Aes192);
}

void cmacAes192CalculateMac(void *dst, const void *key, const void *src, size_t size) {
    CMAC_AES_CALCULATE_MAC(Aes192);
}

void cmacAes256ContextCreate(Aes256CmacContext *out, const void *key) {
    CMAC_CONTEXT_CREATE(aes256);
}

void cmacAes256ContextUpdate(Aes256CmacContext *ctx, const void *src, size_t size) {
    CMAC_CONTEXT_UPDATE(Aes256);
}

void cmacAes256ContextGetMac(Aes256CmacContext *ctx, void *dst) {
    CMAC_CONTEXT_GET_MAC(Aes256);
}

void cmacAes256CalculateMac(void *dst, const void *key, const void *src, size_t size) {
    CMAC_AES_CALCULATE_MAC(Aes256);
}
