#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/pcv.h"
#include "runtime/hosversion.h"

static Service g_pcvSrv;

NX_GENERATE_SERVICE_GUARD(pcv);

Result _pcvInitialize(void) {
    return smGetService(&g_pcvSrv, "pcv");
}

void _pcvCleanup(void) {
    serviceClose(&g_pcvSrv);
}

Service* pcvGetServiceSession(void) {
    return &g_pcvSrv;
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
    if(hosversionAtLeast(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u32 module;
        u32 hz;
    } in = { module, hz };
    return serviceDispatchIn(&g_pcvSrv, 2, in);
}

Result pcvGetClockRate(PcvModule module, u32 *out_hz) {
    if(hosversionAtLeast(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const u32 in = module;
    return serviceDispatchInOut(&g_pcvSrv, 3, in, *out_hz);
}

Result pcvSetVoltageEnabled(u32 power_domain, bool state) {
    if(hosversionAtLeast(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u8 state;
        u32 power_domain;
    } in = { state != 0, power_domain };
    return serviceDispatchIn(&g_pcvSrv, 8, in);
}

Result pcvGetVoltageEnabled(bool *isEnabled, u32 power_domain) {
    if(hosversionAtLeast(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u8 tmp = 0;
    Result rc = serviceDispatchInOut(&g_pcvSrv, 9, power_domain, tmp);

    if (R_SUCCEEDED(rc) && isEnabled) *isEnabled = tmp & 1;

    return rc;
}
