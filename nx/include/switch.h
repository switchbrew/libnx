/**
 * @file switch.h
 * @brief Central Switch header. Includes all others.
 * @copyright libnx Authors
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "switch/types.h"
#include "switch/result.h"

#include "switch/kernel/svc.h"
#include "switch/kernel/tmem.h"
#include "switch/kernel/shmem.h"
#include "switch/kernel/mutex.h"
#include "switch/kernel/rwlock.h"
#include "switch/kernel/thread.h"
#include "switch/kernel/virtmem.h"
#include "switch/kernel/detect.h"
#include "switch/kernel/random.h"
#include "switch/kernel/jit.h"

#include "switch/arm/tls.h"
#include "switch/arm/cache.h"
#include "switch/ipc.h"

#include "switch/services/sm.h"
#include "switch/services/fs.h"
#include "switch/services/acc.h"
#include "switch/services/apm.h"
#include "switch/services/applet.h"
#include "switch/services/bsd.h"
#include "switch/services/fatal.h"
#include "switch/services/usb.h"
#include "switch/services/hid.h"
#include "switch/services/irs.h"
#include "switch/services/vi.h"
#include "switch/services/nv.h"
#include "switch/services/pm.h"

#include "switch/gfx/gfx.h"
#include "switch/gfx/binder.h"
#include "switch/gfx/parcel.h"
#include "switch/gfx/buffer_producer.h"
#include "switch/gfx/ioctl.h"
#include "switch/gfx/nvioctl.h"
#include "switch/gfx/nvgfx.h"

#include "switch/runtime/env.h"

#include "switch/runtime/util/utf.h"

#include "switch/runtime/devices/console.h"
#include "switch/runtime/devices/usb_comms.h"
#include "switch/runtime/devices/fs_dev.h"

#ifdef __cplusplus
}
#endif

