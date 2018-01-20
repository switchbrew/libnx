/**
 * @file result.h
 * @brief Switch result code tools
 */
#pragma once
#include "types.h"

/// Checks whether a result code indicates success.
#define R_SUCCEEDED(res)   ((res)==0)
/// Checks whether a result code indicates failure.
#define R_FAILED(res)      ((res)!=0)
/// Returns the module ID of a result code.
#define R_MODULE(res)      ((res)&0x1FF)
/// Returns the description of a result code.
#define R_DESCRIPTION(res) (((res)>>9)&0x1FFF)

/// Builds a result code from its constituent components.
#define MAKERESULT(module,description) \
    ((((module)&0x1FF)) | ((description)&0x1FFF)<<9)

#define MODULE_LIBNX 345

enum {
    LIBNX_BADRELOC=1,
    LIBNX_OUTOFMEM,
    LIBNX_ALREADYMAPPED,
    LIBNX_BADGETINFO_STACK,
    LIBNX_BADGETINFO_HEAP,
    LIBNX_BADQUERYMEMORY,
    LIBNX_ALREADYINITIALIZED,
    LIBNX_NOTINITIALIZED,
    LIBNX_NOTFOUND,
    LIBNX_IOERROR,
    LIBNX_BADINPUT,
    LIBNX_BADREENT,
    LIBNX_BUFFERPRODUCER_ERROR,
    LIBNX_HANDLETOOEARLY,
    LIBNX_HEAPALLOCFAILED,
    LIBNX_TOOMANYOVERRIDES,
    LIBNX_PARCELERROR,
    LIBNX_BADGFXINIT,
    LIBNX_BADGFXEVENTWAIT,
    LIBNX_BADGFXQUEUEBUFFER,
    LIBNX_BADGFXDEQUEUEBUFFER,
    LIBNX_APPLETCMDIDNOTFOUND,
    LIBNX_BADAPPLETRECEIVEMESSAGE,
    LIBNX_BADAPPLETNOTIFYRUNNING,
    LIBNX_BADAPPLETGETCURRENTFOCUSSTATE,
    LIBNX_BADAPPLETGETOPERATIONMODE,
    LIBNX_BADAPPLETGETPERFORMANCEMODE,
    LIBNX_BADUSBCOMMSREAD,
    LIBNX_BADUSBCOMMSWRITE,
    LIBNX_INITFAIL_SM,
    LIBNX_INITFAIL_AM,
    LIBNX_INITFAIL_HID,
    LIBNX_INITFAIL_FS,
    LIBNX_BADGETINFO_RNG,
    LIBNX_JITUNAVAILABLE,
    LIBNX_WEIRDKERNEL,
};
