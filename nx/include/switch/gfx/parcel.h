#include <string.h>
#include <switch.h>

typedef struct {
    u8  payload[0x400];
    u32 capacity;
    u32 size;
    u32 read_pos;

    u8 *ParcelObjects;
    u32 ParcelObjectsSize;
} parcelContext;

void parcelInitializeContext(parcelContext *ctx);
Result parcelTransact(binderSession *session, u32 code, parcelContext *in_parcel, parcelContext *reply_parcel);

void* parcelWriteData(parcelContext *ctx, void* data, size_t data_size);
void* parcelReadData(parcelContext *ctx, void* data, size_t data_size);

void parcelWriteInt32(parcelContext *ctx, s32 val);
void parcelWriteUInt32(parcelContext *ctx, u32 val);
void parcelWriteString16(parcelContext *ctx, const char *str);

void parcelWriteInterfaceToken(parcelContext *ctx, const char *interface);

s32 parcelReadInt32(parcelContext *ctx);
u32 parcelReadUInt32(parcelContext *ctx);

