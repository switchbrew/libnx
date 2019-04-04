#include <string.h>
#include <stdlib.h>

#include "crypto/hmac.h"

#define HMAC_IPAD_VAL 0x36363636
#define HMAC_OPAD_VAL 0x5c5c5c5c
#define HMAC_IPAD_XOR_OPAD_VAL (HMAC_IPAD_VAL ^ HMAC_OPAD_VAL)

void hmacSha256ContextCreate(HmacSha256Context *out, const void *key, size_t key_size) {
    /* Clear key. */
    memset(out->key, 0, sizeof(out->key));

    /* Either hash the key into the context, or copy it directly if possible. */
    if (key_size <= sizeof(out->key)) {
        memcpy(out->key, key, key_size);
    } else {
        sha256ContextCreate(&out->sha_ctx);
        sha256ContextUpdate(&out->sha_ctx, key, key_size);
        sha256ContextGetHash(&out->sha_ctx, out->key);
    }

    /* XOR key with IPAD. */
    for (size_t i = 0; i < sizeof(out->key) / sizeof(u32); i++) {
        out->key[i] ^= HMAC_IPAD_VAL;
    }

    /* Update hash context with key ^ ipad. */
    sha256ContextCreate(&out->sha_ctx);
    sha256ContextUpdate(&out->sha_ctx, out->key, sizeof(out->key));
}

void hmacSha256ContextUpdate(HmacSha256Context *ctx, const void *src, size_t size) {
    /* Just update, since we want H((key ^ ipad) || data). */
    sha256ContextUpdate(&ctx->sha_ctx, src, size);
}

void hmacSha256ContextGetMac(HmacSha256Context *ctx, void *dst) {
    if (!ctx->finalized) {
        /* Save H((key ^ ipad) || data) into mac. */
        sha256ContextGetHash(&ctx->sha_ctx, ctx->mac);

        /* We want key ^ opad, so we xor (key ^ ipad) with (ipad ^ opad). */
        for (size_t i = 0; i < sizeof(ctx->key) / sizeof(u32); i++) {
            ctx->key[i] ^= HMAC_IPAD_XOR_OPAD_VAL;
        }

        /* Calculate H((key ^ opad) || H((key ^ ipad) || data)). */
        sha256ContextCreate(&ctx->sha_ctx);
        sha256ContextUpdate(&ctx->sha_ctx, ctx->key, sizeof(ctx->key));
        sha256ContextUpdate(&ctx->sha_ctx, ctx->mac, sizeof(ctx->mac));
        sha256ContextGetHash(&ctx->sha_ctx, ctx->mac);

        /* We're done. */
        ctx->finalized = true;
    }

    memcpy(dst, ctx->mac, sizeof(ctx->mac));
}

void hmacSha256CalculateMac(void *dst, const void *key, size_t key_size, const void *src, size_t size) {
    /* Make a new context, calculate hash, store to output, clear memory. */
    HmacSha256Context ctx;
    hmacSha256ContextCreate(&ctx, key, key_size);
    hmacSha256ContextUpdate(&ctx, src, size);
    hmacSha256ContextGetMac(&ctx, dst);
    memset(&ctx, 0, sizeof(ctx));
}
