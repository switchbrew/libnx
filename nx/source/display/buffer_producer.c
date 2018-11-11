#include <string.h>
#include "types.h"
#include "result.h"
#include "display/parcel.h"
#include "display/buffer_producer.h"


// This implements the version of Android IGraphicBufferProducer used by Switch.
// Hence names/params etc here are based on Android IGraphicBufferProducer.cpp.

// Based on an old version of the enum from the above .cpp.
// https://android.googlesource.com/platform/frameworks/native/+/29a3e90879fd96404c971e7187cd0e05927bbce0/libs/gui/IGraphicBufferProducer.cpp#35

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
    /* 0xE */ SET_PREALLOCATED_BUFFER, // Custom Switch-specific command
};

static const char g_bq_InterfaceDescriptor[] = "android.gui.IGraphicBufferProducer";

Result bqRequestBuffer(Binder *b, s32 bufferIdx, BqGraphicBuffer *buf)
{
    Result rc;
    Parcel parcel, parcel_reply;

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, bufferIdx);

    rc = parcelTransact(b, REQUEST_BUFFER, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        int nonNull = parcelReadInt32(&parcel_reply);

        if (nonNull != 0) {
            size_t tmp_size=0;
            void* tmp_ptr;

            tmp_ptr = parcelReadFlattenedObject(&parcel_reply, &tmp_size);
            if (!tmp_ptr)
                return MAKERESULT(Module_Libnx, LibnxError_BadInput);

            if (buf)
                return MAKERESULT(Module_Libnx, LibnxError_BadInput); // not implemented
        }

        rc = binderConvertErrorCode(parcelReadInt32(&parcel_reply));
    }

    return rc;
}

Result bqDequeueBuffer(Binder *b, bool async, u32 width, u32 height, s32 format, u32 usage, s32 *buf, NvMultiFence *fence)
{
    Result rc;
    Parcel parcel, parcel_reply;

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);

    parcelWriteInt32(&parcel, async);
    parcelWriteUInt32(&parcel, width);
    parcelWriteUInt32(&parcel, height);
    parcelWriteInt32(&parcel, format);
    parcelWriteUInt32(&parcel, usage);

    rc = parcelTransact(b, DEQUEUE_BUFFER, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        *buf = parcelReadInt32(&parcel_reply);

        if(parcelReadInt32(&parcel_reply)) {
            size_t tmp_size=0;
            void* tmp_ptr;

            tmp_ptr = parcelReadFlattenedObject(&parcel_reply, &tmp_size);
            if (tmp_ptr == NULL || tmp_size != sizeof(NvMultiFence))
                return MAKERESULT(Module_Libnx, LibnxError_BadInput);

            if (fence)
                memcpy(fence, tmp_ptr, sizeof(NvMultiFence));
        }

        rc = binderConvertErrorCode(parcelReadInt32(&parcel_reply));
    }

    return rc;
}

Result bqDetachBuffer(Binder *b, s32 slot)
{
    Result rc;
    Parcel parcel, parcel_reply;

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, slot);

    rc = parcelTransact(b, DETACH_BUFFER, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        rc = binderConvertErrorCode(parcelReadInt32(&parcel_reply));
    }

    return rc;
}

Result bqQueueBuffer(Binder *b, s32 buf, const BqBufferInput *input, BqBufferOutput *output)
{
    Result rc;
    Parcel parcel, parcel_reply;

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, buf);
    parcelWriteFlattenedObject(&parcel, input, sizeof(*input));

    rc = parcelTransact(b, QUEUE_BUFFER, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        if (parcelReadData(&parcel_reply, output, sizeof(*output)) == NULL)
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);

        rc = binderConvertErrorCode(parcelReadInt32(&parcel_reply));
    }

    return rc;
}

Result bqCancelBuffer(Binder *b, s32 buf, const NvMultiFence *fence)
{
    Result rc;
    Parcel parcel, parcel_reply;

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, buf);
    parcelWriteFlattenedObject(&parcel, fence, sizeof(*fence));

    rc = parcelTransact(b, CANCEL_BUFFER, &parcel, &parcel_reply);
    // Reply parcel has no content

    return rc;
}

Result bqQuery(Binder *b, s32 what, s32* value)
{
    Result rc;
    Parcel parcel, parcel_reply;

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, what);

    rc = parcelTransact(b, QUERY, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        *value = parcelReadInt32(&parcel_reply);

        rc = binderConvertErrorCode(parcelReadInt32(&parcel_reply));
    }

    return rc;
}

Result bqConnect(Binder *b, s32 api, bool producerControlledByApp, BqBufferOutput *output)
{
    Result rc;
    Parcel parcel, parcel_reply;

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);

    // Hard-code this as if listener==NULL, since that's not known to be used officially.
    parcelWriteInt32(&parcel, 0);
    parcelWriteInt32(&parcel, api);
    parcelWriteInt32(&parcel, producerControlledByApp);

    rc = parcelTransact(b, CONNECT, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        if (parcelReadData(&parcel_reply, output, sizeof(*output)) == NULL)
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);

        rc = binderConvertErrorCode(parcelReadInt32(&parcel_reply));
    }

    return rc;
}

Result bqDisconnect(Binder *b, s32 api)
{
    Result rc;
    Parcel parcel, parcel_reply;

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, api);

    rc = parcelTransact(b, DISCONNECT, &parcel, &parcel_reply);

    if (R_SUCCEEDED(rc)) {
        rc = binderConvertErrorCode(parcelReadInt32(&parcel_reply));
    }

    return rc;
}

Result bqSetPreallocatedBuffer(Binder *b, s32 buf, const BqGraphicBuffer *input)
{
    Result rc;
    Parcel parcel, parcel_reply;
    bool hasInput = false;

    if (input) {
        hasInput = true;
        if (!input->native_handle || input->native_handle->num_fds || input->native_handle->num_ints > 0x80)
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    parcelCreate(&parcel);
    parcelCreate(&parcel_reply);

    parcelWriteInterfaceToken(&parcel, g_bq_InterfaceDescriptor);
    parcelWriteInt32(&parcel, buf);

    parcelWriteInt32(&parcel, hasInput);
    if (hasInput) {
        struct {
            u32 magic;
            u32 width;
            u32 height;
            u32 stride;
            u32 format;
            u32 usage;

            u32 pid;
            u32 refcount;

            u32 numFds;
            u32 numInts;

            u32 ints[input->native_handle->num_ints];
        } buf;

        // Serialize the buffer
        buf.magic = 0x47424652; // GBFR (Graphic Buffer)
        buf.width = input->width;
        buf.height = input->height;
        buf.stride = input->stride;
        buf.format = input->format;
        buf.usage = input->usage;
        buf.pid = 42; // Official sw sets this to the value of getpid(), which is hardcoded to return 42.
        buf.refcount = 0; // Official sw sets this to the output of android_atomic_inc(). We instead don't care and set it to zero since it is ignored during marshalling.
        buf.numFds = 0;
        buf.numInts = input->native_handle->num_ints;
        memcpy(buf.ints, input->native_handle+1, sizeof(buf.ints));

        parcelWriteFlattenedObject(&parcel, &buf, sizeof(buf));
    }

    rc = parcelTransact(b, SET_PREALLOCATED_BUFFER, &parcel, &parcel_reply);
    // Reply parcel has no content

    return rc;
}
