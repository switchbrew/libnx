#include <string.h>
#include <stdlib.h>

#include "crypto/hmac.h"

#define HMAC_IPAD_VAL 0x36363636
#define HMAC_OPAD_VAL 0x5c5c5c5c
#define HMAC_IPAD_XOR_OPAD_VAL (HMAC_IPAD_VAL ^ HMAC_OPAD_VAL)

/* Function bodies. */

#define HMAC_CONTEXT_CREATE(cipher) \
    /* Clear key. */ \
    memset(out->key, 0, sizeof(out->key)); \
    out->finalized = false; \
\
    /* Either hash the key into the context, or copy it directly if possible. */ \
    if (key_size <= sizeof(out->key)) { \
        memcpy(out->key, key, key_size); \
    } else { \
        cipher##ContextCreate(&out->sha_ctx); \
        cipher##ContextUpdate(&out->sha_ctx, key, key_size); \
        cipher##ContextGetHash(&out->sha_ctx, out->key); \
    } \
\
    /* XOR key with IPAD. */ \
    for (size_t i = 0; i < sizeof(out->key) / sizeof(u32); i++) { \
        out->key[i] ^= HMAC_IPAD_VAL; \
    } \
\
    /* Update hash context with key ^ ipad. */ \
    cipher##ContextCreate(&out->sha_ctx); \
    cipher##ContextUpdate(&out->sha_ctx, out->key, sizeof(out->key))

#define HMAC_CONTEXT_UPDATE(cipher) \
    /* Just update, since we want H((key ^ ipad) || data). */ \
    cipher##ContextUpdate(&ctx->sha_ctx, src, size)

#define HMAC_CONTEXT_GET_MAC(cipher) \
    if (!ctx->finalized) { \
        /* Save H((key ^ ipad) || data) into mac. */ \
        cipher##ContextGetHash(&ctx->sha_ctx, ctx->mac); \
\
        /* We want key ^ opad, so we xor (key ^ ipad) with (ipad ^ opad). */ \
        for (size_t i = 0; i < sizeof(ctx->key) / sizeof(u32); i++) { \
            ctx->key[i] ^= HMAC_IPAD_XOR_OPAD_VAL; \
        } \
\
        /* Calculate H((key ^ opad) || H((key ^ ipad) || data)). */ \
        cipher##ContextCreate(&ctx->sha_ctx); \
        cipher##ContextUpdate(&ctx->sha_ctx, ctx->key, sizeof(ctx->key)); \
        cipher##ContextUpdate(&ctx->sha_ctx, ctx->mac, sizeof(ctx->mac)); \
        cipher##ContextGetHash(&ctx->sha_ctx, ctx->mac); \
\
        /* We're done. */ \
        ctx->finalized = true; \
    } \
\
    memcpy(dst, ctx->mac, sizeof(ctx->mac))

#define HMAC_CALCULATE_MAC(cipher) \
    /* Make a new context, calculate hash, store to output, clear memory. */ \
    Hmac##cipher##Context ctx; \
    hmac##cipher##ContextCreate(&ctx, key, key_size); \
    hmac##cipher##ContextUpdate(&ctx, src, size); \
    hmac##cipher##ContextGetMac(&ctx, dst); \
    memset(&ctx, 0, sizeof(ctx))

void hmacSha256ContextCreate(HmacSha256Context *out, const void *key, size_t key_size) {
    HMAC_CONTEXT_CREATE(sha256);
}

void hmacSha256ContextUpdate(HmacSha256Context *ctx, const void *src, size_t size) {
    HMAC_CONTEXT_UPDATE(sha256);
}

void hmacSha256ContextGetMac(HmacSha256Context *ctx, void *dst) {
    HMAC_CONTEXT_GET_MAC(sha256);
}

void hmacSha256CalculateMac(void *dst, const void *key, size_t key_size, const void *src, size_t size) {
    HMAC_CALCULATE_MAC(Sha256);
}

void hmacSha1ContextCreate(HmacSha1Context *out, const void *key, size_t key_size) {
    HMAC_CONTEXT_CREATE(sha1);
}

void hmacSha1ContextUpdate(HmacSha1Context *ctx, const void *src, size_t size) {
    HMAC_CONTEXT_UPDATE(sha1);
}

void hmacSha1ContextGetMac(HmacSha1Context *ctx, void *dst) {
    HMAC_CONTEXT_GET_MAC(sha1);
}

void hmacSha1CalculateMac(void *dst, const void *key, size_t key_size, const void *src, size_t size) {
    HMAC_CALCULATE_MAC(Sha1);
}
