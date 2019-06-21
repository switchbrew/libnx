/*
  chacha-ref.c version 20080118
  D. J. Bernstein
  Public domain.

  Modified by plutoo.
*/

#include <string.h>
#include "types.h"
#include "result.h"
#include "services/fatal.h"
#include "kernel/mutex.h"
#include "kernel/svc.h"
#include "kernel/random.h"
#include "runtime/env.h"

#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32-(n))))

#define U32TO8_LITTLE(p, v) \
    (*(u32*)(p) = (v))

#define U8TO32_LITTLE(p) \
    (*(const u32*)(p))

#define ROTATE(v,c) (ROTL32(v,c))
#define XOR(v,w) ((v) ^ (w))
#define PLUS(v,w) ((v) + (w))
#define PLUSONE(v) (PLUS((v),1))

#define QUARTERROUND(a,b,c,d) \
    x[a] = PLUS(x[a],x[b]); x[d] = ROTATE(XOR(x[d],x[a]),16); \
    x[c] = PLUS(x[c],x[d]); x[b] = ROTATE(XOR(x[b],x[c]),12); \
    x[a] = PLUS(x[a],x[b]); x[d] = ROTATE(XOR(x[d],x[a]), 8); \
    x[c] = PLUS(x[c],x[d]); x[b] = ROTATE(XOR(x[b],x[c]), 7);

typedef struct {
    u32 input[16];
} ChaCha;

static void _Round(u8 output[64], const u32 input[16])
{
    u32 x[16];
    int i;

    for (i = 0;i < 16;++i)
        x[i] = input[i];

    for (i = 8;i > 0;i -= 2) {
        QUARTERROUND( 0, 4, 8,12);
        QUARTERROUND( 1, 5, 9,13);
        QUARTERROUND( 2, 6,10,14);
        QUARTERROUND( 3, 7,11,15);
        QUARTERROUND( 0, 5,10,15);
        QUARTERROUND( 1, 6,11,12);
        QUARTERROUND( 2, 7, 8,13);
        QUARTERROUND( 3, 4, 9,14);
    }

    for (i = 0;i < 16;++i)
        x[i] = PLUS(x[i],input[i]);

    for (i = 0;i < 16;++i)
        U32TO8_LITTLE(output + 4 * i,x[i]);
}

static const char sigma[16] = "expand 32-byte k";

static void chachaInit(ChaCha* x, const u8* key, const u8* iv)
{
    // Setup key.
    x->input[0]  = U8TO32_LITTLE(sigma + 0);
    x->input[1]  = U8TO32_LITTLE(sigma + 4);
    x->input[2]  = U8TO32_LITTLE(sigma + 8);
    x->input[3]  = U8TO32_LITTLE(sigma + 12);
    x->input[4]  = U8TO32_LITTLE(key + 0);
    x->input[5]  = U8TO32_LITTLE(key + 4);
    x->input[6]  = U8TO32_LITTLE(key + 8);
    x->input[7]  = U8TO32_LITTLE(key + 12);
    x->input[8]  = U8TO32_LITTLE(key + 16);
    x->input[9]  = U8TO32_LITTLE(key + 20);
    x->input[10] = U8TO32_LITTLE(key + 24);
    x->input[11] = U8TO32_LITTLE(key + 28);

    // Setup iv.
    x->input[12] = 0;
    x->input[13] = 0;
    x->input[14] = U8TO32_LITTLE(iv + 0);
    x->input[15] = U8TO32_LITTLE(iv + 4);
}

static void chachaEncrypt(ChaCha* x, const u8* m, u8* c, size_t bytes)
{
    u8 output[64];
    int i;

    while (bytes > 0)
    {
        _Round(output, x->input);

        x->input[12] = PLUSONE(x->input[12]);

        if (!x->input[12]) {
            x->input[13] = PLUSONE(x->input[13]);
            /* stopping at 2^70 bytes per nonce is user's responsibility */
        }

        if (bytes <= 64) {
            for (i = 0;i < bytes;++i)
                c[i] = m[i] ^ output[i];
            return;
        }

        for (i = 0;i < 64;++i)
            c[i] = m[i] ^ output[i];

        bytes -= 64;
        c += 64;
        m += 64;
    }
}

static ChaCha g_chacha;
static bool   g_randInit = false;
static Mutex  g_randMutex;

static void _randomInit(void)
{
    // Has already initialized?
    if (g_randInit)
        return;

    u64 seed[4];

    int i;
    for (i=0; i<4; i++)
    {
        // Get process TRNG seeds from kernel.
        if (R_FAILED(svcGetInfo(&seed[i], InfoType_RandomEntropy, INVALID_HANDLE, i)))
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadGetInfo_Rng));
    }

    if (envHasRandomSeed())
    {
        u64 other_seed[2];
        envGetRandomSeed(other_seed);
        seed[0] ^= other_seed[1];
        seed[1] ^= seed[0];
        seed[2] ^= other_seed[0];
        seed[3] ^= seed[2];
    }

    u8 iv[8];
    memset(iv, 0, sizeof iv);

    chachaInit(&g_chacha, (const u8*) seed, iv);
    g_randInit = true;
}

void randomGet(void* buf, size_t len)
{
    mutexLock(&g_randMutex);

    _randomInit();

    memset(buf, 0, len);
    chachaEncrypt(&g_chacha, (const u8*)buf, (u8*)buf, len);

    mutexUnlock(&g_randMutex);
}

u64 randomGet64(void)
{
    u64 tmp;
    randomGet(&tmp, sizeof(tmp));
    return tmp;
}
