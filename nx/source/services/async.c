#include "sf/service.h"
#include "runtime/hosversion.h"
#include "services/async.h"

static Result _asyncCmdNoIO(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _asyncCmdNoInOutU64(Service* srv, u64 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _asyncCmdNoInOutBuf(Service* srv, void* buffer, size_t size, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

// IAsyncValue

void asyncValueClose(AsyncValue *a) {
    if (serviceIsActive(&a->s)) {
        asyncValueCancel(a); // Official sw ignores rc from this prior to waiting on the event.
        asyncValueWait(a, U64_MAX);
    }

    serviceClose(&a->s);
    eventClose(&a->event);
}

Result asyncValueWait(AsyncValue *a, u64 timeout) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return eventWait(&a->event, timeout);
}

Result asyncValueGetSize(AsyncValue *a, u64 *size) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _asyncCmdNoInOutU64(&a->s, size, 0);
}

Result asyncValueGet(AsyncValue *a, void* buffer, size_t size) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    Result rc = asyncValueWait(a, U64_MAX);
    if (R_SUCCEEDED(rc)) rc = _asyncCmdNoInOutBuf(&a->s, buffer, size, 1);
    return rc;
}

Result asyncValueCancel(AsyncValue *a) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _asyncCmdNoIO(&a->s, 2);
}

Result asyncValueGetErrorContext(AsyncValue *a, ErrorContext *context) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _asyncCmdNoInOutBuf(&a->s, context, sizeof(*context), 3);
}

// AsyncResult

void asyncResultClose(AsyncResult *a) {
    if (serviceIsActive(&a->s)) {
        asyncResultCancel(a); // Official sw ignores rc from this prior to waiting on the event.
        asyncResultWait(a, U64_MAX);
    }

    serviceClose(&a->s);
    eventClose(&a->event);
}

Result asyncResultWait(AsyncResult *a, u64 timeout) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return eventWait(&a->event, timeout);
}

Result asyncResultGet(AsyncResult *a) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    Result rc = asyncResultWait(a, U64_MAX);
    if (R_SUCCEEDED(rc)) rc = _asyncCmdNoIO(&a->s, 0);
    return rc;
}

Result asyncResultCancel(AsyncResult *a) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _asyncCmdNoIO(&a->s, 1);
}

Result asyncResultGetErrorContext(AsyncResult *a, ErrorContext *context) {
    if (!serviceIsActive(&a->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _asyncCmdNoInOutBuf(&a->s, context, sizeof(*context), 2);
}

