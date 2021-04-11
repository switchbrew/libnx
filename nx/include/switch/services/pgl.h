/**
 * @file pgl.h
 * @brief PGL service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../sf/tipc.h"
#include "../services/ncm_types.h"
#include "../services/pm.h"

/// LaunchFlag
typedef enum {
    PglLaunchFlag_None                                     = 0,
    PglLaunchFlag_EnableDetailedCrashReport                = BIT(0),
    PglLaunchFlag_EnableCrashReportScreenShotForProduction = BIT(1),
    PglLaunchFlag_EnableCrashReportScreenShotForDevelop    = BIT(2),
} PglLaunchFlag;

/// SnapShotDumpType
typedef enum {
    PglSnapShotDumpType_None = 0,
    PglSnapShotDumpType_Auto = 1,
    PglSnapShotDumpType_Full = 2,
} PglSnapShotDumpType;

typedef struct {
    u64 id;            ///< Program Id
    u32 version;       ///< Version
    u8 content_type;   ///< NcmContentType
    u8 id_offset;      ///< Id Offset
    u8 reserved_0E[2]; ///< Padding
} PglContentMetaInfo;

typedef union {
    Service s;
    TipcService t;
} PglEventObserver;

/// Initialize pgl.
Result pglInitialize(void);

/// Exit pgl.
void pglExit(void);

/// Gets the Service object for the actual pgl service session. Requires < 12.0.0
Service* pglGetServiceSessionCmif(void);

/// Gets the TipcService object for the actual pgl service session. Requires 12.0.0+
TipcService* pglGetServiceSessionTipc(void);

Result pglLaunchProgram(u64 *out_pid, const NcmProgramLocation *loc, u32 pm_launch_flags, u8 pgl_launch_flags);
Result pglTerminateProcess(u64 pid);
Result pglLaunchProgramFromHost(u64 *out_pid, const char *content_path, u32 pm_launch_flags);
Result pglGetHostContentMetaInfo(PglContentMetaInfo *out, const char *content_path);
Result pglGetApplicationProcessId(u64 *out);
Result pglBoostSystemMemoryResourceLimit(u64 size);
Result pglIsProcessTracked(bool *out, u64 pid);
Result pglEnableApplicationCrashReport(bool en);
Result pglIsApplicationCrashReportEnabled(bool *out);
Result pglEnableApplicationAllThreadDumpOnCrash(bool en);
Result pglTriggerApplicationSnapShotDumper(PglSnapShotDumpType dump_type, const char *arg);
Result pglGetEventObserver(PglEventObserver *out);

Result pglEventObserverGetProcessEvent(PglEventObserver *observer, Event *out);
Result pglEventObserverGetProcessEventInfo(PglEventObserver *observer, PmProcessEventInfo *out);
void   pglEventObserverClose(PglEventObserver *observer);
