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
    u64 id;                     ///< Program Id
    u32 version;                ///< Version
    u8 content_type;            ///< NcmContentType
    u8 id_offset;               ///< Id Offset
    u8 content_meta_type;       ///< NcmContentMetaType
    u8 reserved_0F;             ///< Padding
} PglProgramLaunchProperty;

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
Result pglGetHostProgramLaunchProperty(PglProgramLaunchProperty *out, const char *content_path);
Result pglGetRunningApplicationProcessId(u64 *out_pid);
Result pglBoostSystemMemoryResourceLimit(u64 size);
Result pglIsRunningProcess(bool *out, u64 pid);
Result pglEnableApplicationCrashReport(bool en);
Result pglIsApplicationCrashReportEnabled(bool *out);
Result pglEnableApplicationAllThreadDumpOnCrash(bool en);
Result pglGetProcessId(u64 *out_pid, u64 program_id); ///< [19.0.0+]
Result pglTriggerSnapShotDumper(PglSnapShotDumpType dump_type, const char *arg); ///< [10.0.0-11.0.1]
Result pglCreateShellEvent(PglEventObserver *out);
Result pglEnableApplicationCrashReport2(u64 pid, bool en); ///< [23.0.0+]

Result pglEventObserverGetShellEvent(PglEventObserver *observer, Event *out);
Result pglEventObserverGetShellEventInfo(PglEventObserver *observer, PmProcessEventInfo *out);
void   pglEventObserverClose(PglEventObserver *observer);
