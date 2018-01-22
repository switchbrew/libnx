#include <string.h>
#include "result.h"
#include "gfx/parcel.h"

//This implements Android Parcel, hence names etc here are based on Android Parcel.cpp.

//#define PARCEL_LOGGING

#ifdef PARCEL_LOGGING
u8 parcel_reply_log[0x10000] = {0};
size_t parcel_reply_log_size = 0;
#endif

void parcelInitialize(Parcel *ctx)
{
    memset(ctx, 0, sizeof(Parcel));
    ctx->capacity = sizeof(ctx->payload);
}

Result parcelTransact(Binder *session, u32 code, Parcel *in_parcel, Parcel *parcel_reply)
{
    Result rc=0;
    u8 inparcel[0x400];
    u8 outparcel[0x400];
    size_t outparcel_size = sizeof(outparcel);
    u32 *inparcel32 = (u32*)inparcel;
    u32 *outparcel32 = (u32*)outparcel;
    u32 payloadSize = in_parcel->size;
    u32 ParcelObjectsSize = in_parcel->ParcelObjectsSize;

    memset(inparcel, 0, sizeof(inparcel));
    memset(outparcel, 0, outparcel_size);

    if((size_t)payloadSize >= sizeof(inparcel) || (size_t)ParcelObjectsSize >= sizeof(inparcel) || ((size_t)payloadSize)+((size_t)ParcelObjectsSize)+0x10 >= sizeof(inparcel)) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    inparcel32[0] = payloadSize;//payloadSize
    inparcel32[1] = 0x10;//payloadOffset
    inparcel32[2] = ParcelObjectsSize;//ParcelObjectsSize
    inparcel32[3] = 0x10+payloadSize;//ParcelObjectsOffset

    if(in_parcel->payload && payloadSize)memcpy(&inparcel[inparcel32[1]], in_parcel->payload, payloadSize);
    if(in_parcel->ParcelObjects && ParcelObjectsSize)memcpy(&inparcel[inparcel32[3]], in_parcel->ParcelObjects, ParcelObjectsSize);

    rc = binderTransactParcel(session, code, inparcel, payloadSize+ParcelObjectsSize+0x10, outparcel, outparcel_size, 0);
    if (R_FAILED(rc)) return rc;

    if((size_t)outparcel32[1] >= outparcel_size || ((size_t)outparcel32[0])+((size_t)outparcel32[1]) >= outparcel_size) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if((size_t)outparcel32[2] >= outparcel_size || ((size_t)outparcel32[2])+((size_t)outparcel32[3]) >= outparcel_size) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if((size_t)outparcel32[0] >= outparcel_size || (size_t)outparcel32[3] >= outparcel_size) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    memcpy(parcel_reply->payload, &outparcel[outparcel32[1]], outparcel32[0]);
    parcel_reply->size = outparcel32[0];

    #ifdef PARCEL_LOGGING
    if(parcel_reply_log_size + sizeof(inparcel) + outparcel_size <= sizeof(parcel_reply_log)) {
        memcpy(&parcel_reply_log[parcel_reply_log_size], inparcel, sizeof(inparcel));
        parcel_reply_log_size+= sizeof(inparcel);
        memcpy(&parcel_reply_log[parcel_reply_log_size], outparcel, outparcel_size);
        parcel_reply_log_size+= outparcel_size;
    }
    #endif

    return 0;
}

void* parcelWriteData(Parcel *ctx, void* data, size_t data_size)
{
    void* ptr = &ctx->payload[ctx->size];

    if(data_size & BIT(31))
        return NULL;

    data_size = (data_size+3) & ~3;

    if(ctx->size + data_size >= ctx->capacity)
        return NULL;

    if(data)
        memcpy(ptr, data, data_size);

    ctx->size += data_size;
    return ptr;
}

void* parcelReadData(Parcel *ctx, void* data, size_t data_size)
{
    void* ptr = &ctx->payload[ctx->pos];
    size_t aligned_data_size;

    if(data_size & BIT(31))
        return NULL;

    aligned_data_size = (data_size+3) & ~3;

    if(ctx->pos + aligned_data_size >= ctx->size)
        return NULL;

    if(data)
        memcpy(data, ptr, data_size);

    ctx->pos += aligned_data_size;
    return ptr;
}

void parcelWriteInt32(Parcel *ctx, s32 val) {
    parcelWriteData(ctx, &val, sizeof(val));
}

void parcelWriteUInt32(Parcel *ctx, u32 val) {
    parcelWriteData(ctx, &val, sizeof(val));
}

void parcelWriteString16(Parcel *ctx, const char *str)
{
    u32 pos, len;
    u16 *ptr;

    len = strlen(str);
    parcelWriteInt32(ctx, len);
    len++;

    ptr = parcelWriteData(ctx, NULL, len*2);
    if(ptr==NULL)return;

    for(pos=0; pos<len; pos++) {
        ptr[pos] = (u16)str[pos];
    }
}

void parcelWriteInterfaceToken(Parcel *ctx, const char *interface) {
    parcelWriteInt32(ctx, 0x100);
    parcelWriteString16(ctx, interface);
}

s32 parcelReadInt32(Parcel *ctx) {
    s32 val = 0;
    parcelReadData(ctx, &val, sizeof(val));
    return val;
}

u32 parcelReadUInt32(Parcel *ctx) {
    u32 val = 0;
    parcelReadData(ctx, &val, sizeof(val));
    return val;
}

void* parcelReadFlattenedObject(Parcel *ctx, size_t *size) {
    s32 len = parcelReadInt32(ctx);
    s32 fd_count = parcelReadInt32(ctx);

    if (size) *size = (u32)len;

    if(fd_count!=0)return NULL;//Not going to support fds.

    return parcelReadData(ctx, NULL, len);
}

void* parcelWriteFlattenedObject(Parcel *ctx, void* data, size_t size) {
    parcelWriteInt32(ctx, size);//len
    parcelWriteInt32(ctx, 0);//fd_count

    return parcelWriteData(ctx, data, size);
}

