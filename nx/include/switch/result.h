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

#define MODULE_LIBNX       345
#define LIBNX_BADRELOC       1
#define LIBNX_OUTOFMEM       2
#define LIBNX_ALREADYMAPPED  3
#define LIBNX_BADGETINFO     4
#define LIBNX_BADQUERYMEMORY 5
#define LIBNX_ALREADYINITIALIZED 6
#define LIBNX_NOTINITIALIZED 7
#define LIBNX_NOTFOUND       8
#define LIBNX_IOERROR        9
#define LIBNX_BADINPUT       10
#define LIBNX_BADREENT       11
#define LIBNX_BUFFERPRODUCER_ERROR 12
#define LIBNX_PARCEL_ERRBASE 100
