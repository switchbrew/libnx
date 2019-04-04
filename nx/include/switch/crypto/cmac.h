/**
 * @file cmac.h
 * @brief Hardware accelerated AES-CMAC implementation.
 * @copyright libnx Authors
 */
#pragma once
#include "aes.h"

/// Context for AES-128 CMAC.
typedef struct {
    Aes128Context ctx;
    u8 subkey[AES_BLOCK_SIZE];
    u8 mac[AES_BLOCK_SIZE];
    u8 buffer[AES_BLOCK_SIZE];
    size_t num_buffered;
    bool finalized;
} Aes128CmacContext;

/// Context for AES-192 CMAC.
typedef struct {
    Aes192Context ctx;
    u8 subkey[AES_BLOCK_SIZE];
    u8 mac[AES_BLOCK_SIZE];
    u8 buffer[AES_BLOCK_SIZE];
    size_t num_buffered;
    bool finalized;
} Aes192CmacContext;

/// Context for AES-256 CMAC.
typedef struct {
    Aes256Context ctx;
    u8 subkey[AES_BLOCK_SIZE];
    u8 mac[AES_BLOCK_SIZE];
    u8 buffer[AES_BLOCK_SIZE];
    size_t num_buffered;
    bool finalized;
} Aes256CmacContext;

/// Initialize an AES-128-CMAC context.
void cmacAes128ContextCreate(Aes128CmacContext *out, const void *key);
/// Updates AES-128-CMAC context with data to hash
void cmacAes128ContextUpdate(Aes128CmacContext *ctx, const void *src, size_t size);
/// Gets the context's output mac, finalizes the context.
void cmacAes128ContextGetMac(Aes128CmacContext *ctx, void *dst);

/// Simple all-in-one AES-128-CMAC calculator.
void cmacAes128CalculateMac(void *dst, const void *key, const void *src, size_t size);

/// Initialize an AES-192-CMAC context.
void cmacAes192ContextCreate(Aes192CmacContext *out, const void *key);
/// Updates AES-192-CMAC context with data to hash
void cmacAes192ContextUpdate(Aes192CmacContext *ctx, const void *src, size_t size);
/// Gets the context's output mac, finalizes the context.
void cmacAes192ContextGetMac(Aes192CmacContext *ctx, void *dst);

/// Simple all-in-one AES-192-CMAC calculator.
void cmacAes192CalculateMac(void *dst, const void *key, const void *src, size_t size);

/// Initialize an AES-256-CMAC context.
void cmacAes256ContextCreate(Aes256CmacContext *out, const void *key);
/// Updates AES-256-CMAC context with data to hash
void cmacAes256ContextUpdate(Aes256CmacContext *ctx, const void *src, size_t size);
/// Gets the context's output mac, finalizes the context.
void cmacAes256ContextGetMac(Aes256CmacContext *ctx, void *dst);

/// Simple all-in-one AES-256-CMAC calculator.
void cmacAes256CalculateMac(void *dst, const void *key, const void *src, size_t size);
