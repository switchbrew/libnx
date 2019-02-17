// Copyright 2018 SciresM
#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/sm.h"
#include "services/spl.h"

static Service g_splSrv, g_splCryptoSrv, g_splSslSrv, g_splEsSrv, g_splFsSrv, g_splManuSrv;
static u64 g_splRefCnt, g_splCryptoRefCnt, g_splSslRefCnt, g_splEsRefCnt, g_splFsRefCnt, g_splManuRefCnt;

/* Helper prototypes for accessing handles. */
static Service* _splGetGeneralSrv(void);
static Service* _splGetCryptoSrv(void);
static Service* _splGetRsaSrv(void);

static Service* _splGetEsSrv(void);
static Service* _splGetFsSrv(void);

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

/* There are like six services, so these helpers will initialize/exit the relevant services. */
static Result _splSrvInitialize(Service* srv, u64 *refcnt, const char *name) {
    atomicIncrement64(refcnt);
    
    if (serviceIsActive(srv))
        return 0;
    
    return smGetService(srv, name);
}

static void _splSrvExit(Service* srv, u64 *refcnt) {
    if (atomicDecrement64(refcnt) == 0)
        serviceClose(srv);
}

Result splInitialize(void) {
    return _splSrvInitialize(&g_splSrv, &g_splRefCnt, "spl:");
}

void splExit(void) {
    return _splSrvExit(&g_splSrv, &g_splRefCnt);
}

Result splCryptoInitialize(void) {
    if (hosversionAtLeast(4,0,0)) {
        return _splSrvInitialize(&g_splCryptoSrv, &g_splCryptoRefCnt, "spl:mig");
    } else {
        return splInitialize();
    }
}

void splCryptoExit(void) {
    if (hosversionAtLeast(4,0,0)) {
        return _splSrvExit(&g_splCryptoSrv, &g_splCryptoRefCnt);
    } else {
        return splExit();
    }
}

Result splSslInitialize(void) {
    if (hosversionAtLeast(4,0,0)) {
        return _splSrvInitialize(&g_splSslSrv, &g_splSslRefCnt, "spl:ssl");
    } else {
        return splInitialize();
    }
}

void splSslExit(void) {
    if (hosversionAtLeast(4,0,0)) {
        return _splSrvExit(&g_splSslSrv, &g_splSslRefCnt);
    } else {
        return splExit();
    }
}

Result splEsInitialize(void) {
    if (hosversionAtLeast(4,0,0)) {
        return _splSrvInitialize(&g_splEsSrv, &g_splEsRefCnt, "spl:es");
    } else {
        return splInitialize();
    }
}

void splEsExit(void) {
    if (hosversionAtLeast(4,0,0)) {
        return _splSrvExit(&g_splEsSrv, &g_splEsRefCnt);
    } else {
        return splExit();
    }
}

Result splFsInitialize(void) {
    if (hosversionAtLeast(4,0,0)) {
        return _splSrvInitialize(&g_splFsSrv, &g_splFsRefCnt, "spl:fs");
    } else {
        return splInitialize();
    }
}

void splFsExit(void) {
    if (hosversionAtLeast(4,0,0)) {
        return _splSrvExit(&g_splFsSrv, &g_splFsRefCnt);
    } else {
        return splExit();
    }
}

Result splManuInitialize(void) {
    return _splSrvInitialize(&g_splManuSrv, &g_splManuRefCnt, "spl:manu");
}

void splManuExit(void) {
     return _splSrvExit(&g_splManuSrv, &g_splManuRefCnt);
}


/* SPL IGeneralService functionality. */
Result splGetConfig(SplConfigItem config_item, u64 *out_config) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 config_item;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->config_item = config_item;

    Result rc = serviceIpcDispatch(_splGetGeneralSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 out;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            *out_config = resp->out;
        }
    }

    return rc;
}

Result splUserExpMod(const void *input, const void *modulus, const void *exp, size_t exp_size, void *dst) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddSendStatic(&c, input, SPL_RSA_BUFFER_SIZE, 0);
    ipcAddSendStatic(&c, exp, exp_size, 1);
    ipcAddSendStatic(&c, modulus, SPL_RSA_BUFFER_SIZE, 2);
    ipcAddRecvStatic(&c, dst, SPL_RSA_BUFFER_SIZE, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;

    Result rc = serviceIpcDispatch(_splGetGeneralSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result splSetConfig(SplConfigItem config_item, u64 value) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 config_item;
        u64 value;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->config_item = config_item;
    raw->value = value;

    Result rc = serviceIpcDispatch(_splGetGeneralSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result splGetRandomBytes(void *out, size_t out_size) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddRecvStatic(&c, out, out_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7;

    Result rc = serviceIpcDispatch(_splGetGeneralSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result splIsDevelopment(bool *out_is_development) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 11;

    Result rc = serviceIpcDispatch(_splGetGeneralSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 is_development;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            *out_is_development = resp->is_development != 0;
        }
    }

    return rc;
}

Result splSetSharedData(u32 value) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 value;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 24;
    raw->value = value;

    Result rc = serviceIpcDispatch(_splGetGeneralSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result splGetSharedData(u32 *out_value) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 25;

    Result rc = serviceIpcDispatch(_splGetGeneralSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 value;
        } *resp = r.Raw;

        rc = resp->result;
        
        if (R_SUCCEEDED(rc)) {
            *out_value = resp->value;
        }
    }

    return rc;
}

/* SPL ICryptoService functionality. */
Result splCryptoGenerateAesKek(const void *wrapped_kek, u32 key_generation, u32 option, void *out_sealed_kek) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 wrapped_kek[0x10];
        u32 key_generation;
        u32 option;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    memcpy(raw->wrapped_kek, wrapped_kek, sizeof(raw->wrapped_kek));
    raw->key_generation = key_generation;
    raw->option = option;

    Result rc = serviceIpcDispatch(_splGetCryptoSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 sealed_kek[0x10];
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            memcpy(out_sealed_kek, resp->sealed_kek, sizeof(resp->sealed_kek));
        }
    }

    return rc;
}

Result splCryptoLoadAesKey(const void *sealed_kek, const void *wrapped_key, u32 keyslot) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 sealed_kek[0x10];
        u8 wrapped_key[0x10];
        u32 keyslot;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    memcpy(raw->sealed_kek, sealed_kek, sizeof(raw->sealed_kek));
    memcpy(raw->wrapped_key, wrapped_key, sizeof(raw->wrapped_key));
    raw->keyslot = keyslot;

    Result rc = serviceIpcDispatch(_splGetCryptoSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result splCryptoGenerateAesKey(const void *sealed_kek, const void *wrapped_key, void *out_sealed_key) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 sealed_kek[0x10];
        u8 wrapped_key[0x10];
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;
    memcpy(raw->sealed_kek, sealed_kek, sizeof(raw->sealed_kek));
    memcpy(raw->wrapped_key, wrapped_key, sizeof(raw->wrapped_key));

    Result rc = serviceIpcDispatch(_splGetCryptoSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 sealed_key[0x10];
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            memcpy(out_sealed_key, resp->sealed_key, sizeof(resp->sealed_key));
        }
    }

    return rc;
}

Result splCryptoDecryptAesKey(const void *wrapped_key, u32 key_generation, u32 option, void *out_sealed_key) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 wrapped_key[0x10];
        u32 key_generation;
        u32 option;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 14;
    memcpy(raw->wrapped_key, wrapped_key, sizeof(raw->wrapped_key));
    raw->key_generation = key_generation;
    raw->option = option;

    Result rc = serviceIpcDispatch(_splGetCryptoSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 sealed_key[0x10];
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            memcpy(out_sealed_key, resp->sealed_key, sizeof(resp->sealed_key));
        }
    }

    return rc;
}

Result splCryptoCryptAesCtr(const void *input, void *output, size_t size, const void *ctr) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddSendBuffer(&c, input, size, BufferType_Type1);
    ipcAddRecvBuffer(&c, output, size, BufferType_Type1);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 ctr[0x10];
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 15;
    memcpy(raw->ctr, ctr, sizeof(raw->ctr));

    Result rc = serviceIpcDispatch(_splGetCryptoSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result splCryptoComputeCmac(const void *input, size_t size, u32 keyslot, void *out_cmac) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddSendStatic(&c, input, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 keyslot;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 16;
    raw->keyslot = keyslot;

    Result rc = serviceIpcDispatch(_splGetCryptoSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 cmac[0x10];
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            memcpy(out_cmac, resp->cmac, sizeof(resp->cmac));
        }
    }

    return rc;
}

Result splCryptoLockAesEngine(u32 *out_keyslot) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 21;

    Result rc = serviceIpcDispatch(_splGetCryptoSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u32 keyslot;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            *out_keyslot = resp->keyslot;
        }
    }

    return rc;
}

Result splCryptoUnlockAesEngine(u32 keyslot) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u32 keyslot;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 22;
    raw->keyslot = keyslot;

    Result rc = serviceIpcDispatch(_splGetCryptoSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result splCryptoGetSecurityEngineEvent(Handle *out_event) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 23;

    Result rc = serviceIpcDispatch(_splGetCryptoSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out_event = r.Handles[0];
        }
    }

    return rc;
}

/* SPL IRsaService functionality. NOTE: IRsaService is not a real part of inheritance, unlike ICryptoService/IGeneralService. */
Result splRsaDecryptPrivateKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version, void *dst, size_t dst_size) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddSendStatic(&c, wrapped_rsa_key, wrapped_rsa_key_size, 0);
    ipcAddRecvStatic(&c, dst, dst_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 sealed_kek[0x10];
        u8 wrapped_key[0x10];
        u32 version;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 13;
    memcpy(raw->sealed_kek, sealed_kek, sizeof(raw->sealed_kek));
    memcpy(raw->wrapped_key, wrapped_key, sizeof(raw->wrapped_key));
    raw->version = version;

    Result rc = serviceIpcDispatch(_splGetRsaSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

/* Helper function for RSA key importing. */
static Result _splImportSecureExpModKey(Service* srv, u64 cmd_id, const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddSendStatic(&c, wrapped_rsa_key, wrapped_rsa_key_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 sealed_kek[0x10];
        u8 wrapped_key[0x10];
        u32 version;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    memcpy(raw->sealed_kek, sealed_kek, sizeof(raw->sealed_kek));
    memcpy(raw->wrapped_key, wrapped_key, sizeof(raw->wrapped_key));
    raw->version = version;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _splSecureExpMod(Service* srv, u64 cmd_id, const void *input, const void *modulus, void *dst) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddSendStatic(&c, input, SPL_RSA_BUFFER_SIZE, 0);
    ipcAddSendStatic(&c, modulus, SPL_RSA_BUFFER_SIZE, 1);
    ipcAddRecvStatic(&c, dst, SPL_RSA_BUFFER_SIZE, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

/* SPL ISslService functionality. */
Result splSslLoadSecureExpModKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version) {
    return _splImportSecureExpModKey(&g_splSslSrv, 26, sealed_kek, wrapped_key, wrapped_rsa_key, wrapped_rsa_key_size, version);
}

Result splSslSecureExpMod(const void *input, const void *modulus, void *dst) {
    return _splSecureExpMod(&g_splSslSrv, 27, input, modulus, dst);
}

/* SPL IEsService functionality. */
Result splEsLoadRsaOaepKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version) {
    return _splImportSecureExpModKey(_splGetEsSrv(), 17, sealed_kek, wrapped_key, wrapped_rsa_key, wrapped_rsa_key_size, version);
}

Result splEsUnwrapRsaOaepWrappedTitlekey(const void *rsa_wrapped_titlekey, const void *modulus, const void *label_hash, size_t label_hash_size, u32 key_generation, void *out_sealed_titlekey) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddSendStatic(&c, rsa_wrapped_titlekey, SPL_RSA_BUFFER_SIZE, 0);
    ipcAddSendStatic(&c, modulus, SPL_RSA_BUFFER_SIZE, 1);
    ipcAddSendStatic(&c, label_hash, label_hash_size, 2);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 key_generation;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 18;
    raw->key_generation = key_generation;

    Result rc = serviceIpcDispatch(_splGetEsSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 sealed_titlekey[0x10];
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            memcpy(out_sealed_titlekey, resp->sealed_titlekey, sizeof(resp->sealed_titlekey));
        }
    }

    return rc;
}

Result splEsUnwrapAesWrappedTitlekey(const void *aes_wrapped_titlekey, u32 key_generation, void *out_sealed_titlekey) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u8 aes_wrapped_titlekey[0x10];
        u32 key_generation;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 20;
    memcpy(raw->aes_wrapped_titlekey, aes_wrapped_titlekey, sizeof(raw->aes_wrapped_titlekey));
    raw->key_generation = key_generation;

    Result rc = serviceIpcDispatch(_splGetEsSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 sealed_titlekey[0x10];
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            memcpy(out_sealed_titlekey, resp->sealed_titlekey, sizeof(resp->sealed_titlekey));
        }
    }

    return rc;
}

Result splEsLoadSecureExpModKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version) {
    return _splImportSecureExpModKey(&g_splEsSrv, 28, sealed_kek, wrapped_key, wrapped_rsa_key, wrapped_rsa_key_size, version);
}

Result splEsSecureExpMod(const void *input, const void *modulus, void *dst) {
    return _splSecureExpMod(&g_splEsSrv, 29, input, modulus, dst);
}

/* SPL IFsService functionality. */
Result splFsLoadSecureExpModKey(const void *sealed_kek, const void *wrapped_key, const void *wrapped_rsa_key, size_t wrapped_rsa_key_size, RsaKeyVersion version) {
    return _splImportSecureExpModKey(_splGetFsSrv(), 9, sealed_kek, wrapped_key, wrapped_rsa_key, wrapped_rsa_key_size, version);
}

Result splFsSecureExpMod(const void *input, const void *modulus, void *dst) {
    return _splSecureExpMod(_splGetFsSrv(), 10, input, modulus, dst);
}

Result splFsGenerateSpecificAesKey(const void *wrapped_key, u32 key_generation, u32 option, void *out_sealed_key) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 wrapped_key[0x10];
        u32 key_generation;
        u32 option;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;
    memcpy(raw->wrapped_key, wrapped_key, sizeof(raw->wrapped_key));
    raw->key_generation = key_generation;
    raw->option = option;

    Result rc = serviceIpcDispatch(_splGetFsSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 sealed_key[0x10];
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            memcpy(out_sealed_key, resp->sealed_key, sizeof(resp->sealed_key));
        }
    }

    return rc;
}

Result splFsLoadTitlekey(const void *sealed_titlekey, u32 keyslot) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 sealed_titlekey[0x10];
        u32 keyslot;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 19;
    memcpy(raw->sealed_titlekey, sealed_titlekey, sizeof(raw->sealed_titlekey));
    raw->keyslot = keyslot;

    Result rc = serviceIpcDispatch(_splGetFsSrv());

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result splFsGetPackage2Hash(void *out_hash) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 31;

    Result rc = serviceIpcDispatch(&g_splFsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 package2_hash[0x20];
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            memcpy(out_hash, resp->package2_hash, sizeof(resp->package2_hash));
        }
    }

    return rc;
}

/* SPL IManuService funcionality. */
Result splManuEncryptRsaKeyForImport(const void *sealed_kek_pre, const void *wrapped_key_pre, const void *sealed_kek_post, const void *wrapped_kek_post, u32 option, const void *wrapped_rsa_key, void *out_wrapped_rsa_key, size_t rsa_key_size) {
    IpcCommand c;
    ipcInitialize(&c);
    
    ipcAddSendStatic(&c, wrapped_rsa_key, rsa_key_size, 0);
    ipcAddRecvStatic(&c, out_wrapped_rsa_key, rsa_key_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 sealed_kek_pre[0x10];
        u8 wrapped_key_pre[0x10];
        u8 sealed_kek_post[0x10];
        u8 wrapped_kek_post[0x10];
        u32 option;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 30;
    memcpy(raw->sealed_kek_pre, sealed_kek_pre, sizeof(raw->sealed_kek_pre));
    memcpy(raw->wrapped_key_pre, wrapped_key_pre, sizeof(raw->wrapped_key_pre));
    memcpy(raw->sealed_kek_post, sealed_kek_post, sizeof(raw->sealed_kek_post));
    memcpy(raw->wrapped_kek_post, wrapped_kek_post, sizeof(raw->wrapped_kek_post));
    raw->option = option;

    Result rc = serviceIpcDispatch(&g_splManuSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}
