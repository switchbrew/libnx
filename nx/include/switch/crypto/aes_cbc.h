/**
 * @file aes_cbc.h
 * @brief Hardware accelerated AES-CBC implementation.
 * @copyright libnx Authors
 */
#pragma once
#include "aes.h"

/// Context for AES-128 CBC.
typedef struct {
    Aes128Context aes_ctx;
    u8 iv[AES_BLOCK_SIZE];
    u8 buffer[AES_BLOCK_SIZE];
    size_t num_buffered;
} Aes128CbcContext;

/// Context for AES-192 CBC.
typedef struct {
    Aes192Context aes_ctx;
    u8 iv[AES_BLOCK_SIZE];
    u8 buffer[AES_BLOCK_SIZE];
    size_t num_buffered;
} Aes192CbcContext;

/// Context for AES-256 CBC.
typedef struct {
    Aes256Context aes_ctx;
    u8 iv[AES_BLOCK_SIZE];
    u8 buffer[AES_BLOCK_SIZE];
    size_t num_buffered;
} Aes256CbcContext;

/// 128-bit CBC API.
void aes128CbcContextCreate(Aes128CbcContext *out, const void *key, const void *iv, bool is_encryptor);
void aes128CbcContextResetIv(Aes128CbcContext *ctx, const void *iv);
size_t aes128CbcEncrypt(Aes128CbcContext *ctx, void *dst, const void *src, size_t size);
size_t aes128CbcDecrypt(Aes128CbcContext *ctx, void *dst, const void *src, size_t size);

/// 192-bit CBC API.
void aes192CbcContextCreate(Aes192CbcContext *out, const void *key, const void *iv, bool is_encryptor);
void aes192CbcContextResetIv(Aes192CbcContext *ctx, const void *iv);
size_t aes192CbcEncrypt(Aes192CbcContext *ctx, void *dst, const void *src, size_t size);
size_t aes192CbcDecrypt(Aes192CbcContext *ctx, void *dst, const void *src, size_t size);

/// 256-bit CBC API.
void aes256CbcContextCreate(Aes256CbcContext *out, const void *key, const void *iv, bool is_encryptor);
void aes256CbcContextResetIv(Aes256CbcContext *ctx, const void *iv);
size_t aes256CbcEncrypt(Aes256CbcContext *ctx, void *dst, const void *src, size_t size);
size_t aes256CbcDecrypt(Aes256CbcContext *ctx, void *dst, const void *src, size_t size);
