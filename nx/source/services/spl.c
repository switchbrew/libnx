// Copyright 2018 SciresM
#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/spl.h"
#include "crypto/sha256.h"

/* Structs used in service implementations. */
typedef struct SplKey {
    u8 key[0x10];
} SplKey;

static Service g_splSrv, g_splCryptoSrv, g_splSslSrv, g_splEsSrv, g_splFsSrv, g_splManuSrv;

/* Helper prototypes for accessing handles. */
NX_INLINE Service* _splGetGeneralSrv(void);
NX_INLINE Service* _splGetCryptoSrv(void);
NX_INLINE Service* _splGetRsaSrv(void);
NX_INLINE Service* _splGetEsSrv(void);
NX_INLINE Service* _splGetFsSrv(void);
NX_INLINE Service* _splGetSslSrv(void);
NX_INLINE Service* _splGetManuSrv(void);

Service* _splGetGeneralSrv(void) {
    if (hosversionBefore(4,0,0)) {
        return &g_splSrv;
    }

    if (serviceIsActive(&g_splSrv)) {
        return &g_splSrv;
    } else {
        return _splGetCryptoSrv();
    }
}

Service* _splGetCryptoSrv(void) {
    if (hosversionBefore(4,0,0)) {
        return &g_splSrv;
    }

    if (serviceIsActive(&g_splManuSrv)) {
        return &g_splManuSrv;
    } else if (serviceIsActive(&g_splFsSrv)) {
        return &g_splFsSrv;
    } else if (serviceIsActive(&g_splEsSrv)) {
        return &g_splEsSrv;
    } else if (serviceIsActive(&g_splSslSrv)) {
        return &g_splSslSrv;
    } else {
        return &g_splCryptoSrv;
    }
}

Service* _splGetRsaSrv(void) {
    if (hosversionBefore(4,0,0)) {
        return &g_splSrv;
    }

    if (serviceIsActive(&g_splFsSrv)) {
        return &g_splFsSrv;
    } else if (serviceIsActive(&g_splEsSrv)) {
        return &g_splEsSrv;
    } else {
        return &g_splSslSrv;
    }
}

Service* _splGetEsSrv(void) {
    return hosversionAtLeast(4,0,0) ? &g_splEsSrv : &g_splSrv;
}

Service* _splGetFsSrv(void) {
    return hosversionAtLeast(4,0,0) ? &g_splFsSrv : &g_splSrv;
}

Service* _splGetSslSrv(void) {
    return hosversionAtLeast(4,0,0) ? &g_splSslSrv : &g_splSrv;
}

Service* _splGetManuSrv(void) {
    return hosversionAtLeast(4,0,0) ? &g_splManuSrv : &g_splSrv;
}

/* Initialization. */
NX_GENERATE_SERVICE_GUARD(spl)
NX_GENERATE_SERVICE_GUARD(splCrypto)
NX_GENERATE_SERVICE_GUARD(splSsl)
NX_GENERATE_SERVICE_GUARD(splEs)
NX_GENERATE_SERVICE_GUARD(splFs)
NX_GENERATE_SERVICE_GUARD(splManu)

static Result _splInitialize(void) {
    return smGetService(&g_splSrv, "spl:");
}

static void _splCleanup() {
    serviceClose(&g_splSrv);
}

Service* splGetServiceSession(void) {
    return _splGetGeneralSrv();
}

#define NX_GENERATE_SPL_SRV_INIT(name, subsrv)                  \
static Result _spl##name##Initialize(void) {                    \
    if (hosversionAtLeast(4,0,0)) {                             \
        return smGetService(&g_spl##name##Srv, "spl:"#subsrv);  \
    } else {                                                    \
        return splInitialize();                                 \
    }                                                           \
}                                                               \
                                                                \
static void _spl##name##Cleanup() {                             \
    if (hosversionAtLeast(4,0,0)) {                             \
        serviceClose(&g_spl##name##Srv);                        \
    } else {                                                    \
        splExit();                                              \
    }                                                           \
}                                                               \
Service* _spl##name##GetServiceSession() {                      \
    return _splGet##name##Srv();                                \
}

NX_GENERATE_SPL_SRV_INIT(Crypto, mig)
NX_GENERATE_SPL_SRV_INIT(Ssl,    ssl)
NX_GENERATE_SPL_SRV_INIT(Es,     es)
NX_GENERATE_SPL_SRV_INIT(Fs,     fs)
NX_GENERATE_SPL_SRV_INIT(Manu,   manu)

static Result _splCmdNoInOutU8(Service* srv, u8 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _splCmdNoInOutBool(Service* srv, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _splCmdNoInOutU8(srv, &tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

/* SPL IGeneralService functionality. */
Result splGetConfig(SplConfigItem config_item, u64 *out_config) {
    const struct {
        u32 config_item;
    } in = { config_item };
    return serviceDispatchInOut(_splGetGeneralSrv(), 0, in, *out_config);
}

Result splUserExpMod(const void *input, const void *modulus, const void *exp, size_t exp_size, void *dst) {
    return serviceDispatch(_splGetGeneralSrv(), 1,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_Out,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { dst,     SPL_RSA_BUFFER_SIZE },
            { input,   SPL_RSA_BUFFER_SIZE },
            { exp,     exp_size },
            { modulus, SPL_RSA_BUFFER_SIZE },
        },
    );
}

Result splSetConfig(SplConfigItem config_item, u64 value) {
    const struct {
        u32 config_item;
        u64 value;
    } in = { config_item, value };
    return serviceDispatchIn(_splGetGeneralSrv(), 5, in);
}

Result splGetRandomBytes(void *out, size_t out_size) {
    return serviceDispatch(_splGetGeneralSrv(), 7,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_Out,
        },
        .buffers = {
            { out, out_size },
        },
    );
}

Result splIsDevelopment(bool *out_is_development) {
    return _splCmdNoInOutBool(_splGetGeneralSrv(), out_is_development, 11);
}

Result splSetBootReason(u32 value) {
    if (hosversionBefore(3,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return serviceDispatchIn(_splGetGeneralSrv(), 24, value);
}

Result splGetBootReason(u32 *out_value) {
    if (hosversionBefore(3,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return serviceDispatchOut(_splGetGeneralSrv(), 25, *out_value);
}

/* SPL ICryptoService functionality. */

Result splCryptoGenerateAesKek(const void *wrapped_kek, u32 key_generation, u32 option, void *out_sealed_kek) {
    const struct {
        SplKey wrapped_kek;
        u32 key_generation;
        u32 option;
    } in = { *((const SplKey *)wrapped_kek), key_generation, option };
    return serviceDispatchInOut(_splGetCryptoSrv(), 2, in, *((SplKey *)out_sealed_kek));
}

Result splCryptoLoadAesKey(const void *sealed_kek, const void *wrapped_key, u32 keyslot) {
    const struct {
        SplKey sealed_kek;
        SplKey wrapped_key;
        u32 keyslot;
    } in = { *((const SplKey *)sealed_kek), *((const SplKey *)wrapped_key), keyslot };
    return serviceDispatchIn(_splGetCryptoSrv(), 3, in);
}

Result splCryptoGenerateAesKey(const void *sealed_kek, const void *wrapped_key, void *out_sealed_key) {
    const struct {
        SplKey sealed_kek;
        SplKey wrapped_key;
    } in = { *((const SplKey *)sealed_kek), *((const SplKey *)wrapped_key) };
    return serviceDispatchInOut(_splGetCryptoSrv(), 4, in, *((SplKey *)out_sealed_key));
}

Result splCryptoDecryptAesKey(const void *wrapped_key, u32 key_generation, u32 option, void *out_sealed_key) {
    const struct {
        SplKey wrapped_key;
        u32 key_generation;
        u32 option;
    } in = { *((const SplKey *)wrapped_key), key_generation, option };
    return serviceDispatchInOut(_splGetCryptoSrv(), 14, in, *((SplKey *)out_sealed_key));
}

Result splCryptoCryptAesCtr(const void *input, void *output, size_t size, u32 keyslot, const void *ctr) {
    const struct {
        SplKey ctr;
        u32 keyslot;
    } in = { *((const SplKey *)ctr), keyslot };
    return serviceDispatchIn(_splGetCryptoSrv(), 15, in,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out | SfBufferAttr_HipcMapTransferAllowsNonSecure,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In  | SfBufferAttr_HipcMapTransferAllowsNonSecure,
        },
        .buffers = {
            { output, size },
            { input,  size },
        },
    );
}

Result splCryptoComputeCmac(const void *input, size_t size, u32 keyslot, void *out_cmac) {
    return serviceDispatchInOut(_splGetCryptoSrv(), 16, keyslot, *((SplKey *)out_cmac),
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { input,  size },
        },
    );
}

Result splCryptoLockAesEngine(u32 *out_keyslot) {
    if (hosversionBefore(2,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return serviceDispatchOut(_splGetCryptoSrv(), 21, *out_keyslot);
}

Result splCryptoUnlockAesEngine(u32 keyslot) {
    if (hosversionBefore(2,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return serviceDispatchIn(_splGetCryptoSrv(), 22, keyslot);
}

Result splCryptoGetSecurityEngineEvent(Event *out_event) {
    if (hosversionBefore(2,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(_splGetCryptoSrv(), 23);
    if (R_SUCCEEDED(rc))
        eventLoadRemote(out_event, event, true);
    return rc;
}

/* SPL IRsaService functionality. NOTE: IRsaService is not a real part of inheritance, unlike ICryptoService/IGeneralService. */
Result splRsaDecryptPrivateKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version, void *dst, size_t dst_size) {
    const struct {
        SplKey sealed_kek;
        SplKey wrapped_key;
        u32    version;
    } in = { *((const SplKey *)sealed_kek), *((const SplKey *)wrapped_key), version };
    return serviceDispatchIn(_splGetRsaSrv(), 13, in,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_Out,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { dst, dst_size },
            { wrapped_rsa_key, wrapped_rsa_key_size },
        },
    );
}

/* Helper function for RSA key importing. */
NX_INLINE Result _splImportSecureExpModKey(Service* srv, u32 cmd_id, const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version) {
    const struct {
        SplKey sealed_kek;
        SplKey wrapped_key;
        u32    version;
    } in = { *((const SplKey *)sealed_kek), *((const SplKey *)wrapped_key), version };
    return serviceDispatchIn(srv, cmd_id, in,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { wrapped_rsa_key, wrapped_rsa_key_size },
        },
    );
}

NX_INLINE Result _splSecureExpMod(Service* srv, u32 cmd_id, const void *input, const void *modulus, void *dst) {
    return serviceDispatch(srv, cmd_id,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_Out,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { dst,     SPL_RSA_BUFFER_SIZE },
            { input,   SPL_RSA_BUFFER_SIZE },
            { modulus, SPL_RSA_BUFFER_SIZE },
        },
    );
}

/* SPL ISslService functionality. */
Result splSslLoadSecureExpModKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version) {
    if (hosversionBefore(5,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return _splImportSecureExpModKey(&g_splSslSrv, 26, sealed_kek, wrapped_key, wrapped_rsa_key, wrapped_rsa_key_size, version);
}

Result splSslSecureExpMod(const void *input, const void *modulus, void *dst) {
    if (hosversionBefore(5,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return _splSecureExpMod(&g_splSslSrv, 27, input, modulus, dst);
}

/* SPL IEsService functionality. */
NX_INLINE Result _splUnwrapRsaOaepWrappedKey(Service *srv, u32 cmd_id, const void *rsa_wrapped_key, const void *modulus, const void *label_hash, size_t label_hash_size, u32 key_generation, void *out_sealed_key) {
    return serviceDispatchInOut(srv, cmd_id, key_generation, *((SplKey *)out_sealed_key),
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { rsa_wrapped_key, SPL_RSA_BUFFER_SIZE },
            { modulus,              SPL_RSA_BUFFER_SIZE },
            { label_hash,           label_hash_size },
        },
    );
}

NX_INLINE Result _splLoadContentKey(Service *srv, u32 cmd_id, const void *sealed_key, u32 keyslot) {
    const struct {
        SplKey sealed_key;
        u32 keyslot;
    } in = { *((const SplKey *)sealed_key), keyslot };
    return serviceDispatchIn(srv, cmd_id, in);
}

Result splEsLoadRsaOaepKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version) {
    return _splImportSecureExpModKey(_splGetEsSrv(), 17, sealed_kek, wrapped_key, wrapped_rsa_key, wrapped_rsa_key_size, version);
}

Result splEsUnwrapRsaOaepWrappedTitlekey(const void *rsa_wrapped_titlekey, const void *modulus, const void *label_hash, size_t label_hash_size, u32 key_generation, void *out_sealed_titlekey) {
    return _splUnwrapRsaOaepWrappedKey(_splGetEsSrv(), 18, rsa_wrapped_titlekey, modulus, label_hash, label_hash_size, key_generation, out_sealed_titlekey);
}

Result splEsUnwrapAesWrappedTitlekey(const void *aes_wrapped_titlekey, u32 key_generation, void *out_sealed_titlekey) {
    if (hosversionBefore(2,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    const struct {
        SplKey aes_wrapped_titlekey;
        u32 key_generation;
    } in = { *(const SplKey *)aes_wrapped_titlekey, key_generation };
    return serviceDispatchInOut(_splGetEsSrv(), 20, in, *((SplKey *)out_sealed_titlekey));
}

Result splEsLoadSecureExpModKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version) {
    if (hosversionBefore(5,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return _splImportSecureExpModKey(&g_splEsSrv, 28, sealed_kek, wrapped_key, wrapped_rsa_key, wrapped_rsa_key_size, version);
}

Result splEsSecureExpMod(const void *input, const void *modulus, void *dst) {
    if (hosversionBefore(5,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return _splSecureExpMod(&g_splEsSrv, 29, input, modulus, dst);
}

Result splEsUnwrapElicenseKey(const void *rsa_wrapped_elicense_key, const void *modulus, const void *label_hash, size_t label_hash_size, u32 key_generation, void *out_sealed_elicense_key) {
    if (hosversionBefore(6,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return _splUnwrapRsaOaepWrappedKey(&g_splEsSrv, 31, rsa_wrapped_elicense_key, modulus, label_hash, label_hash_size, key_generation, out_sealed_elicense_key);
}

Result splEsLoadElicenseKey(const void *sealed_elicense_key, u32 keyslot) {
    if (hosversionBefore(6,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return _splLoadContentKey(&g_splEsSrv, 32, sealed_elicense_key, keyslot);
}

/* SPL IFsService functionality. */
Result splFsLoadSecureExpModKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version) {
    return _splImportSecureExpModKey(_splGetFsSrv(), 9, sealed_kek, wrapped_key, wrapped_rsa_key, wrapped_rsa_key_size, version);
}

Result splFsSecureExpMod(const void *input, const void *modulus, void *dst) {
    return _splSecureExpMod(_splGetFsSrv(), 10, input, modulus, dst);
}

Result splFsGenerateSpecificAesKey(const void *wrapped_key, u32 key_generation, u32 option, void *out_sealed_key) {
    const struct {
        SplKey wrapped_key;
        u32 key_generation;
        u32 option;
    } in = { *((const SplKey *)wrapped_key), key_generation, option };
    return serviceDispatchInOut(_splGetFsSrv(), 12, in, *((SplKey *)out_sealed_key));
}

Result splFsLoadTitlekey(const void *sealed_titlekey, u32 keyslot) {
    return _splLoadContentKey(_splGetFsSrv(), 19, sealed_titlekey, keyslot);
}

Result splFsGetPackage2Hash(void *out_hash) {
    if (hosversionBefore(5,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return serviceDispatch(&g_splFsSrv, 31,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_Out,
        },
        .buffers = {
            { out_hash, SHA256_HASH_SIZE },
        },
    );
}

/* SPL IManuService funcionality. */
Result splManuEncryptRsaKeyForImport(const void *sealed_kek_pre, const void *wrapped_key_pre, const void *sealed_kek_post, const void *wrapped_kek_post, u32 option, const void *wrapped_rsa_key, void *out_wrapped_rsa_key, size_t rsa_key_size) {
    if (hosversionBefore(5,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    const struct {
        SplKey sealed_kek_pre;
        SplKey wrapped_key_pre;
        SplKey sealed_kek_post;
        SplKey wrapped_kek_post;
        u32 option;
    } in = { *((const SplKey *)sealed_kek_pre), *((const SplKey *)wrapped_key_pre), *((const SplKey *)sealed_kek_post), *((const SplKey *)wrapped_kek_post), option };
    return serviceDispatchIn(&g_splManuSrv, 30, in,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_Out,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { out_wrapped_rsa_key, rsa_key_size },
            { wrapped_rsa_key,     rsa_key_size },
        },
    );
}
