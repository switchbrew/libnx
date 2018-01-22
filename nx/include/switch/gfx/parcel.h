#pragma once
#include "../result.h"
#include "../gfx/binder.h"

typedef struct {
    u8 payload[0x400];
    u32 capacity;
    u32 size;
    u32 pos;

    u8 *ParcelObjects;
    u32 ParcelObjectsSize;
} Parcel;

void parcelInitialize(Parcel *ctx);
Result parcelTransact(Binder *session, u32 code, Parcel *in_parcel, Parcel *reply_parcel);

void* parcelWriteData(Parcel *ctx, void* data, size_t data_size);
void* parcelReadData(Parcel *ctx, void* data, size_t data_size);

void parcelWriteInt32(Parcel *ctx, s32 val);
void parcelWriteUInt32(Parcel *ctx, u32 val);
void parcelWriteString16(Parcel *ctx, const char *str);

s32 parcelReadInt32(Parcel *ctx);
u32 parcelReadUInt32(Parcel *ctx);
void parcelWriteInterfaceToken(Parcel *ctx, const char *str);

void* parcelReadFlattenedObject(Parcel *ctx, size_t *size);
void* parcelWriteFlattenedObject(Parcel *ctx, void* data, size_t size);

