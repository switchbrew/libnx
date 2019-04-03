/**
 * @file aes_ctr.h
 * @brief Hardware accelerated AES-CTR implementation.
 * @copyright libnx Authors
 */
#pragma once
#include "aes.h"

/// Context for AES-128 CTR.
typedef struct {
    Aes128Context aes_ctx;
    u8 ctr[AES_BLOCK_SIZE];
    u8 enc_ctr_buffer[AES_BLOCK_SIZE];
    size_t buffer_offset;
} Aes128CtrContext;

/// Context for AES-192 CTR.
typedef struct {
    Aes192Context aes_ctx;
    u8 ctr[AES_BLOCK_SIZE];
    u8 enc_ctr_buffer[AES_BLOCK_SIZE];
    size_t buffer_offset;
} Aes192CtrContext;

/// Context for AES-256 CTR.
typedef struct {
    Aes256Context aes_ctx;
    u8 ctr[AES_BLOCK_SIZE];
    u8 enc_ctr_buffer[AES_BLOCK_SIZE];
    size_t buffer_offset;
} Aes256CtrContext;

/// 128-bit CTR API.
void aes128CtrContextCreate(Aes128CtrContext *out, const void *key, const void *ctr);
void aes128CtrContextResetCtr(Aes128CtrContext *ctx, const void *ctr);
void aes128CtrCrypt(Aes128CtrContext *ctx, void *dst, const void *src, size_t size);

/// 192-bit CTR API.
void aes192CtrContextCreate(Aes192CtrContext *out, const void *key, const void *ctr);
void aes192CtrContextResetCtr(Aes192CtrContext *ctx, const void *ctr);
void aes192CtrCrypt(Aes192CtrContext *ctx, void *dst, const void *src, size_t size);

/// 256-bit CTR API.
void aes256CtrContextCreate(Aes256CtrContext *out, const void *key, const void *ctr);
void aes256CtrContextResetCtr(Aes256CtrContext *ctx, const void *ctr);
void aes256CtrCrypt(Aes256CtrContext *ctx, void *dst, const void *src, size_t size);
