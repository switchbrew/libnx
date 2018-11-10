#include <string.h>
#include "result.h"
#include "display/parcel.h"

// This implements Android Parcel, hence names etc here are based on Android Parcel.cpp.

void parcelCreate(Parcel *ctx)
{
    memset(ctx, 0, sizeof(Parcel));
    ctx->capacity = sizeof(ctx->payload);
}

Result parcelTransact(Binder *session, u32 code, Parcel *in_parcel, Parcel *out_parcel)
{
    Result rc;
    char in[PARCEL_MAX_PAYLOAD];
    char out[PARCEL_MAX_PAYLOAD];

    memset(in,  0, sizeof(in));
    memset(out, 0, sizeof(out));

    if (in_parcel->payload_size > sizeof(in))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (in_parcel->objects_size > sizeof(in))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    size_t total_size = 0;
    total_size += sizeof(ParcelHeader);
    total_size += in_parcel->payload_size;
    total_size += in_parcel->objects_size;

    if (total_size > sizeof(in))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    ParcelHeader* in_hdr = (ParcelHeader*) &in;
    in_hdr->payload_size = in_parcel->payload_size;
    in_hdr->payload_off  = sizeof(ParcelHeader);
    in_hdr->objects_size = in_parcel->objects_size;
    in_hdr->objects_off  = sizeof(ParcelHeader) + in_parcel->payload_size;

    if (in_parcel->payload != NULL)
        memcpy(&in[in_hdr->payload_off], in_parcel->payload, in_parcel->payload_size);

    if (in_parcel->objects != NULL)
        memcpy(&in[in_hdr->objects_off], in_parcel->objects, in_parcel->objects_size);

    rc = binderTransactParcel(session, code, in, total_size, out, sizeof(out), 0);

    if (R_SUCCEEDED(rc))
    {
        ParcelHeader* out_hdr = (ParcelHeader*) &out;

        if (out_hdr->payload_size > sizeof(out))
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
        if (out_hdr->objects_size > sizeof(out))
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
        if (out_hdr->payload_off > sizeof(out))
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
        if (out_hdr->objects_off > sizeof(out))
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
        if ((out_hdr->payload_off+out_hdr->payload_size) > sizeof(out))
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
        if ((out_hdr->objects_off+out_hdr->objects_size) > sizeof(out))
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);

        memcpy(out_parcel->payload, &out[out_hdr->payload_off], out_hdr->payload_size);
        out_parcel->payload_size = out_hdr->payload_size;

        // TODO: Objects are not populated on response.
        out_parcel->objects = NULL;
        out_parcel->objects_size = 0;
    }

    return rc;
}

void* parcelWriteData(Parcel *ctx, const void* data, size_t data_size)
{
    void* ptr = &ctx->payload[ctx->payload_size];

    if (data_size & BIT(31))
        return NULL;

    data_size = (data_size+3) & ~3;

    if (ctx->payload_size + data_size >= ctx->capacity)
        return NULL;

    if (data)
        memcpy(ptr, data, data_size);

    ctx->payload_size += data_size;
    return ptr;
}

void* parcelReadData(Parcel *ctx, void* data, size_t data_size)
{
    void* ptr = &ctx->payload[ctx->pos];
    size_t aligned_data_size;

    if (data_size & BIT(31))
        return NULL;

    aligned_data_size = (data_size+3) & ~3;

    if (ctx->pos + aligned_data_size >= ctx->payload_size)
        return NULL;

    if (data)
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

void parcelWriteString16(Parcel *ctx, const char *str) {
    u32 pos, len;
    u16 *ptr;

    len = strlen(str);
    parcelWriteInt32(ctx, len);
    len++;

    ptr = parcelWriteData(ctx, NULL, len*2);
    if (ptr == NULL)
        return;

    for (pos=0; pos<len; pos++) {
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

    if (size)
        *size = (u32)len;

    if (fd_count != 0)
        return NULL; // Not going to support fds.

    return parcelReadData(ctx, NULL, len);
}

void* parcelWriteFlattenedObject(Parcel *ctx, const void* data, size_t size) {
    parcelWriteInt32(ctx, size); // len
    parcelWriteInt32(ctx, 0); // fd_count

    return parcelWriteData(ctx, data, size);
}

