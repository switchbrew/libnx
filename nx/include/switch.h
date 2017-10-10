/**
 * @file switch.h
 * @brief Central Switch header. Includes all others.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <switch/types.h>
#include <switch/result.h>
#include <switch/svc.h>
#include <switch/ipc.h>

#include <switch/kernel/tmem.h>
#include <switch/kernel/mutex.h>
#include <switch/kernel/thread.h>
#include <switch/kernel/virtmem.h>

#include <switch/services/sm.h>
#include <switch/services/fs.h>
#include <switch/services/bsd.h>
#include <switch/services/fatal.h>
#include <switch/services/usb.h>
#include <switch/services/vi.h>

#ifdef __cplusplus
}
#endif

