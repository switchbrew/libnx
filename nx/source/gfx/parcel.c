#include <string.h>
#include <switch.h>

//This implements Android Parcel, hence names etc here are based on Android Parcel.cpp.

//#define PARCEL_LOGGING

#ifdef PARCEL_LOGGING
u8 parcel_reply_log[0x10000] = {0};
size_t parcel_reply_log_size = 0;
#endif

void parcelInitializeContext(parcelContext *ctx) {
    memset(ctx, 0, sizeof(parcelContext));
    ctx->ParcelData_maxsize = sizeof(ctx->ParcelData);
}

Result parcelTransact(binderSession *session, u32 code, parcelContext *in_parcel, parcelContext *parcel_reply) {
    Result rc=0;
    u8 inparcel[0x400];
    u8 outparcel[0x400];
    size_t outparcel_size = sizeof(outparcel);
    u32 *inparcel32 = (u32*)inparcel;
    u32 *outparcel32 = (u32*)outparcel;
    u32 ParcelDataSize = in_parcel->ParcelData_size;
    u32 ParcelObjectsSize = in_parcel->ParcelObjectsSize;

    memset(inparcel, 0, sizeof(inparcel));
    memset(outparcel, 0, outparcel_size);

    if((size_t)ParcelDataSize >= sizeof(inparcel) || (size_t)ParcelObjectsSize >= sizeof(inparcel) || ((size_t)ParcelDataSize)+((size_t)ParcelObjectsSize)+0x10 >= sizeof(inparcel)) return MAKERESULT(MODULE_LIBNX, LIBNX_BADINPUT);

    inparcel32[0] = ParcelDataSize;//ParcelDataSize
    inparcel32[1] = 0x10;//ParcelDataOffset
    inparcel32[2] = ParcelObjectsSize;//ParcelObjectsSize
    inparcel32[3] = 0x10+ParcelDataSize;//ParcelObjectsOffset

    if(in_parcel->ParcelData && ParcelDataSize)memcpy(&inparcel[inparcel32[1]], in_parcel->ParcelData, ParcelDataSize);
    if(in_parcel->ParcelObjects && ParcelObjectsSize)memcpy(&inparcel[inparcel32[3]], in_parcel->ParcelObjects, ParcelObjectsSize);

    rc = binderTransactParcel(session, code, inparcel, ParcelDataSize+ParcelObjectsSize+0x10, outparcel, outparcel_size, 0);
    if (R_FAILED(rc)) return rc;

    if((size_t)outparcel32[1] >= outparcel_size || ((size_t)outparcel32[0])+((size_t)outparcel32[1]) >= outparcel_size) return MAKERESULT(MODULE_LIBNX, LIBNX_BADINPUT);
    if((size_t)outparcel32[2] >= outparcel_size || ((size_t)outparcel32[2])+((size_t)outparcel32[3]) >= outparcel_size) return MAKERESULT(MODULE_LIBNX, LIBNX_BADINPUT);
    if((size_t)outparcel32[0] >= outparcel_size || (size_t)outparcel32[3] >= outparcel_size) return MAKERESULT(MODULE_LIBNX, LIBNX_BADINPUT);

    memcpy(parcel_reply->ParcelData, &outparcel[outparcel32[1]], outparcel32[0]);
    parcel_reply->ParcelData_size = outparcel32[0];

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

void* parcelWriteData(parcelContext *ctx, void* data, size_t data_size) {
    void* ptr = &ctx->ParcelData[ctx->ParcelData_size];

    if(data_size & BIT(31)) return NULL;
    data_size = (data_size+3) & ~3;

    if(ctx->ParcelData_size + data_size >= ctx->ParcelData_maxsize) return NULL;

    if(data)memcpy(ptr, data, data_size);
    ctx->ParcelData_size+= data_size;

    return ptr;
}

void* parcelReadData(parcelContext *ctx, void* data, size_t data_size) {
    void* ptr = ctx->ParcelData;
    size_t aligned_data_size;

    if(data_size & BIT(31)) return NULL;
    aligned_data_size = (data_size+3) & ~3;

    if(ctx->ParcelData_pos + aligned_data_size >= ctx->ParcelData_size) return NULL;

    if(data)memcpy(data, &ctx->ParcelData[ctx->ParcelData_pos], data_size);
    ctx->ParcelData_pos+= aligned_data_size;

    return ptr;
}

void parcelWriteInt32(parcelContext *ctx, s32 val) {
    parcelWriteData(ctx, &val, sizeof(val));
}

void parcelWriteUInt32(parcelContext *ctx, u32 val) {
    parcelWriteData(ctx, &val, sizeof(val));
}

void parcelWriteString16_fromchar(parcelContext *ctx, const char *str) {
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

void parcelWriteInterfaceToken(parcelContext *ctx, const char *interface) {
    parcelWriteInt32(ctx, 0x100);
    parcelWriteString16_fromchar(ctx, interface);
}

s32 parcelReadInt32(parcelContext *ctx) {
    s32 val = 0;
    parcelReadData(ctx, &val, sizeof(val));
    return val;
}

u32 parcelReadUInt32(parcelContext *ctx) {
    u32 val = 0;
    parcelReadData(ctx, &val, sizeof(val));
    return val;
}

