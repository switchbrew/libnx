#include <string.h>
#include <stdlib.h>
#include <arm_neon.h>

#include "crypto/sha256.h"

alignas(SHA256_BLOCK_SIZE) static const u32 s_roundConstants[0x40] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

void sha256ContextCreate(Sha256Context *out) {
    static const u32 H_0[SHA256_HASH_SIZE / sizeof(u32)] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    memcpy(out->intermediate_hash, H_0, sizeof(out->intermediate_hash));
    memset(out->buffer, 0, sizeof(out->buffer));
    out->bits_consumed = 0;
    out->num_buffered = 0;
    out->finalized = false;
}

static void _sha256ProcessBlocks(Sha256Context *ctx, const u8 *src_u8, size_t num_blocks) {
    /* Load previous hash with intermediate state, current hash with zeroes. */
    uint32x4_t prev_hash0 = vld1q_u32(ctx->intermediate_hash + 0);
    uint32x4_t prev_hash1 = vld1q_u32(ctx->intermediate_hash + 4);
    uint32x4_t cur_hash0  = vdupq_n_u32(0);
    uint32x4_t cur_hash1  = vdupq_n_u32(0);

    /* Actually do hash processing blocks. */
    while (num_blocks > 0) {
        uint32x4_t round_constant0, round_constant1;
        uint32x4_t data0, data1, data2, data3;
        uint32x4_t tmp0, tmp1, tmp2, tmp3;
        uint32x4_t tmp_hash;

        /* Use optimized ASM implementation to process the block. */
        __asm__ __volatile__ (
            "ldp       %q[data0], %q[data1], [%[src_u8]], #0x20\n"
            "ldp       %q[data2], %q[data3], [%[src_u8]], #0x20\n"
            "add       %[cur_hash0].4s, %[cur_hash0].4s, %[prev_hash0].4s\n"
            "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0x00]\n"
            "add       %[cur_hash1].4s, %[cur_hash1].4s, %[prev_hash1].4s\n"
            "rev32     %[data0].16b, %[data0].16b\n"
            "rev32     %[data1].16b, %[data1].16b\n"
            "rev32     %[data2].16b, %[data2].16b\n"
            "rev32     %[data3].16b, %[data3].16b\n"
            "add       %[tmp0].4s, %[data0].4s, %[round_constant0].4s\n"
            "add       %[tmp1].4s, %[data1].4s, %[round_constant1].4s\n"
            "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0x20]\n"
            "sha256su0 %[data0].4s, %[data1].4s\n"
            "mov       %[prev_hash0].16b, %[cur_hash0].16b\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp0].4s\n"
            "mov       %[prev_hash1].16b, %[cur_hash1].16b\n"
            "sha256h2  %q[cur_hash1], %q[prev_hash0], %[tmp0].4s\n"
            "sha256su0 %[data1].4s, %[data2].4s\n"
            "sha256su1 %[data0].4s, %[data2].4s, %[data3].4s\n"
            "add       %[tmp2].4s, %[data2].4s, %[round_constant0].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp1].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp1].4s\n"
            "sha256su0 %[data2].4s, %[data3].4s\n"
            "sha256su1 %[data1].4s, %[data3].4s, %[data0].4s\n"
            "add       %[tmp3].4s, %[data3].4s, %[round_constant1].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0x40]\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp2].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp2].4s\n"
            "sha256su0 %[data3].4s, %[data0].4s\n"
            "sha256su1 %[data2].4s, %[data0].4s, %[data1].4s\n"
            "add       %[tmp0].4s, %[data0].4s, %[round_constant0].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp3].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp3].4s\n"
            "sha256su0 %[data0].4s, %[data1].4s\n"
            "sha256su1 %[data3].4s, %[data1].4s, %[data2].4s\n"
            "add       %[tmp1].4s, %[data1].4s, %[round_constant1].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0x60]\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp0].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp0].4s\n"
            "sha256su0 %[data1].4s, %[data2].4s\n"
            "sha256su1 %[data0].4s, %[data2].4s, %[data3].4s\n"
            "add       %[tmp2].4s, %[data2].4s, %[round_constant0].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp1].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp1].4s\n"
            "sha256su0 %[data2].4s, %[data3].4s\n"
            "sha256su1 %[data1].4s, %[data3].4s, %[data0].4s\n"
            "add       %[tmp3].4s, %[data3].4s, %[round_constant1].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0x80]\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp2].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp2].4s\n"
            "sha256su0 %[data3].4s, %[data0].4s\n"
            "sha256su1 %[data2].4s, %[data0].4s, %[data1].4s\n"
            "add       %[tmp0].4s, %[data0].4s, %[round_constant0].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp3].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp3].4s\n"
            "sha256su0 %[data0].4s, %[data1].4s\n"
            "sha256su1 %[data3].4s, %[data1].4s, %[data2].4s\n"
            "add       %[tmp1].4s, %[data1].4s, %[round_constant1].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0xA0]\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp0].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp0].4s\n"
            "sha256su0 %[data1].4s, %[data2].4s\n"
            "sha256su1 %[data0].4s, %[data2].4s, %[data3].4s\n"
            "add       %[tmp2].4s, %[data2].4s, %[round_constant0].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp1].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp1].4s\n"
            "sha256su0 %[data2].4s, %[data3].4s\n"
            "sha256su1 %[data1].4s, %[data3].4s, %[data0].4s\n"
            "add       %[tmp3].4s, %[data3].4s, %[round_constant1].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0xC0]\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp2].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp2].4s\n"
            "sha256su0 %[data3].4s, %[data0].4s\n"
            "sha256su1 %[data2].4s, %[data0].4s, %[data1].4s\n"
            "add       %[tmp0].4s, %[data0].4s, %[round_constant0].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp3].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp3].4s\n"
            "sha256su1 %[data3].4s, %[data1].4s, %[data2].4s\n"
            "add       %[tmp1].4s, %[data1].4s, %[round_constant1].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "ldp       %q[round_constant0], %q[round_constant1], [%[round_constants], 0xE0]\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp0].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp0].4s\n"
            "add       %[tmp2].4s, %[data2].4s, %[round_constant0].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp1].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp1].4s\n"
            "add       %[tmp3].4s, %[data3].4s, %[round_constant1].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp2].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp2].4s\n"
            "mov       %[tmp_hash].16b, %[cur_hash0].16b\n"
            "sha256h   %q[cur_hash0], %q[cur_hash1], %[tmp3].4s\n"
            "sha256h2  %q[cur_hash1], %q[tmp_hash], %[tmp3].4s\n"
            : [data0]"=w"(data0), [data1]"=w"(data1), [data2]"=w"(data2), [data3]"=w"(data3),
              [tmp0]"=w"(tmp0), [tmp1]"=w"(tmp1), [tmp2]"=w"(tmp2), [tmp3]"=w"(tmp3),
              [round_constant0]"=w"(round_constant0), [round_constant1]"=w"(round_constant1),
              [cur_hash0]"+w"(cur_hash0), [cur_hash1]"+w"(cur_hash1),
              [prev_hash0]"+w"(prev_hash0), [prev_hash1]"+w"(prev_hash1),
              [tmp_hash]"=w"(tmp_hash), [src_u8]"+r"(src_u8)
            : [round_constants]"r"(s_roundConstants)
            :
        );

        num_blocks--;
    }

    /* Add hashes together, and store. */
    cur_hash0 = vaddq_u32(prev_hash0, cur_hash0);
    cur_hash1 = vaddq_u32(prev_hash1, cur_hash1);
    vst1q_u32(ctx->intermediate_hash + 0, cur_hash0);
    vst1q_u32(ctx->intermediate_hash + 4, cur_hash1);
}

void sha256ContextUpdate(Sha256Context *ctx, const void *src, size_t size) {
    /* Convert src to u8* for utility. */
    const u8 *cur_src = (const u8 *)src;

    /* Update bits consumed. */
    ctx->bits_consumed += (((ctx->num_buffered + size) / SHA256_BLOCK_SIZE) * SHA256_BLOCK_SIZE) * 8;

    /* Handle pre-buffered data. */
    if (ctx->num_buffered > 0) {
        const size_t needed = SHA256_BLOCK_SIZE - ctx->num_buffered;
        const size_t copyable = (size > needed ? needed : size);
        memcpy(&ctx->buffer[ctx->num_buffered], cur_src, copyable);
        cur_src += copyable;
        ctx->num_buffered += copyable;
        size -= copyable;

        if (ctx->num_buffered == SHA256_BLOCK_SIZE) {
            _sha256ProcessBlocks(ctx, ctx->buffer, 1);
            ctx->num_buffered = 0;
        }
    }

    /* Handle complete blocks. */
    if (size >= SHA256_BLOCK_SIZE) {
        const size_t num_blocks = size / SHA256_BLOCK_SIZE;
        _sha256ProcessBlocks(ctx, cur_src, num_blocks);
        size -= SHA256_BLOCK_SIZE * num_blocks;
        cur_src += SHA256_BLOCK_SIZE * num_blocks;
    }

    /* Buffer remaining data. */
    if (size > 0) {
        memcpy(ctx->buffer, cur_src, size);
        ctx->num_buffered = size;
    }
}

void sha256ContextGetHash(Sha256Context *ctx, void *dst) {
    if (!ctx->finalized) {
        /* Process last block, if necessary. */
        {
            ctx->bits_consumed += 8 * ctx->num_buffered;
            ctx->buffer[ctx->num_buffered++] = 0x80;

            const size_t last_block_max_size = SHA256_BLOCK_SIZE - sizeof(u64);
            /* If we've got space for the bits consumed field, just set to zero. */
            if (ctx->num_buffered <= last_block_max_size) {
                memset(ctx->buffer + ctx->num_buffered, 0, last_block_max_size - ctx->num_buffered);
            } else {
                /* Pad with zeroes, and process. */
                memset(ctx->buffer + ctx->num_buffered, 0, SHA256_BLOCK_SIZE - ctx->num_buffered);
                _sha256ProcessBlocks(ctx, ctx->buffer, 1);

                /* Clear the rest of the buffer with zeroes. */
                memset(ctx->buffer, 0, last_block_max_size);
            }

            /* Copy in bits consumed field, then process last block. */
            u64 big_endian_bits_consumed = __builtin_bswap64(ctx->bits_consumed);
            memcpy(ctx->buffer + last_block_max_size, &big_endian_bits_consumed, sizeof(big_endian_bits_consumed));
            _sha256ProcessBlocks(ctx, ctx->buffer, 1);
        }
        ctx->finalized = true;
    }

    /* Copy endian-swapped intermediate hash out. */
    u32 *dst_u32 = (u32 *)dst;
    for (size_t i = 0; i < sizeof(ctx->intermediate_hash) / sizeof(u32); i++) {
        dst_u32[i] = __builtin_bswap32(ctx->intermediate_hash[i]);
    }
}

void sha256CalculateHash(void *dst, const void *src, size_t size) {
    /* Make a new context, calculate hash, store to output. */
    Sha256Context ctx;
    sha256ContextCreate(&ctx);
    sha256ContextUpdate(&ctx, src, size);
    sha256ContextGetHash(&ctx, dst);
}
