#include <string.h>
#include "types.h"
#include "result.h"
#include "display/parcel.h"
#include "display/buffer_producer.h"


// This implements the version of Android IGraphicBufferProducer used by Switch.
// Hence names/params etc here are based on Android IGraphicBufferProducer.cpp.

// Based on an old version of the enum from the above .cpp.
// Unknown whether all of these are correct for Switch.
enum {
    /* 0x1 */ REQUEST_BUFFER = BINDER_FIRST_CALL_TRANSACTION,
    /* 0x2 */ SET_BUFFER_COUNT,
    /* 0x3 */ DEQUEUE_BUFFER,
    /* 0x4 */ DETACH_BUFFER,
    /* 0x5 */ DETACH_NEXT_BUFFER,
    /* 0x6 */ ATTACH_BUFFER,
    /* 0x7 */ QUEUE_BUFFER,
    /* 0x8 */ CANCEL_BUFFER,
    /* 0x9 */ QUERY,
    /* 0xA */ CONNECT,
    /* 0xB */ DISCONNECT,
    /* 0xC */ SET_SIDEBAND_STREAM,
    /* 0xD */ ALLOCATE_BUFFERS,
    /* 0xE */ GRAPHIC_BUFFER_INIT, // Custom Switch-specific command - unofficial name.
};

static char g_bq_InterfaceDescriptor[] = "android.gui.IGraphicBufferProducer";

static Binder *g_bqBinderSession;

Result bqInitialize(Binder *session)
{
    g_bqBinderSession = session;
    return 0;
}

void bqExit(void)
{
    g_bqBinderSession = NULL;
}

Result bqRequestBuffer(s32 bufferIdx, BqGraphicBuffer *buf)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bqBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, bufferIdx);

    rc = parcelTransact(g_bqBinderSession, REQUEST_BUFFER, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        int nonNull = parcelReadInt32(&parcel_reply);

        if (nonNull != 0) {
            size_t tmp_size=0;
            void* tmp_ptr;

            tmp_ptr = parcelReadFlattenedObject(&parcel_reply, &tmp_size);
            if (tmp_ptr == NULL || tmp_size != sizeof(BqGraphicBuffer))
                return MAKERESULT(Module_Libnx, LibnxError_BadInput);

            if (buf)
                memcpy(buf, tmp_ptr, sizeof(BqGraphicBuffer));
        }

        int status = parcelReadInt32(&parcel_reply);

        if (status != 0) {
            rc = MAKERESULT(Module_Libnx, LibnxError_BufferProducerError);
        }
    }

    return rc;
}

Result bqDequeueBuffer(bool async, u32 width, u32 height, s32 format, u32 usage, s32 *buf, BqFence *fence)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bqBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);

    parcelWriteInt32(&parcel, async);
    parcelWriteUInt32(&parcel, width);
    parcelWriteUInt32(&parcel, height);
    parcelWriteInt32(&parcel, format);
    parcelWriteUInt32(&parcel, usage);

    rc = parcelTransact(g_bqBinderSession, DEQUEUE_BUFFER, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        *buf = parcelReadInt32(&parcel_reply);

        if(parcelReadInt32(&parcel_reply)) {
            size_t tmp_size=0;
            void* tmp_ptr;

            tmp_ptr = parcelReadFlattenedObject(&parcel_reply, &tmp_size);
            if (tmp_ptr == NULL || tmp_size != sizeof(BqFence))
                return MAKERESULT(Module_Libnx, LibnxError_BadInput);

            if (fence)
                memcpy(fence, tmp_ptr, sizeof(BqFence));
        }

        int result = parcelReadInt32(&parcel_reply);
        if (result != 0)
            rc = MAKERESULT(Module_Libnx, LibnxError_BufferProducerError);
    }

    return rc;
}

Result bqDetachBuffer(s32 slot)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bqBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, slot);

    rc = parcelTransact(g_bqBinderSession, DETACH_BUFFER, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        //TODO: parse reply
    }

    return rc;
}

Result bqQueueBuffer(s32 buf, BqQueueBufferInput *input, BqQueueBufferOutput *output)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bqBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, buf);
    parcelWriteFlattenedObject(&parcel, input, sizeof(*input));

    rc = parcelTransact(g_bqBinderSession, QUEUE_BUFFER, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        if (parcelReadData(&parcel_reply, output, sizeof(*output)) == NULL)
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);

        int result = parcelReadInt32(&parcel_reply);
        if (result != 0)
            rc = MAKERESULT(Module_Libnx, LibnxError_BufferProducerError);
    }

    return rc;
}

Result bqQuery(s32 what, s32* value)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bqBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, what);

    rc = parcelTransact(g_bqBinderSession, QUERY, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        *value = parcelReadInt32(&parcel_reply);

        int result = parcelReadInt32(&parcel_reply);
        if (result != 0)
            rc = MAKERESULT(Module_Libnx, LibnxError_BufferProducerError);
    }

    return rc;
}

Result bqConnect(s32 api, bool producerControlledByApp, BqQueueBufferOutput *output)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bqBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);

    // Hard-code this as if listener==NULL, since that's not known to be used officially.
    parcelWriteInt32(&parcel, 0);
    parcelWriteInt32(&parcel, api);
    parcelWriteInt32(&parcel, producerControlledByApp);

    rc = parcelTransact(g_bqBinderSession, CONNECT, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        if (parcelReadData(&parcel_reply, output, sizeof(*output)) == NULL)
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);

        int result = parcelReadInt32(&parcel_reply);
        if (result != 0)
            rc = MAKERESULT(Module_Libnx, LibnxError_BufferProducerError);
    }

    return rc;
}

Result bqDisconnect(s32 api)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bqBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, api);

    rc = parcelTransact(g_bqBinderSession, DISCONNECT, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        // TODO: parse reply
    }

    return rc;
}

Result bqGraphicBufferInit(s32 buf, BqGraphicBuffer *input)
{
    Result rc;
    Parcel parcel, parcel_reply;
    bool flag = 0;

    if (g_bqBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, buf);

    if (input != NULL)
        flag = 1;

    parcelWriteInt32(&parcel, flag);
    if (flag)
        parcelWriteFlattenedObject(&parcel, input, sizeof(BqGraphicBuffer));

    rc = parcelTransact(g_bqBinderSession, GRAPHIC_BUFFER_INIT, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        int result = parcelReadInt32(&parcel_reply);
        if (result != 0)
            rc = MAKERESULT(Module_Libnx, LibnxError_BufferProducerError);
    }

    return rc;
}

