#include <string.h>
#include "types.h"
#include "result.h"
#include "gfx/parcel.h"
#include "gfx/buffer_producer.h"


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

static char g_bufferProducer_InterfaceDescriptor[] = "android.gui.IGraphicBufferProducer";

static Binder *g_bufferProducerBinderSession;

Result bufferProducerInitialize(Binder *session)
{
    g_bufferProducerBinderSession = session;
    return 0;
}

void bufferProducerExit(void)
{
    g_bufferProducerBinderSession = NULL;
}

Result bufferProducerRequestBuffer(s32 bufferIdx, bufferProducerGraphicBuffer *buf)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bufferProducerBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelInitialize(&parcel);
    parcelInitialize(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bufferProducer_InterfaceDescriptor);
    parcelWriteInt32(&parcel, bufferIdx);

    rc = parcelTransact(g_bufferProducerBinderSession, REQUEST_BUFFER, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        int nonNull = parcelReadInt32(&parcel_reply);

        if (nonNull != 0) {
            size_t tmpsize=0;
            void* tmp_ptr;

            tmp_ptr = parcelReadFlattenedObject(&parcel_reply, &tmpsize);
            if (tmp_ptr==NULL || tmpsize!=sizeof(bufferProducerGraphicBuffer)) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
            if (buf) memcpy(buf, tmp_ptr, sizeof(bufferProducerGraphicBuffer));
        }

        int status = parcelReadInt32(&parcel_reply);

        if (status != 0) {
            rc = MAKERESULT(Module_Libnx, LibnxError_BufferProducerError);
        }
    }

    return rc;
}

Result bufferProducerDequeueBuffer(bool async, u32 width, u32 height, s32 format, u32 usage, s32 *buf, bufferProducerFence *fence)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bufferProducerBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelInitialize(&parcel);
    parcelInitialize(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bufferProducer_InterfaceDescriptor);

    parcelWriteInt32(&parcel, async);
    parcelWriteUInt32(&parcel, width);
    parcelWriteUInt32(&parcel, height);
    parcelWriteInt32(&parcel, format);
    parcelWriteUInt32(&parcel, usage);

    rc = parcelTransact(g_bufferProducerBinderSession, DEQUEUE_BUFFER, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        *buf = parcelReadInt32(&parcel_reply);

        if(parcelReadInt32(&parcel_reply)) {
            size_t tmpsize=0;
            void* tmp_ptr;

            tmp_ptr = parcelReadFlattenedObject(&parcel_reply, &tmpsize);
            if (tmp_ptr==NULL || tmpsize!=sizeof(bufferProducerFence)) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
            if (fence) memcpy(fence, tmp_ptr, sizeof(bufferProducerFence));
        }

        int result = parcelReadInt32(&parcel_reply);
        if (result != 0)
            rc = MAKERESULT(Module_Libnx, LibnxError_BufferProducerError);
    }

    return rc;
}

Result bufferProducerDetachBuffer(s32 slot)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bufferProducerBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelInitialize(&parcel);
    parcelInitialize(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bufferProducer_InterfaceDescriptor);
    parcelWriteInt32(&parcel, slot);

    rc = parcelTransact(g_bufferProducerBinderSession, DETACH_BUFFER, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        //TODO: parse reply
    }

    return rc;
}

Result bufferProducerQueueBuffer(s32 buf, bufferProducerQueueBufferInput *input, bufferProducerQueueBufferOutput *output)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bufferProducerBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelInitialize(&parcel);
    parcelInitialize(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bufferProducer_InterfaceDescriptor);
    parcelWriteInt32(&parcel, buf);
    parcelWriteFlattenedObject(&parcel, input, sizeof(bufferProducerQueueBufferInput));

    rc = parcelTransact(g_bufferProducerBinderSession, QUEUE_BUFFER, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        if (parcelReadData(&parcel_reply, output, sizeof(bufferProducerQueueBufferOutput))==NULL) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

        int result = parcelReadInt32(&parcel_reply);
        if (result != 0)
            rc = MAKERESULT(Module_Libnx, LibnxError_BufferProducerError);
    }

    return rc;
}

Result bufferProducerQuery(s32 what, s32* value)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bufferProducerBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelInitialize(&parcel);
    parcelInitialize(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bufferProducer_InterfaceDescriptor);
    parcelWriteInt32(&parcel, what);

    rc = parcelTransact(g_bufferProducerBinderSession, QUERY, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        *value = parcelReadInt32(&parcel_reply);

        int result = parcelReadInt32(&parcel_reply);
        if (result != 0)
            rc = MAKERESULT(Module_Libnx, LibnxError_BufferProducerError);
    }

    return rc;
}

Result bufferProducerConnect(s32 api, bool producerControlledByApp, bufferProducerQueueBufferOutput *output)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bufferProducerBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelInitialize(&parcel);
    parcelInitialize(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bufferProducer_InterfaceDescriptor);

    // Hard-code this as if listener==NULL, since that's not known to be used officially.
    parcelWriteInt32(&parcel, 0);
    parcelWriteInt32(&parcel, api);
    parcelWriteInt32(&parcel, producerControlledByApp);

    rc = parcelTransact(g_bufferProducerBinderSession, CONNECT, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        if (parcelReadData(&parcel_reply, output, sizeof(bufferProducerQueueBufferOutput))==NULL) return MAKERESULT(Module_Libnx, LibnxError_BadInput);

        int result = parcelReadInt32(&parcel_reply);
        if (result != 0)
            rc = MAKERESULT(Module_Libnx, LibnxError_BufferProducerError);
    }

    return rc;
}

Result bufferProducerDisconnect(s32 api)
{
    Result rc;
    Parcel parcel, parcel_reply;

    if (g_bufferProducerBinderSession == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelInitialize(&parcel);
    parcelInitialize(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bufferProducer_InterfaceDescriptor);
    parcelWriteInt32(&parcel, api);

    rc = parcelTransact(g_bufferProducerBinderSession, DISCONNECT, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        //TODO: parse reply
    }

    return rc;
}

Result bufferProducerGraphicBufferInit(s32 buf, bufferProducerGraphicBuffer *input)
{
    Result rc;
    Parcel parcel, parcel_reply;
    bool flag = 0;

    if (g_bufferProducerBinderSession==NULL) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    parcelInitialize(&parcel);
    parcelInitialize(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bufferProducer_InterfaceDescriptor);
    parcelWriteInt32(&parcel, buf);

    if (input!=NULL) flag = 1;
    parcelWriteInt32(&parcel, flag);
    if (flag) parcelWriteFlattenedObject(&parcel, input, sizeof(bufferProducerGraphicBuffer));

    rc = parcelTransact(g_bufferProducerBinderSession, GRAPHIC_BUFFER_INIT, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        int result = parcelReadInt32(&parcel_reply);
        if (result != 0)
            rc = MAKERESULT(Module_Libnx, LibnxError_BufferProducerError);
    }

    return rc;
}

