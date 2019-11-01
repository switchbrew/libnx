/**
 * @file spl.h
 * @brief Security Processor Liaison (spl*) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"

#define SPL_RSA_BUFFER_SIZE (0x100)

typedef enum {
    SplConfigItem_DisableProgramVerification = 1,
    SplConfigItem_DramId = 2,
    SplConfigItem_SecurityEngineIrqNumber = 3,
    SplConfigItem_Version = 4,
    SplConfigItem_HardwareType = 5,
    SplConfigItem_IsRetail = 6,
    SplConfigItem_IsRecoveryBoot = 7,
    SplConfigItem_DeviceId = 8,
    SplConfigItem_BootReason = 9,
    SplConfigItem_MemoryArrange = 10,
    SplConfigItem_IsDebugMode = 11,
    SplConfigItem_KernelMemoryConfiguration = 12,
    SplConfigItem_IsChargerHiZModeEnabled = 13,
    SplConfigItem_IsKiosk = 14,
    SplConfigItem_NewHardwareType = 15,
    SplConfigItem_NewKeyGeneration = 16,
    SplConfigItem_Package2Hash = 17,
} SplConfigItem;

typedef enum {
    RsaKeyVersion_Deprecated = 0,
    RsaKeyVersion_Extended = 1,
} RsaKeyVersion;

/// Initialize 'spl:'.
Result splInitialize(void);

/// Exit 'spl:'.
void splExit(void);

/// Gets the Service object for the IGeneralInterface usable with spl*().
Service* splGetServiceSession(void);

/// Initialize spl:mig. On pre-4.0.0 this just calls \ref splInitialize.
Result splCryptoInitialize(void);

/// Exit spl:mig. On pre-4.0.0 this just calls \ref splExit.
void splCryptoExit(void);

/// Gets the Service object for the IGeneralInterface usable with splCrypto*().
Service* splCryptoGetServiceSession(void);

/// Initialize spl:ssl. On pre-4.0.0 this just calls \ref splInitialize.
Result splSslInitialize(void);

/// Exit spl:ssl. On pre-4.0.0 this just calls \ref splExit.
void splSslExit(void);

/// Gets the Service object for the IGeneralInterface usable with splSsl*().
Service* splSslGetServiceSession(void);

/// Initialize spl:es. On pre-4.0.0 this just calls \ref splInitialize.
Result splEsInitialize(void);

/// Exit spl:es. On pre-4.0.0 this just calls \ref splExit.
void splEsExit(void);

/// Gets the Service object for the IGeneralInterface usable with splEs*().
Service* splEsGetServiceSession(void);

/// Initialize spl:fs. On pre-4.0.0 this just calls \ref splInitialize.
Result splFsInitialize(void);

/// Exit spl:fs. On pre-4.0.0 this just calls \ref splExit.
void splFsExit(void);

/// Gets the Service object for the IGeneralInterface usable with splFs*().
Service* splFsGetServiceSession(void);

/// Initialize spl:manu. On pre-4.0.0 this just calls \ref splInitialize.
Result splManuInitialize(void);

/// Exit spl:manu. On pre-4.0.0 this just calls \ref splExit.
void splManuExit(void);

/// Gets the Service object for the IGeneralInterface usable with splManu*().
Service* splManuGetServiceSession(void);

Result splGetConfig(SplConfigItem config_item, u64 *out_config);
Result splUserExpMod(const void *input, const void *modulus, const void *exp, size_t exp_size, void *dst);
Result splSetConfig(SplConfigItem config_item, u64 value);
Result splGetRandomBytes(void *out, size_t out_size);
Result splIsDevelopment(bool *out_is_development);
Result splSetBootReason(u32 value);
Result splGetBootReason(u32 *out_value);

Result splCryptoGenerateAesKek(const void *wrapped_kek, u32 key_generation, u32 option, void *out_sealed_kek);
Result splCryptoLoadAesKey(const void *sealed_kek, const void *wrapped_key, u32 keyslot);
Result splCryptoGenerateAesKey(const void *sealed_kek, const void *wrapped_key, void *out_sealed_key);
Result splCryptoDecryptAesKey(const void *wrapped_key, u32 key_generation, u32 option, void *out_sealed_key);
Result splCryptoCryptAesCtr(const void *input, void *output, size_t size, u32 keyslot, const void *ctr);
Result splCryptoComputeCmac(const void *input, size_t size, u32 keyslot, void *out_cmac);
Result splCryptoLockAesEngine(u32 *out_keyslot);
Result splCryptoUnlockAesEngine(u32 keyslot);
Result splCryptoGetSecurityEngineEvent(Event *out_event);

Result splRsaDecryptPrivateKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version, void *dst, size_t dst_size);

Result splSslLoadSecureExpModKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version);
Result splSslSecureExpMod(const void *input, const void *modulus, void *dst);

Result splEsLoadRsaOaepKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version);
Result splEsUnwrapRsaOaepWrappedTitlekey(const void *rsa_wrapped_titlekey, const void *modulus, const void *label_hash, size_t label_hash_size, u32 key_generation, void *out_sealed_titlekey);
Result splEsUnwrapAesWrappedTitlekey(const void *aes_wrapped_titlekey, u32 key_generation, void *out_sealed_titlekey);
Result splEsLoadSecureExpModKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version);
Result splEsSecureExpMod(const void *input, const void *modulus, void *dst);
Result splEsUnwrapElicenseKey(const void *rsa_wrapped_elicense_key, const void *modulus, const void *label_hash, size_t label_hash_size, u32 key_generation, void *out_sealed_elicense_key);
Result splEsLoadElicenseKey(const void *sealed_elicense_key, u32 keyslot);

Result splFsLoadSecureExpModKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version);
Result splFsSecureExpMod(const void *input, const void *modulus, void *dst);
Result splFsGenerateSpecificAesKey(const void *wrapped_key, u32 key_generation, u32 option, void *out_sealed_key);
Result splFsLoadTitlekey(const void *sealed_titlekey, u32 keyslot);
Result splFsGetPackage2Hash(void *out_hash);

Result splManuEncryptRsaKeyForImport(const void *sealed_kek_pre, const void *wrapped_key_pre, const void *sealed_kek_post, const void *wrapped_kek_post, u32 option, const void *wrapped_rsa_key, void *out_wrapped_rsa_key, size_t rsa_key_size);
