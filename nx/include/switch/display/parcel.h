#pragma once
#include "../result.h"
#include "../display/binder.h"

typedef struct {
    u32 payload_size;
    u32 payload_off;
    u32 objects_size;
    u32 objects_off;
} ParcelHeader;

#define PARCEL_MAX_PAYLOAD 0x400

typedef struct {
    u8  payload[PARCEL_MAX_PAYLOAD];
    u32 payload_size;
    u8* objects;
    u32 objects_size;

    u32 capacity;
    u32 pos;
} Parcel;

void parcelCreate(Parcel *ctx);
Result parcelTransact(Binder *session, u32 code, Parcel *in_parcel, Parcel *reply_parcel);

void* parcelWriteData(Parcel *ctx, const void* data, size_t data_size);
void* parcelReadData(Parcel *ctx, void* data, size_t data_size);

void parcelWriteInt32(Parcel *ctx, s32 val);
void parcelWriteUInt32(Parcel *ctx, u32 val);
void parcelWriteString16(Parcel *ctx, const char *str);

s32 parcelReadInt32(Parcel *ctx);
u32 parcelReadUInt32(Parcel *ctx);
void parcelWriteInterfaceToken(Parcel *ctx, const char *str);

void* parcelReadFlattenedObject(Parcel *ctx, size_t *size);
void* parcelWriteFlattenedObject(Parcel *ctx, const void* data, size_t size);

