#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/ipc.h"
#include "runtime/hosversion.h"
#include "services/pcv.h"
#include "services/sm.h"

static Service g_pcvSrv;
static u64 g_refCnt;

Result pcvInitialize(void) {
    Result rc = 0;
    
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_pcvSrv))
        return 0;

    rc = smGetService(&g_pcvSrv, "pcv");

    if (R_FAILED(rc)) pcvExit();

    return rc;
}

void pcvExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        serviceClose(&g_pcvSrv);
    }
}

Result pcvGetModuleId(PcvModuleId *module_id, PcvModule module) {
    static const PcvModuleId s_moduleIdMap[PcvModule_Count] = {
        PcvModuleId_CpuBus,         PcvModuleId_GPU,            PcvModuleId_I2S1,               PcvModuleId_I2S2,
        PcvModuleId_I2S3,           PcvModuleId_PWM,            PcvModuleId_I2C1,               PcvModuleId_I2C2,
        PcvModuleId_I2C3,           PcvModuleId_I2C4,           PcvModuleId_I2C5,               PcvModuleId_I2C6,
        PcvModuleId_SPI1,           PcvModuleId_SPI2,           PcvModuleId_SPI3,               PcvModuleId_SPI4,
        PcvModuleId_DISP1,          PcvModuleId_DISP2,          PcvModuleId_ISP,                PcvModuleId_VI,
        PcvModuleId_SDMMC1,         PcvModuleId_SDMMC2,         PcvModuleId_SDMMC3,             PcvModuleId_SDMMC4,
        PcvModuleId_OWR,            PcvModuleId_CSITE,          PcvModuleId_TSEC,               PcvModuleId_MSELECT,
        PcvModuleId_HDA2CODEC_2X,   PcvModuleId_ACTMON,         PcvModuleId_I2C_SLOW,           PcvModuleId_SOR1,
        PcvModuleId_SATA,           PcvModuleId_HDA,            PcvModuleId_XUSB_CORE_HOST,     PcvModuleId_XUSB_FALCON,
        PcvModuleId_XUSB_FS,        PcvModuleId_XUSB_CORE_DEV,  PcvModuleId_XUSB_SS_HOSTDEV,    PcvModuleId_UARTA,
        PcvModuleId_UARTB,          PcvModuleId_UARTC,          PcvModuleId_UARTD,              PcvModuleId_HOST1X,
        PcvModuleId_ENTROPY,        PcvModuleId_SOC_THERM,      PcvModuleId_VIC,                PcvModuleId_NVENC,
        PcvModuleId_NVJPG,          PcvModuleId_NVDEC,          PcvModuleId_QSPI,               PcvModuleId_VI_I2C,
        PcvModuleId_TSECB,          PcvModuleId_APE,            PcvModuleId_ACLK,               PcvModuleId_UARTAPE,
        PcvModuleId_EMC,            PcvModuleId_PLLE0_0,        PcvModuleId_PLLE0_1,            PcvModuleId_DSI,
        PcvModuleId_MAUD,           PcvModuleId_DPAUX1,         PcvModuleId_MIPI_CAL,           PcvModuleId_UART_FST_MIPI_CAL,
        PcvModuleId_OSC,            PcvModuleId_SCLK,           PcvModuleId_SOR_SAFE,           PcvModuleId_XUSB_SS,
        PcvModuleId_XUSB_HOST,      PcvModuleId_XUSB_DEV,       PcvModuleId_EXTPERIPH1,         PcvModuleId_AHUB,
        PcvModuleId_HDA2HDMICODEC,  PcvModuleId_PLLP5,          PcvModuleId_USBD,               PcvModuleId_USB2,
        PcvModuleId_PCIE,           PcvModuleId_AFI,            PcvModuleId_PCIEXCLK,           PcvModuleId_PEX_USB_UPHY,
        PcvModuleId_XUSB_PADCTL,    PcvModuleId_APBDMA,         PcvModuleId_USB2_TRK,           PcvModuleId_PLLE0_2,
        PcvModuleId_PLLE0_3,        PcvModuleId_CEC,            PcvModuleId_EXTPERIPH2,
    };

    if (module >= PcvModule_Count) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    *module_id = s_moduleIdMap[module];
    return 0;
}

Result pcvSetClockRate(PcvModule module, u32 hz) {
    if(hosversionAtLeast(8,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 module;
        u32 hz;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pcvSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->module = module;
    raw->hz = hz;

    Result rc = serviceIpcDispatch(&g_pcvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_pcvSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result pcvGetClockRate(PcvModule module, u32 *out_hz) {
    if(hosversionAtLeast(8,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 module;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pcvSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 3;
    raw->module = module;

    Result rc = serviceIpcDispatch(&g_pcvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 hz;
        } *resp;

        serviceIpcParse(&g_pcvSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        
        if (R_SUCCEEDED(rc)) {
            *out_hz = resp->hz;
        }
    }

    return rc;
}

Result pcvSetVoltageEnabled(bool state, u32 voltage) {
    if(hosversionAtLeast(8,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 state;
        u32 voltage;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pcvSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 8;
    raw->state = (u8)state;
    raw->voltage = voltage;

    Result rc = serviceIpcDispatch(&g_pcvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_pcvSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result pcvGetVoltageEnabled(bool *isEnabled, u32 voltage) {
    if(hosversionAtLeast(8,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 voltage;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_pcvSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 9;
    raw->voltage = voltage;

    Result rc = serviceIpcDispatch(&g_pcvSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 isEnabled;
        } *resp;

        serviceIpcParse(&g_pcvSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if(R_SUCCEEDED(rc) && isEnabled) {
            *isEnabled = (bool)resp->isEnabled;
        }
    }

    return rc;
}
