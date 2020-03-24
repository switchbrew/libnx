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

#include "switch/nro.h"
#include "switch/nacp.h"

#include "switch/arm/tls.h"
#include "switch/arm/cache.h"
#include "switch/arm/counter.h"

#include "switch/kernel/svc.h"
#include "switch/kernel/wait.h"
#include "switch/kernel/tmem.h"
#include "switch/kernel/shmem.h"
#include "switch/kernel/mutex.h"
#include "switch/kernel/event.h"
#include "switch/kernel/uevent.h"
#include "switch/kernel/utimer.h"
#include "switch/kernel/rwlock.h"
#include "switch/kernel/condvar.h"
#include "switch/kernel/thread.h"
#include "switch/kernel/semaphore.h"
#include "switch/kernel/virtmem.h"
#include "switch/kernel/detect.h"
#include "switch/kernel/random.h"
#include "switch/kernel/jit.h"
#include "switch/kernel/barrier.h"

#include "switch/sf/hipc.h"
#include "switch/sf/cmif.h"
#include "switch/sf/service.h"
#include "switch/sf/sessionmgr.h"

#include "switch/services/sm.h"
#include "switch/services/smm.h"
#include "switch/services/fs.h"
#include "switch/services/fsldr.h"
#include "switch/services/fspr.h"
#include "switch/services/acc.h"
#include "switch/services/apm.h"
#include "switch/services/applet.h"
#include "switch/services/async.h"
#include "switch/services/audin.h"
#include "switch/services/audout.h"
#include "switch/services/audren.h"
#include "switch/services/auddev.h"
#include "switch/services/hwopus.h"
#include "switch/services/csrng.h"
#include "switch/services/lbl.h"
#include "switch/services/i2c.h"
#include "switch/services/gpio.h"
#include "switch/services/bpc.h"
#include "switch/services/pcv.h"
#include "switch/services/clkrst.h"
#include "switch/services/fan.h"
#include "switch/services/psm.h"
#include "switch/services/spsm.h"
//#include "switch/services/bsd.h" Use <sys/socket.h> instead
//#include "switch/services/sfdnsres.h" Use <netdb.h> instead
#include "switch/services/fatal.h"
#include "switch/services/time.h"
#include "switch/services/usb.h"
#include "switch/services/usbds.h"
#include "switch/services/usbhs.h"
#include "switch/services/hid.h"
#include "switch/services/hidbus.h"
#include "switch/services/hiddbg.h"
#include "switch/services/hidsys.h"
#include "switch/services/irs.h"
#include "switch/services/pl.h"
#include "switch/services/vi.h"
#include "switch/services/nv.h"
#include "switch/services/nifm.h"
#include "switch/services/ns.h"
#include "switch/services/ldr.h"
#include "switch/services/ro.h"
#include "switch/services/tc.h"
#include "switch/services/ts.h"
#include "switch/services/pm.h"
#include "switch/services/set.h"
#include "switch/services/lr.h"
#include "switch/services/spl.h"
#include "switch/services/ncm.h"
#include "switch/services/psc.h"
#include "switch/services/caps.h"
#include "switch/services/capsa.h"
#include "switch/services/capsc.h"
#include "switch/services/capsdc.h"
#include "switch/services/capsu.h"
#include "switch/services/capssc.h"
#include "switch/services/capssu.h"
#include "switch/services/nfc.h"
#include "switch/services/wlaninf.h"
#include "switch/services/pctl.h"
#include "switch/services/pdm.h"
#include "switch/services/grc.h"
#include "switch/services/friends.h"
#include "switch/services/notif.h"

#include "switch/display/binder.h"
#include "switch/display/parcel.h"
#include "switch/display/buffer_producer.h"
#include "switch/display/native_window.h"
#include "switch/display/framebuffer.h"

#include "switch/nvidia/ioctl.h"
#include "switch/nvidia/graphic_buffer.h"
#include "switch/nvidia/fence.h"
#include "switch/nvidia/map.h"
#include "switch/nvidia/address_space.h"
#include "switch/nvidia/channel.h"
#include "switch/nvidia/gpu.h"
#include "switch/nvidia/gpu_channel.h"

#include "switch/audio/driver.h"

#include "switch/applets/libapplet.h"
#include "switch/applets/album_la.h"
#include "switch/applets/friends_la.h"
#include "switch/applets/hid_la.h"
#include "switch/applets/pctlauth.h"
#include "switch/applets/psel.h"
#include "switch/applets/error.h"
#include "switch/applets/swkbd.h"
#include "switch/applets/web.h"

#include "switch/runtime/env.h"
#include "switch/runtime/hosversion.h"
#include "switch/runtime/nxlink.h"
#include "switch/runtime/resolver.h"
#include "switch/runtime/ringcon.h"

#include "switch/runtime/util/utf.h"

#include "switch/runtime/devices/console.h"
#include "switch/runtime/devices/usb_comms.h"
#include "switch/runtime/devices/fs_dev.h"
#include "switch/runtime/devices/romfs_dev.h"
#include "switch/runtime/devices/socket.h"

#include "switch/crypto/aes.h"
#include "switch/crypto/aes_cbc.h"
#include "switch/crypto/aes_ctr.h"
#include "switch/crypto/aes_xts.h"
#include "switch/crypto/cmac.h"

#include "switch/crypto/sha256.h"
#include "switch/crypto/sha1.h"
#include "switch/crypto/hmac.h"

#include "switch/crypto/crc.h"

#ifdef __cplusplus
}
#endif

