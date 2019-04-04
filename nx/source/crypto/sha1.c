#include <string.h>
#include <stdlib.h>
#include <arm_neon.h>

#include "crypto/sha1.h"

/* Define for doing four rounds of SHA1. */
#define SHA1_DO_ROUND(insn, constant) \
const u32 a = vgetq_lane_u32(cur_abcd, 0); \
cur_abcd = v##insn##q_u32(cur_abcd, cur_e, vaddq_u32(w[i], constant)); \
cur_e = vsha1h_u32(a)

static const u32 s_roundConstants[4] = {
    0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6
};

void sha1ContextCreate(Sha1Context *out) {
    static const u32 H_0[SHA1_HASH_SIZE / sizeof(u32)] = {
        0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0
    };

    memcpy(out->intermediate_hash, H_0, sizeof(out->intermediate_hash));
    memset(out->buffer, 0, sizeof(out->buffer));
    out->bits_consumed = 0;
    out->num_buffered = 0;
    out->finalized = false;
}

static void _sha1ProcessBlocks(Sha1Context *ctx, const u8 *src_u8, size_t num_blocks) {
    /* Setup round constants. */
    const uint32x4_t k0 = vdupq_n_u32(s_roundConstants[0]);
    const uint32x4_t k1 = vdupq_n_u32(s_roundConstants[1]);
    const uint32x4_t k2 = vdupq_n_u32(s_roundConstants[2]);
    const uint32x4_t k3 = vdupq_n_u32(s_roundConstants[3]);

    /* Load hash variables with intermediate state. */
    uint32x4_t cur_abcd = vld1q_u32(ctx->intermediate_hash + 0);
    u32 cur_e = ctx->intermediate_hash[4];

    /* Actually do hash processing blocks. */
    while (num_blocks > 0) {
        /* Save current state. */
        const uint32x4_t prev_abcd = cur_abcd;
        const u32 prev_e = cur_e;

        uint32x4_t w[20];

        /* Setup w[0-3] with message. */
        for (size_t i = 0; i < 4; i++) {
            w[i] = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(src_u8)));
            src_u8 += 0x10;
        }

        /* Calculate w[4-19], w[i] = sha1su1(sha1su0(w[i-4], w[i-3], w[i-2]), w[i-1]); */
        for (size_t i = 4; i < 20; i++) {
            w[i] = vsha1su1q_u32(vsha1su0q_u32(w[i-4], w[i-3], w[i-2]), w[i-1]);
        }

        /* Do round calculations 0-20. Uses sha1c, k0. */
        size_t i = 0;
        while (i < 5) {
            SHA1_DO_ROUND(sha1c, k0);
            i++;
        }

        /* Do round calculations 20-40. Uses sha1p, k1. */
        while (i < 10) {
            SHA1_DO_ROUND(sha1p, k1);
            i++;
        }

        /* Do round calculations 40-60. Uses sha1m, k2. */
        while (i < 15) {
            SHA1_DO_ROUND(sha1m, k2);
            i++;
        }

        /* Do round calculations 60-80. Uses sha1p, k3. */
        while (i < 20) {
            SHA1_DO_ROUND(sha1p, k3);
            i++;
        }

        /* Add to previous. */
        cur_abcd = vaddq_u32(cur_abcd, prev_abcd);
        cur_e = cur_e + prev_e;

        num_blocks--;
    }

    /* Save result to intermediate hash. */
    vst1q_u32(ctx->intermediate_hash, cur_abcd);
    ctx->intermediate_hash[4] = cur_e;
}

void sha1ContextUpdate(Sha1Context *ctx, const void *src, size_t size) {
    /* Convert src to u8* for utility. */
    const u8 *cur_src = (const u8 *)src;

    /* Update bits consumed. */
    ctx->bits_consumed += (((ctx->num_buffered + size) / SHA1_BLOCK_SIZE) * SHA1_BLOCK_SIZE) * 8;

    /* Handle pre-buffered data. */
    if (ctx->num_buffered > 0) {
        const size_t needed = SHA1_BLOCK_SIZE - ctx->num_buffered;
        const size_t copyable = (size > needed ? needed : size);
        memcpy(&ctx->buffer[ctx->num_buffered], cur_src, copyable);
        cur_src += copyable;
        ctx->num_buffered += copyable;
        size -= copyable;

        if (ctx->num_buffered == SHA1_BLOCK_SIZE) {
            _sha1ProcessBlocks(ctx, ctx->buffer, 1);
            ctx->num_buffered = 0;
        }
    }

    /* Handle complete blocks. */
    if (size >= SHA1_BLOCK_SIZE) {
        const size_t num_blocks = size / SHA1_BLOCK_SIZE;
        _sha1ProcessBlocks(ctx, cur_src, num_blocks);
        size -= SHA1_BLOCK_SIZE * num_blocks;
        cur_src += SHA1_BLOCK_SIZE * num_blocks;
    }

    /* Buffer remaining data. */
    if (size > 0) {
        memcpy(ctx->buffer, cur_src, size);
        ctx->num_buffered = size;
    }
}

void sha1ContextGetHash(Sha1Context *ctx, void *dst) {
    if (!ctx->finalized) {
        /* Process last block, if necessary. */
        {
            ctx->bits_consumed += 8 * ctx->num_buffered;
            ctx->buffer[ctx->num_buffered++] = 0x80;

            const size_t last_block_max_size = SHA1_BLOCK_SIZE - sizeof(u64);
            /* If we've got space for the bits consumed field, just set to zero. */
            if (ctx->num_buffered <= last_block_max_size) {
                memset(ctx->buffer + ctx->num_buffered, 0, last_block_max_size - ctx->num_buffered);
            } else {
                /* Pad with zeroes, and process. */
                memset(ctx->buffer + ctx->num_buffered, 0, SHA1_BLOCK_SIZE - ctx->num_buffered);
                _sha1ProcessBlocks(ctx, ctx->buffer, 1);

                /* Clear the rest of the buffer with zeroes. */
                memset(ctx->buffer, 0, last_block_max_size);
            }

            /* Copy in bits consumed field, then process last block. */
            u64 big_endian_bits_consumed = __builtin_bswap64(ctx->bits_consumed);
            memcpy(ctx->buffer + last_block_max_size, &big_endian_bits_consumed, sizeof(big_endian_bits_consumed));
            _sha1ProcessBlocks(ctx, ctx->buffer, 1);
        }
        ctx->finalized = true;
    }

    /* Copy endian-swapped intermediate hash out. */
    u32 *dst_u32 = (u32 *)dst;
    for (size_t i = 0; i < sizeof(ctx->intermediate_hash) / sizeof(u32); i++) {
        dst_u32[i] = __builtin_bswap32(ctx->intermediate_hash[i]);
    }
}

void sha1CalculateHash(void *dst, const void *src, size_t size) {
    /* Make a new context, calculate hash, store to output. */
    Sha1Context ctx;
    sha1ContextCreate(&ctx);
    sha1ContextUpdate(&ctx, src, size);
    sha1ContextGetHash(&ctx, dst);
}
