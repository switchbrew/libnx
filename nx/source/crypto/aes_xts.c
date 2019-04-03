#include <string.h>
#include <stdlib.h>
#include <arm_neon.h>

#include "result.h"
#include "crypto/aes.h"
#include "crypto/aes_xts.h"

/* Variable management macros. */
#define DECLARE_ROUND_KEY_VAR(n) \
const uint8x16_t round_key_##n = vld1q_u8(ctx->aes_ctx.round_keys[n])

#define AES_ENC_DEC_OUTPUT_THREE_BLOCKS() \
[tmp0]"+w"(tmp0), [tmp1]"+w"(tmp1), [tmp2]"+w"(tmp2)

#define AES_ENC_DEC_OUTPUT_THREE_TWEAKS() \
[tweak0]"+w"(tweak0), [tweak1]"+w"(tweak1), [tweak2]"+w"(tweak2)

#define AES_ENC_DEC_OUTPUT_ONE_BLOCK() \
[tmp0]"+w"(tmp0)

#define AES_ENC_DEC_OUTPUT_ONE_TWEAK() \
[tweak0]"+w"(tweak0)

#define XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK() \
[high]"=&r"(high), [low]"=&r"(low), [mask]"=&r"(mask)

#define XTS_INCREMENT_INPUT_XOR() \
[xor]"r"(xor)

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

static inline uint8x16_t _multiplyTweak(const uint8x16_t tweak) {
    uint8x16_t mult;
    uint64_t high, low, mask;
    const uint64_t xor = 0x87ul;
    /* Use ASM. TODO: Better than using intrinsics? */
    __asm__ __volatile__ (
        "mov %[high], %[tweak].d[1]\n"
        "mov %[low], %[tweak].d[0]\n"
        "and %[mask], %[xor], %[high], asr 63\n"
        "extr %[high], %[high], %[low], 63\n"
        "eor %[low], %[mask], %[low], lsl 1\n"
        "mov %[mult].d[1], %[high]\n"
        "mov %[mult].d[0], %[low]\n"
        : [mult]"=w"(mult),
          XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
        : [tweak]"w"(tweak),
          XTS_INCREMENT_INPUT_XOR()
        : "cc"
    );
    return mult;
}

void aes128XtsContextCreate(Aes128XtsContext *out, const void *key0, const void *key1, bool is_encryptor) {
    /* Initialize inner context. */
    aes128ContextCreate(&out->aes_ctx, key0, is_encryptor);
    aes128ContextCreate(&out->tweak_ctx, key1, true);
    aes128XtsContextResetSector(out, 0, false);
}

void aes128XtsContextResetTweak(Aes128XtsContext *ctx, const void *tweak) {
    /* Set and encrypt tweak, nothing is buffered. */
    memcpy(ctx->tweak, tweak, sizeof(ctx->tweak));
    aes128EncryptBlock(&ctx->tweak_ctx, ctx->tweak, ctx->tweak);
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
    ctx->num_buffered = 0;
}

void aes128XtsContextResetSector(Aes128XtsContext *ctx, uint64_t sector, bool is_nintendo) {
    /* Set and encrypt tweak, nothing is buffered. */
    uint64_t *tweak_u64 = (uint64_t *)(&ctx->tweak);
    if (is_nintendo) {
        /* Nintendo uses big endian tweak-from-sector, despite little endian gf multiplication. */
        /* This is probably a Nintendo bug, but given all their content relies on it, not like it can change... */
        tweak_u64[0] = 0;
        tweak_u64[1] = __builtin_bswap64(sector);
    } else {
        /* Tweaks are normally little endian. */
        tweak_u64[0] = sector;
        tweak_u64[1] = 0;
    }
    aes128EncryptBlock(&ctx->tweak_ctx, ctx->tweak, ctx->tweak);
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
    ctx->num_buffered = 0;
}

static inline void _aes128XtsEncryptBlocks(Aes128XtsContext *ctx, u8 *dst_u8, const u8 *src_u8, size_t num_blocks) {
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
    uint8x16_t tweak0 = vld1q_u8(ctx->tweak);
    const uint64_t xor = 0x87ul;
    uint64_t high, low, mask;

    /* Process three blocks at a time, when possible. */
    if (num_blocks >= 3) {
        /* Multiply tweak twice. */
        uint8x16_t tweak1 = _multiplyTweak(tweak0);
        uint8x16_t tweak2 = _multiplyTweak(tweak1);

        while (num_blocks >= 3) {
            /* Save tweaks for xor usage. */
            const uint8x16_t mask0 = tweak0;
            const uint8x16_t mask1 = tweak1;
            const uint8x16_t mask2 = tweak2;

            /* Read blocks in, XOR with tweaks. */
            uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;
            uint8x16_t tmp1 = veorq_u8(mask1, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;
            uint8x16_t tmp2 = veorq_u8(mask2, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;

            /* Actually do encryption, use optimized asm. */
            /* Interleave GF mult calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_ENC_ROUND(0, 0) "mov %[high], %[tweak2].d[1]\n"
                AES_ENC_ROUND(0, 1) "mov %[low], %[tweak2].d[0]\n"
                AES_ENC_ROUND(0, 2) "and %[mask], %[xor], %[high], asr 63\n"
                AES_ENC_ROUND(1, 0) "extr %[high], %[high], %[low], 63\n"
                AES_ENC_ROUND(1, 1) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_ENC_ROUND(1, 2) "mov %[tweak0].d[1], %[high]\n"
                AES_ENC_ROUND(2, 0) "mov %[tweak0].d[0], %[low]\n"
                AES_ENC_ROUND(2, 1) "and %[mask], %[xor], %[high], asr 63\n"
                AES_ENC_ROUND(2, 2) "extr %[high], %[high], %[low], 63\n"
                AES_ENC_ROUND(3, 0) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_ENC_ROUND(3, 1) "mov %[tweak1].d[1], %[high]\n"
                AES_ENC_ROUND(3, 2) "mov %[tweak1].d[0], %[low]\n"
                AES_ENC_ROUND(4, 0) "and %[mask], %[xor], %[high], asr 63\n"
                AES_ENC_ROUND(4, 1) "extr %[high], %[high], %[low], 63\n"
                AES_ENC_ROUND(4, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_ENC_ROUND(5, 0) "mov %[tweak2].d[1], %[high]\n"
                AES_ENC_ROUND(5, 1) "mov %[tweak2].d[0], %[low]\n"
                AES_ENC_ROUND(5, 2)
                AES_ENC_ROUND(6, 0) AES_ENC_ROUND(6, 1) AES_ENC_ROUND(6, 2)
                AES_ENC_ROUND(7, 0) AES_ENC_ROUND(7, 1) AES_ENC_ROUND(7, 2)
                AES_ENC_ROUND(8, 0) AES_ENC_ROUND(8, 1) AES_ENC_ROUND(8, 2)
                AES_ENC_SECOND_LAST_ROUND(9, 0) AES_ENC_SECOND_LAST_ROUND(9, 1) AES_ENC_SECOND_LAST_ROUND(9, 2)
                AES_ENC_LAST_ROUND(10, 0) AES_ENC_LAST_ROUND(10, 1) AES_ENC_LAST_ROUND(10, 2)
                : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                  AES_ENC_DEC_OUTPUT_THREE_TWEAKS(),
                  XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                : XTS_INCREMENT_INPUT_XOR(),
                  AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
                : "cc"
            );

            /* XOR blocks. */
            tmp0 = veorq_u8(mask0, tmp0);
            tmp1 = veorq_u8(mask1, tmp1);
            tmp2 = veorq_u8(mask2, tmp2);

            /* Store to output. */
            vst1q_u8(dst_u8, tmp0);
            dst_u8 += AES_BLOCK_SIZE;
            vst1q_u8(dst_u8, tmp1);
            dst_u8 += AES_BLOCK_SIZE;
            vst1q_u8(dst_u8, tmp2);
            dst_u8 += AES_BLOCK_SIZE;

            num_blocks -= 3;
        }
    }

    while (num_blocks >= 1) {
        /* Save tweak for xor usage. */
        const uint8x16_t mask0 = tweak0;

        /* Read block in, XOR with tweak. */
        uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src_u8));
        src_u8 += AES_BLOCK_SIZE;

        /* Actually do encryption, use optimized asm. */
        /* Interleave CTR calculations with AES ones, to mask latencies. */
        __asm__ __volatile__ (
            AES_ENC_ROUND(0, 0) "mov %[high], %[tweak0].d[1]\n"
            AES_ENC_ROUND(1, 0) "mov %[low], %[tweak0].d[0]\n"
            AES_ENC_ROUND(2, 0) "and %[mask], %[xor], %[high], asr 63\n"
            AES_ENC_ROUND(3, 0) "extr %[high], %[high], %[low], 63\n"
            AES_ENC_ROUND(4, 0) "eor %[low], %[mask], %[low], lsl 1\n"
            AES_ENC_ROUND(5, 0) "mov %[tweak0].d[1], %[high]\n"
            AES_ENC_ROUND(6, 0) "mov %[tweak0].d[0], %[low]\n"
            AES_ENC_ROUND(7, 0)
            AES_ENC_ROUND(8, 0)
            AES_ENC_SECOND_LAST_ROUND(9, 0)
            AES_ENC_LAST_ROUND(10, 0)
            : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
              AES_ENC_DEC_OUTPUT_ONE_TWEAK(),
              XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
            : XTS_INCREMENT_INPUT_XOR(),
              AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
            : "cc"
        );

        /* XOR blocks. */
        tmp0 = veorq_u8(mask0, tmp0);

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks--;
    }

    vst1q_u8(ctx->tweak, tweak0);
}

size_t aes128XtsEncrypt(Aes128XtsContext *ctx, void *dst, const void *src, size_t size) {
    CRYPT_FUNC_BODY(_aes128XtsEncryptBlocks);
}

static inline void _aes128XtsDecryptBlocks(Aes128XtsContext *ctx, u8 *dst_u8, const u8 *src_u8, size_t num_blocks) {
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
    uint8x16_t tweak0 = vld1q_u8(ctx->tweak);
    const uint64_t xor = 0x87ul;
    uint64_t high, low, mask;

    /* Process three blocks at a time, when possible. */
    if (num_blocks >= 3) {
        /* Multiply tweak twice. */
        uint8x16_t tweak1 = _multiplyTweak(tweak0);
        uint8x16_t tweak2 = _multiplyTweak(tweak1);

        while (num_blocks >= 3) {
            /* Save tweaks for xor usage. */
            const uint8x16_t mask0 = tweak0;
            const uint8x16_t mask1 = tweak1;
            const uint8x16_t mask2 = tweak2;

            /* Read blocks in, XOR with tweaks. */
            uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;
            uint8x16_t tmp1 = veorq_u8(mask1, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;
            uint8x16_t tmp2 = veorq_u8(mask2, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;

            /* Actually do encryption, use optimized asm. */
            /* Interleave GF mult calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_DEC_ROUND(10, 0) "mov %[high], %[tweak2].d[1]\n"
                AES_DEC_ROUND(10, 1) "mov %[low], %[tweak2].d[0]\n"
                AES_DEC_ROUND(10, 2) "and %[mask], %[xor], %[high], asr 63\n"
                AES_DEC_ROUND(9, 0)  "extr %[high], %[high], %[low], 63\n"
                AES_DEC_ROUND(9, 1)  "eor %[low], %[mask], %[low], lsl 1\n"
                AES_DEC_ROUND(9, 2)  "mov %[tweak0].d[1], %[high]\n"
                AES_DEC_ROUND(8, 0)  "mov %[tweak0].d[0], %[low]\n"
                AES_DEC_ROUND(8, 1)  "and %[mask], %[xor], %[high], asr 63\n"
                AES_DEC_ROUND(8, 2)  "extr %[high], %[high], %[low], 63\n"
                AES_DEC_ROUND(7, 0)  "eor %[low], %[mask], %[low], lsl 1\n"
                AES_DEC_ROUND(7, 1)  "mov %[tweak1].d[1], %[high]\n"
                AES_DEC_ROUND(7, 2)  "mov %[tweak1].d[0], %[low]\n"
                AES_DEC_ROUND(6, 0)  "and %[mask], %[xor], %[high], asr 63\n"
                AES_DEC_ROUND(6, 1)  "extr %[high], %[high], %[low], 63\n"
                AES_DEC_ROUND(6, 2)  "eor %[low], %[mask], %[low], lsl 1\n"
                AES_DEC_ROUND(5, 0)  "mov %[tweak2].d[1], %[high]\n"
                AES_DEC_ROUND(5, 1)  "mov %[tweak2].d[0], %[low]\n"
                AES_DEC_ROUND(5, 2)
                AES_DEC_ROUND(4, 0) AES_DEC_ROUND(4, 1) AES_DEC_ROUND(4, 2)
                AES_DEC_ROUND(3, 0) AES_DEC_ROUND(3, 1) AES_DEC_ROUND(3, 2)
                AES_DEC_ROUND(2, 0) AES_DEC_ROUND(2, 1) AES_DEC_ROUND(2, 2)
                AES_DEC_SECOND_LAST_ROUND(1, 0) AES_DEC_SECOND_LAST_ROUND(1, 1) AES_DEC_SECOND_LAST_ROUND(1, 2)
                AES_DEC_LAST_ROUND(0, 0) AES_DEC_LAST_ROUND(0, 1) AES_DEC_LAST_ROUND(0, 2)
                : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                  AES_ENC_DEC_OUTPUT_THREE_TWEAKS(),
                  XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                : XTS_INCREMENT_INPUT_XOR(),
                  AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
                : "cc"
            );

            /* XOR blocks. */
            tmp0 = veorq_u8(mask0, tmp0);
            tmp1 = veorq_u8(mask1, tmp1);
            tmp2 = veorq_u8(mask2, tmp2);

            /* Store to output. */
            vst1q_u8(dst_u8, tmp0);
            dst_u8 += AES_BLOCK_SIZE;
            vst1q_u8(dst_u8, tmp1);
            dst_u8 += AES_BLOCK_SIZE;
            vst1q_u8(dst_u8, tmp2);
            dst_u8 += AES_BLOCK_SIZE;

            num_blocks -= 3;
        }
    }

    while (num_blocks >= 1) {
        /* Save tweak for xor usage. */
        const uint8x16_t mask0 = tweak0;

        /* Read block in, XOR with tweak. */
        uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src_u8));
        src_u8 += AES_BLOCK_SIZE;

        /* Actually do encryption, use optimized asm. */
        /* Interleave CTR calculations with AES ones, to mask latencies. */
        __asm__ __volatile__ (
            AES_DEC_ROUND(10, 0) "mov %[high], %[tweak0].d[1]\n"
            AES_DEC_ROUND(9, 0)  "mov %[low], %[tweak0].d[0]\n"
            AES_DEC_ROUND(8, 0)  "and %[mask], %[xor], %[high], asr 63\n"
            AES_DEC_ROUND(7, 0)  "extr %[high], %[high], %[low], 63\n"
            AES_DEC_ROUND(6, 0)  "eor %[low], %[mask], %[low], lsl 1\n"
            AES_DEC_ROUND(5, 0)  "mov %[tweak0].d[1], %[high]\n"
            AES_DEC_ROUND(4, 0)  "mov %[tweak0].d[0], %[low]\n"
            AES_DEC_ROUND(3, 0)
            AES_DEC_ROUND(2, 0)
            AES_DEC_SECOND_LAST_ROUND(1, 0)
            AES_DEC_LAST_ROUND(0, 0)
            : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
              AES_ENC_DEC_OUTPUT_ONE_TWEAK(),
              XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
            : XTS_INCREMENT_INPUT_XOR(),
              AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
            : "cc"
        );

        /* XOR blocks. */
        tmp0 = veorq_u8(mask0, tmp0);

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks--;
    }

    vst1q_u8(ctx->tweak, tweak0);
}

size_t aes128XtsDecrypt(Aes128XtsContext *ctx, void *dst, const void *src, size_t size) {
    CRYPT_FUNC_BODY(_aes128XtsDecryptBlocks);
}

void aes192XtsContextCreate(Aes192XtsContext *out, const void *key0, const void *key1, bool is_encryptor) {
    /* Initialize inner context. */
    aes192ContextCreate(&out->aes_ctx, key0, is_encryptor);
    aes192ContextCreate(&out->tweak_ctx, key1, true);
    aes192XtsContextResetSector(out, 0, false);
}

void aes192XtsContextResetTweak(Aes192XtsContext *ctx, const void *tweak) {
    /* Set and encrypt tweak, nothing is buffered. */
    memcpy(ctx->tweak, tweak, sizeof(ctx->tweak));
    aes192EncryptBlock(&ctx->tweak_ctx, ctx->tweak, ctx->tweak);
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
    ctx->num_buffered = 0;
}

void aes192XtsContextResetSector(Aes192XtsContext *ctx, uint64_t sector, bool is_nintendo) {
    /* Set and encrypt tweak, nothing is buffered. */
    uint64_t *tweak_u64 = (uint64_t *)(&ctx->tweak);
    if (is_nintendo) {
        /* Nintendo uses big endian tweak-from-sector, despite little endian gf multiplication. */
        /* This is probably a Nintendo bug, but given all their content relies on it, not like it can change... */
        tweak_u64[0] = 0;
        tweak_u64[1] = __builtin_bswap64(sector);
    } else {
        /* Tweaks are normally little endian. */
        tweak_u64[0] = sector;
        tweak_u64[1] = 0;
    }
    aes192EncryptBlock(&ctx->tweak_ctx, ctx->tweak, ctx->tweak);
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
    ctx->num_buffered = 0;
}

static inline void _aes192XtsEncryptBlocks(Aes192XtsContext *ctx, u8 *dst_u8, const u8 *src_u8, size_t num_blocks) {
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
    uint8x16_t tweak0 = vld1q_u8(ctx->tweak);
    const uint64_t xor = 0x87ul;
    uint64_t high, low, mask;

    /* Process three blocks at a time, when possible. */
    if (num_blocks >= 3) {
        /* Multiply tweak twice. */
        uint8x16_t tweak1 = _multiplyTweak(tweak0);
        uint8x16_t tweak2 = _multiplyTweak(tweak1);

        while (num_blocks >= 3) {
            /* Save tweaks for xor usage. */
            const uint8x16_t mask0 = tweak0;
            const uint8x16_t mask1 = tweak1;
            const uint8x16_t mask2 = tweak2;

            /* Read blocks in, XOR with tweaks. */
            uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;
            uint8x16_t tmp1 = veorq_u8(mask1, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;
            uint8x16_t tmp2 = veorq_u8(mask2, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;

            /* Actually do encryption, use optimized asm. */
            /* Interleave GF mult calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_ENC_ROUND(0, 0) "mov %[high], %[tweak2].d[1]\n"
                AES_ENC_ROUND(0, 1) "mov %[low], %[tweak2].d[0]\n"
                AES_ENC_ROUND(0, 2) "and %[mask], %[xor], %[high], asr 63\n"
                AES_ENC_ROUND(1, 0) "extr %[high], %[high], %[low], 63\n"
                AES_ENC_ROUND(1, 1) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_ENC_ROUND(1, 2) "mov %[tweak0].d[1], %[high]\n"
                AES_ENC_ROUND(2, 0) "mov %[tweak0].d[0], %[low]\n"
                AES_ENC_ROUND(2, 1) "and %[mask], %[xor], %[high], asr 63\n"
                AES_ENC_ROUND(2, 2) "extr %[high], %[high], %[low], 63\n"
                AES_ENC_ROUND(3, 0) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_ENC_ROUND(3, 1) "mov %[tweak1].d[1], %[high]\n"
                AES_ENC_ROUND(3, 2) "mov %[tweak1].d[0], %[low]\n"
                AES_ENC_ROUND(4, 0) "and %[mask], %[xor], %[high], asr 63\n"
                AES_ENC_ROUND(4, 1) "extr %[high], %[high], %[low], 63\n"
                AES_ENC_ROUND(4, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_ENC_ROUND(5, 0) "mov %[tweak2].d[1], %[high]\n"
                AES_ENC_ROUND(5, 1) "mov %[tweak2].d[0], %[low]\n"
                AES_ENC_ROUND(5, 2)
                AES_ENC_ROUND(6, 0) AES_ENC_ROUND(6, 1) AES_ENC_ROUND(6, 2)
                AES_ENC_ROUND(7, 0) AES_ENC_ROUND(7, 1) AES_ENC_ROUND(7, 2)
                AES_ENC_ROUND(8, 0) AES_ENC_ROUND(8, 1) AES_ENC_ROUND(8, 2)
                AES_ENC_ROUND(9, 0) AES_ENC_ROUND(9, 1) AES_ENC_ROUND(9, 2)
                AES_ENC_ROUND(10, 0) AES_ENC_ROUND(10, 1) AES_ENC_ROUND(10, 2)
                AES_ENC_SECOND_LAST_ROUND(11, 0) AES_ENC_SECOND_LAST_ROUND(11, 1) AES_ENC_SECOND_LAST_ROUND(11, 2)
                AES_ENC_LAST_ROUND(12, 0) AES_ENC_LAST_ROUND(12, 1) AES_ENC_LAST_ROUND(12, 2)
                : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                  AES_ENC_DEC_OUTPUT_THREE_TWEAKS(),
                  XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                : XTS_INCREMENT_INPUT_XOR(),
                  AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
                : "cc"
            );

            /* XOR blocks. */
            tmp0 = veorq_u8(mask0, tmp0);
            tmp1 = veorq_u8(mask1, tmp1);
            tmp2 = veorq_u8(mask2, tmp2);

            /* Store to output. */
            vst1q_u8(dst_u8, tmp0);
            dst_u8 += AES_BLOCK_SIZE;
            vst1q_u8(dst_u8, tmp1);
            dst_u8 += AES_BLOCK_SIZE;
            vst1q_u8(dst_u8, tmp2);
            dst_u8 += AES_BLOCK_SIZE;

            num_blocks -= 3;
        }
    }

    while (num_blocks >= 1) {
        /* Save tweak for xor usage. */
        const uint8x16_t mask0 = tweak0;

        /* Read block in, XOR with tweak. */
        uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src_u8));
        src_u8 += AES_BLOCK_SIZE;

        /* Actually do encryption, use optimized asm. */
        /* Interleave CTR calculations with AES ones, to mask latencies. */
        __asm__ __volatile__ (
            AES_ENC_ROUND(0, 0) "mov %[high], %[tweak0].d[1]\n"
            AES_ENC_ROUND(1, 0) "mov %[low], %[tweak0].d[0]\n"
            AES_ENC_ROUND(2, 0) "and %[mask], %[xor], %[high], asr 63\n"
            AES_ENC_ROUND(3, 0) "extr %[high], %[high], %[low], 63\n"
            AES_ENC_ROUND(4, 0) "eor %[low], %[mask], %[low], lsl 1\n"
            AES_ENC_ROUND(5, 0) "mov %[tweak0].d[1], %[high]\n"
            AES_ENC_ROUND(6, 0) "mov %[tweak0].d[0], %[low]\n"
            AES_ENC_ROUND(7, 0)
            AES_ENC_ROUND(8, 0)
            AES_ENC_ROUND(9, 0)
            AES_ENC_ROUND(10, 0)
            AES_ENC_SECOND_LAST_ROUND(11, 0)
            AES_ENC_LAST_ROUND(12, 0)
            : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
              AES_ENC_DEC_OUTPUT_ONE_TWEAK(),
              XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
            : XTS_INCREMENT_INPUT_XOR(),
              AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
            : "cc"
        );

        /* XOR blocks. */
        tmp0 = veorq_u8(mask0, tmp0);

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks--;
    }

    vst1q_u8(ctx->tweak, tweak0);
}

size_t aes192XtsEncrypt(Aes192XtsContext *ctx, void *dst, const void *src, size_t size) {
    CRYPT_FUNC_BODY(_aes192XtsEncryptBlocks);
}

static inline void _aes192XtsDecryptBlocks(Aes192XtsContext *ctx, u8 *dst_u8, const u8 *src_u8, size_t num_blocks) {
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
    uint8x16_t tweak0 = vld1q_u8(ctx->tweak);
    const uint64_t xor = 0x87ul;
    uint64_t high, low, mask;

    /* Process three blocks at a time, when possible. */
    if (num_blocks >= 3) {
        /* Multiply tweak twice. */
        uint8x16_t tweak1 = _multiplyTweak(tweak0);
        uint8x16_t tweak2 = _multiplyTweak(tweak1);

        while (num_blocks >= 3) {
            /* Save tweaks for xor usage. */
            const uint8x16_t mask0 = tweak0;
            const uint8x16_t mask1 = tweak1;
            const uint8x16_t mask2 = tweak2;

            /* Read blocks in, XOR with tweaks. */
            uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;
            uint8x16_t tmp1 = veorq_u8(mask1, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;
            uint8x16_t tmp2 = veorq_u8(mask2, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;

            /* Actually do encryption, use optimized asm. */
            /* Interleave GF mult calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_DEC_ROUND(12, 0) "mov %[high], %[tweak2].d[1]\n"
                AES_DEC_ROUND(12, 1) "mov %[low], %[tweak2].d[0]\n"
                AES_DEC_ROUND(12, 2) "and %[mask], %[xor], %[high], asr 63\n"
                AES_DEC_ROUND(11, 0) "extr %[high], %[high], %[low], 63\n"
                AES_DEC_ROUND(11, 1) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_DEC_ROUND(11, 2) "mov %[tweak0].d[1], %[high]\n"
                AES_DEC_ROUND(10, 0) "mov %[tweak0].d[0], %[low]\n"
                AES_DEC_ROUND(10, 1) "and %[mask], %[xor], %[high], asr 63\n"
                AES_DEC_ROUND(10, 2) "extr %[high], %[high], %[low], 63\n"
                AES_DEC_ROUND(9, 0)  "eor %[low], %[mask], %[low], lsl 1\n"
                AES_DEC_ROUND(9, 1)  "mov %[tweak1].d[1], %[high]\n"
                AES_DEC_ROUND(9, 2)  "mov %[tweak1].d[0], %[low]\n"
                AES_DEC_ROUND(8, 0)  "and %[mask], %[xor], %[high], asr 63\n"
                AES_DEC_ROUND(8, 1)  "extr %[high], %[high], %[low], 63\n"
                AES_DEC_ROUND(8, 2)  "eor %[low], %[mask], %[low], lsl 1\n"
                AES_DEC_ROUND(7, 0)  "mov %[tweak2].d[1], %[high]\n"
                AES_DEC_ROUND(7, 1)  "mov %[tweak2].d[0], %[low]\n"
                AES_DEC_ROUND(7, 2)
                AES_DEC_ROUND(6, 0) AES_DEC_ROUND(6, 1) AES_DEC_ROUND(6, 2)
                AES_DEC_ROUND(5, 0) AES_DEC_ROUND(5, 1) AES_DEC_ROUND(5, 2)
                AES_DEC_ROUND(4, 0) AES_DEC_ROUND(4, 1) AES_DEC_ROUND(4, 2)
                AES_DEC_ROUND(3, 0) AES_DEC_ROUND(3, 1) AES_DEC_ROUND(3, 2)
                AES_DEC_ROUND(2, 0) AES_DEC_ROUND(2, 1) AES_DEC_ROUND(2, 2)
                AES_DEC_SECOND_LAST_ROUND(1, 0) AES_DEC_SECOND_LAST_ROUND(1, 1) AES_DEC_SECOND_LAST_ROUND(1, 2)
                AES_DEC_LAST_ROUND(0, 0) AES_DEC_LAST_ROUND(0, 1) AES_DEC_LAST_ROUND(0, 2)
                : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                  AES_ENC_DEC_OUTPUT_THREE_TWEAKS(),
                  XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
                : XTS_INCREMENT_INPUT_XOR(),
                  AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
                : "cc"
            );

            /* XOR blocks. */
            tmp0 = veorq_u8(mask0, tmp0);
            tmp1 = veorq_u8(mask1, tmp1);
            tmp2 = veorq_u8(mask2, tmp2);

            /* Store to output. */
            vst1q_u8(dst_u8, tmp0);
            dst_u8 += AES_BLOCK_SIZE;
            vst1q_u8(dst_u8, tmp1);
            dst_u8 += AES_BLOCK_SIZE;
            vst1q_u8(dst_u8, tmp2);
            dst_u8 += AES_BLOCK_SIZE;

            num_blocks -= 3;
        }
    }

    while (num_blocks >= 1) {
        /* Save tweak for xor usage. */
        const uint8x16_t mask0 = tweak0;

        /* Read block in, XOR with tweak. */
        uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src_u8));
        src_u8 += AES_BLOCK_SIZE;

        /* Actually do encryption, use optimized asm. */
        /* Interleave CTR calculations with AES ones, to mask latencies. */
        __asm__ __volatile__ (
            AES_DEC_ROUND(12, 0) "mov %[high], %[tweak0].d[1]\n"
            AES_DEC_ROUND(11, 0) "mov %[low], %[tweak0].d[0]\n"
            AES_DEC_ROUND(10, 0) "and %[mask], %[xor], %[high], asr 63\n"
            AES_DEC_ROUND(9, 0)  "extr %[high], %[high], %[low], 63\n"
            AES_DEC_ROUND(8, 0)  "eor %[low], %[mask], %[low], lsl 1\n"
            AES_DEC_ROUND(7, 0)  "mov %[tweak0].d[1], %[high]\n"
            AES_DEC_ROUND(6, 0)  "mov %[tweak0].d[0], %[low]\n"
            AES_DEC_ROUND(5, 0)
            AES_DEC_ROUND(4, 0)
            AES_DEC_ROUND(3, 0)
            AES_DEC_ROUND(2, 0)
            AES_DEC_SECOND_LAST_ROUND(1, 0)
            AES_DEC_LAST_ROUND(0, 0)
            : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
              AES_ENC_DEC_OUTPUT_ONE_TWEAK(),
              XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
            : XTS_INCREMENT_INPUT_XOR(),
              AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
            : "cc"
        );

        /* XOR blocks. */
        tmp0 = veorq_u8(mask0, tmp0);

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks--;
    }

    vst1q_u8(ctx->tweak, tweak0);
}

size_t aes192XtsDecrypt(Aes192XtsContext *ctx, void *dst, const void *src, size_t size) {
    CRYPT_FUNC_BODY(_aes192XtsDecryptBlocks);
}

void aes256XtsContextCreate(Aes256XtsContext *out, const void *key0, const void *key1, bool is_encryptor) {
    /* Initialize inner context. */
    aes256ContextCreate(&out->aes_ctx, key0, is_encryptor);
    aes256ContextCreate(&out->tweak_ctx, key1, true);
    aes256XtsContextResetSector(out, 0, false);
}

void aes256XtsContextResetTweak(Aes256XtsContext *ctx, const void *tweak) {
    /* Set and encrypt tweak, nothing is buffered. */
    memcpy(ctx->tweak, tweak, sizeof(ctx->tweak));
    aes256EncryptBlock(&ctx->tweak_ctx, ctx->tweak, ctx->tweak);
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
    ctx->num_buffered = 0;
}

void aes256XtsContextResetSector(Aes256XtsContext *ctx, uint64_t sector, bool is_nintendo) {
    /* Set and encrypt tweak, nothing is buffered. */
    uint64_t *tweak_u64 = (uint64_t *)(&ctx->tweak);
    if (is_nintendo) {
        /* Nintendo uses big endian tweak-from-sector, despite little endian gf multiplication. */
        /* This is probably a Nintendo bug, but given all their content relies on it, not like it can change... */
        tweak_u64[0] = 0;
        tweak_u64[1] = __builtin_bswap64(sector);
    } else {
        /* Tweaks are normally little endian. */
        tweak_u64[0] = sector;
        tweak_u64[1] = 0;
    }
    aes256EncryptBlock(&ctx->tweak_ctx, ctx->tweak, ctx->tweak);
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
    ctx->num_buffered = 0;
}

static inline void _aes256XtsEncryptBlocks(Aes256XtsContext *ctx, u8 *dst_u8, const u8 *src_u8, size_t num_blocks) {
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
    uint8x16_t tweak0 = vld1q_u8(ctx->tweak);
    const uint64_t xor = 0x87ul;
    uint64_t high, low, mask;

    /* Process three blocks at a time, when possible. */
    if (num_blocks >= 3) {
        /* Multiply tweak twice. */
        uint8x16_t tweak1 = _multiplyTweak(tweak0);
        uint8x16_t tweak2 = _multiplyTweak(tweak1);

        while (num_blocks >= 3) {
            /* Save tweaks for xor usage. */
            const uint8x16_t mask0 = tweak0;
            const uint8x16_t mask1 = tweak1;
            const uint8x16_t mask2 = tweak2;

            /* Read blocks in, XOR with tweaks. */
            uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;
            uint8x16_t tmp1 = veorq_u8(mask1, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;
            uint8x16_t tmp2 = veorq_u8(mask2, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;

            /* Actually do encryption, use optimized asm. */
            /* Interleave GF mult calculations with AES ones, to mask latencies. */
            /* Note: ASM here cannot use constant xor reg due to operand limitations. */
            __asm__ __volatile__ (
                AES_ENC_ROUND(0, 0) "mov %[high], %[tweak2].d[1]\n"
                AES_ENC_ROUND(0, 1) "mov %[low], %[tweak2].d[0]\n"
                AES_ENC_ROUND(0, 2) "mov %[mask], #0x87\n"
                AES_ENC_ROUND(1, 0) "and %[mask], %[mask], %[high], asr 63\n"
                AES_ENC_ROUND(1, 1) "extr %[high], %[high], %[low], 63\n"
                AES_ENC_ROUND(1, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_ENC_ROUND(2, 0) "mov %[tweak0].d[1], %[high]\n"
                AES_ENC_ROUND(2, 1) "mov %[tweak0].d[0], %[low]\n"
                AES_ENC_ROUND(2, 2) "mov %[mask], #0x87\n"
                AES_ENC_ROUND(3, 0) "and %[mask], %[mask], %[high], asr 63\n"
                AES_ENC_ROUND(3, 1) "extr %[high], %[high], %[low], 63\n"
                AES_ENC_ROUND(3, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_ENC_ROUND(4, 0) "mov %[tweak1].d[1], %[high]\n"
                AES_ENC_ROUND(4, 1) "mov %[tweak1].d[0], %[low]\n"
                AES_ENC_ROUND(4, 2) "mov %[mask], #0x87\n"
                AES_ENC_ROUND(5, 0) "and %[mask], %[mask], %[high], asr 63\n"
                AES_ENC_ROUND(5, 1) "extr %[high], %[high], %[low], 63\n"
                AES_ENC_ROUND(5, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_ENC_ROUND(6, 0) "mov %[tweak2].d[1], %[high]\n"
                AES_ENC_ROUND(6, 1) "mov %[tweak2].d[0], %[low]\n"
                AES_ENC_ROUND(6, 2)
                AES_ENC_ROUND(7, 0) AES_ENC_ROUND(7, 1) AES_ENC_ROUND(7, 2)
                AES_ENC_ROUND(8, 0) AES_ENC_ROUND(8, 1) AES_ENC_ROUND(8, 2)
                AES_ENC_ROUND(9, 0) AES_ENC_ROUND(9, 1) AES_ENC_ROUND(9, 2)
                AES_ENC_ROUND(10, 0) AES_ENC_ROUND(10, 1) AES_ENC_ROUND(10, 2)
                AES_ENC_ROUND(11, 0) AES_ENC_ROUND(11, 1) AES_ENC_ROUND(11, 2)
                AES_ENC_ROUND(12, 0) AES_ENC_ROUND(12, 1) AES_ENC_ROUND(12, 2)
                AES_ENC_SECOND_LAST_ROUND(13, 0) AES_ENC_SECOND_LAST_ROUND(13, 1) AES_ENC_SECOND_LAST_ROUND(13, 2)
                AES_ENC_LAST_ROUND(14, 0) AES_ENC_LAST_ROUND(14, 1) AES_ENC_LAST_ROUND(14, 2)
                : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                  AES_ENC_DEC_OUTPUT_THREE_TWEAKS(),
                  XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
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
                : "cc"
            );

            /* XOR blocks. */
            tmp0 = veorq_u8(mask0, tmp0);
            tmp1 = veorq_u8(mask1, tmp1);
            tmp2 = veorq_u8(mask2, tmp2);

            /* Store to output. */
            vst1q_u8(dst_u8, tmp0);
            dst_u8 += AES_BLOCK_SIZE;
            vst1q_u8(dst_u8, tmp1);
            dst_u8 += AES_BLOCK_SIZE;
            vst1q_u8(dst_u8, tmp2);
            dst_u8 += AES_BLOCK_SIZE;

            num_blocks -= 3;
        }
    }

    while (num_blocks >= 1) {
        /* Save tweak for xor usage. */
        const uint8x16_t mask0 = tweak0;

        /* Read block in, XOR with tweak. */
        uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src_u8));
        src_u8 += AES_BLOCK_SIZE;

        /* Actually do encryption, use optimized asm. */
        /* Interleave CTR calculations with AES ones, to mask latencies. */
        __asm__ __volatile__ (
            AES_ENC_ROUND(0, 0) "mov %[high], %[tweak0].d[1]\n"
            AES_ENC_ROUND(1, 0) "mov %[low], %[tweak0].d[0]\n"
            AES_ENC_ROUND(2, 0) "and %[mask], %[xor], %[high], asr 63\n"
            AES_ENC_ROUND(3, 0) "extr %[high], %[high], %[low], 63\n"
            AES_ENC_ROUND(4, 0) "eor %[low], %[mask], %[low], lsl 1\n"
            AES_ENC_ROUND(5, 0) "mov %[tweak0].d[1], %[high]\n"
            AES_ENC_ROUND(6, 0) "mov %[tweak0].d[0], %[low]\n"
            AES_ENC_ROUND(7, 0)
            AES_ENC_ROUND(8, 0)
            AES_ENC_ROUND(9, 0)
            AES_ENC_ROUND(10, 0)
            AES_ENC_ROUND(11, 0)
            AES_ENC_ROUND(12, 0)
            AES_ENC_SECOND_LAST_ROUND(13, 0)
            AES_ENC_LAST_ROUND(14, 0)
            : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
              AES_ENC_DEC_OUTPUT_ONE_TWEAK(),
              XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
            : XTS_INCREMENT_INPUT_XOR(),
              AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
            : "cc"
        );

        /* XOR blocks. */
        tmp0 = veorq_u8(mask0, tmp0);

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks--;
    }

    vst1q_u8(ctx->tweak, tweak0);
}

size_t aes256XtsEncrypt(Aes256XtsContext *ctx, void *dst, const void *src, size_t size) {
    CRYPT_FUNC_BODY(_aes256XtsEncryptBlocks);
}

static inline void _aes256XtsDecryptBlocks(Aes256XtsContext *ctx, u8 *dst_u8, const u8 *src_u8, size_t num_blocks) {
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
    uint8x16_t tweak0 = vld1q_u8(ctx->tweak);
    const uint64_t xor = 0x87ul;
    uint64_t high, low, mask;

    /* Process three blocks at a time, when possible. */
    if (num_blocks >= 3) {
        /* Multiply tweak twice. */
        uint8x16_t tweak1 = _multiplyTweak(tweak0);
        uint8x16_t tweak2 = _multiplyTweak(tweak1);

        while (num_blocks >= 3) {
            /* Save tweaks for xor usage. */
            const uint8x16_t mask0 = tweak0;
            const uint8x16_t mask1 = tweak1;
            const uint8x16_t mask2 = tweak2;

            /* Read blocks in, XOR with tweaks. */
            uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;
            uint8x16_t tmp1 = veorq_u8(mask1, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;
            uint8x16_t tmp2 = veorq_u8(mask2, vld1q_u8(src_u8));
            src_u8 += AES_BLOCK_SIZE;

            /* Actually do encryption, use optimized asm. */
            /* Interleave GF mult calculations with AES ones, to mask latencies. */
            __asm__ __volatile__ (
                AES_DEC_ROUND(14, 0) "mov %[high], %[tweak2].d[1]\n"
                AES_DEC_ROUND(14, 1) "mov %[low], %[tweak2].d[0]\n"
                AES_DEC_ROUND(14, 2) "mov %[mask], 0x87\n"
                AES_DEC_ROUND(13, 0) "and %[mask], %[mask], %[high], asr 63\n"
                AES_DEC_ROUND(13, 1) "extr %[high], %[high], %[low], 63\n"
                AES_DEC_ROUND(13, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_DEC_ROUND(12, 0) "mov %[tweak0].d[1], %[high]\n"
                AES_DEC_ROUND(12, 1) "mov %[tweak0].d[0], %[low]\n"
                AES_DEC_ROUND(12, 2) "mov %[mask], 0x87\n"
                AES_DEC_ROUND(11, 0) "and %[mask], %[mask], %[high], asr 63\n"
                AES_DEC_ROUND(11, 1) "extr %[high], %[high], %[low], 63\n"
                AES_DEC_ROUND(11, 2) "eor %[low], %[mask], %[low], lsl 1\n"
                AES_DEC_ROUND(10, 0) "mov %[tweak1].d[1], %[high]\n"
                AES_DEC_ROUND(10, 1) "mov %[tweak1].d[0], %[low]\n"
                AES_DEC_ROUND(10, 2) "mov %[mask], 0x87\n"
                AES_DEC_ROUND(9, 0)  "and %[mask], %[mask], %[high], asr 63\n"
                AES_DEC_ROUND(9, 1)  "extr %[high], %[high], %[low], 63\n"
                AES_DEC_ROUND(9, 2)  "eor %[low], %[mask], %[low], lsl 1\n"
                AES_DEC_ROUND(8, 0)  "mov %[tweak2].d[1], %[high]\n"
                AES_DEC_ROUND(8, 1)  "mov %[tweak2].d[0], %[low]\n"
                AES_DEC_ROUND(8, 2)
                AES_DEC_ROUND(7, 0) AES_DEC_ROUND(7, 1) AES_DEC_ROUND(7, 2)
                AES_DEC_ROUND(6, 0) AES_DEC_ROUND(6, 1) AES_DEC_ROUND(6, 2)
                AES_DEC_ROUND(5, 0) AES_DEC_ROUND(5, 1) AES_DEC_ROUND(5, 2)
                AES_DEC_ROUND(4, 0) AES_DEC_ROUND(4, 1) AES_DEC_ROUND(4, 2)
                AES_DEC_ROUND(3, 0) AES_DEC_ROUND(3, 1) AES_DEC_ROUND(3, 2)
                AES_DEC_ROUND(2, 0) AES_DEC_ROUND(2, 1) AES_DEC_ROUND(2, 2)
                AES_DEC_SECOND_LAST_ROUND(1, 0) AES_DEC_SECOND_LAST_ROUND(1, 1) AES_DEC_SECOND_LAST_ROUND(1, 2)
                AES_DEC_LAST_ROUND(0, 0) AES_DEC_LAST_ROUND(0, 1) AES_DEC_LAST_ROUND(0, 2)
                : AES_ENC_DEC_OUTPUT_THREE_BLOCKS(),
                  AES_ENC_DEC_OUTPUT_THREE_TWEAKS(),
                  XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
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
                : "cc"
            );

            /* XOR blocks. */
            tmp0 = veorq_u8(mask0, tmp0);
            tmp1 = veorq_u8(mask1, tmp1);
            tmp2 = veorq_u8(mask2, tmp2);

            /* Store to output. */
            vst1q_u8(dst_u8, tmp0);
            dst_u8 += AES_BLOCK_SIZE;
            vst1q_u8(dst_u8, tmp1);
            dst_u8 += AES_BLOCK_SIZE;
            vst1q_u8(dst_u8, tmp2);
            dst_u8 += AES_BLOCK_SIZE;

            num_blocks -= 3;
        }
    }

    while (num_blocks >= 1) {
        /* Save tweak for xor usage. */
        const uint8x16_t mask0 = tweak0;

        /* Read block in, XOR with tweak. */
        uint8x16_t tmp0 = veorq_u8(mask0, vld1q_u8(src_u8));
        src_u8 += AES_BLOCK_SIZE;

        /* Actually do encryption, use optimized asm. */
        /* Interleave CTR calculations with AES ones, to mask latencies. */
        __asm__ __volatile__ (
            AES_DEC_ROUND(14, 0) "mov %[high], %[tweak0].d[1]\n"
            AES_DEC_ROUND(13, 0) "mov %[low], %[tweak0].d[0]\n"
            AES_DEC_ROUND(12, 0) "and %[mask], %[xor], %[high], asr 63\n"
            AES_DEC_ROUND(11, 0) "extr %[high], %[high], %[low], 63\n"
            AES_DEC_ROUND(10, 0) "eor %[low], %[mask], %[low], lsl 1\n"
            AES_DEC_ROUND(9, 0)  "mov %[tweak0].d[1], %[high]\n"
            AES_DEC_ROUND(8, 0)  "mov %[tweak0].d[0], %[low]\n"
            AES_DEC_ROUND(7, 0)
            AES_DEC_ROUND(6, 0)
            AES_DEC_ROUND(5, 0)
            AES_DEC_ROUND(4, 0)
            AES_DEC_ROUND(3, 0)
            AES_DEC_ROUND(2, 0)
            AES_DEC_SECOND_LAST_ROUND(1, 0)
            AES_DEC_LAST_ROUND(0, 0)
            : AES_ENC_DEC_OUTPUT_ONE_BLOCK(),
              AES_ENC_DEC_OUTPUT_ONE_TWEAK(),
              XTS_INCREMENT_OUTPUT_HIGH_LOW_MASK()
            : XTS_INCREMENT_INPUT_XOR(),
              AES_ENC_DEC_INPUT_ROUND_KEY(0),
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
            : "cc"
        );

        /* XOR blocks. */
        tmp0 = veorq_u8(mask0, tmp0);

        /* Store to output. */
        vst1q_u8(dst_u8, tmp0);
        dst_u8 += AES_BLOCK_SIZE;

        num_blocks--;
    }

    vst1q_u8(ctx->tweak, tweak0);
}

size_t aes256XtsDecrypt(Aes256XtsContext *ctx, void *dst, const void *src, size_t size) {
    CRYPT_FUNC_BODY(_aes256XtsDecryptBlocks);
}
