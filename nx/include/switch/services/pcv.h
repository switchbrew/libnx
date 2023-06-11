/**
 * @file pcv.h
 * @brief PCV service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

typedef enum {
    PcvModule_CpuBus            = 0,
    PcvModule_GPU               = 1,
    PcvModule_I2S1              = 2,
    PcvModule_I2S2              = 3,
    PcvModule_I2S3              = 4,
    PcvModule_PWM               = 5,
    PcvModule_I2C1              = 6,
    PcvModule_I2C2              = 7,
    PcvModule_I2C3              = 8,
    PcvModule_I2C4              = 9,
    PcvModule_I2C5              = 10,
    PcvModule_I2C6              = 11,
    PcvModule_SPI1              = 12,
    PcvModule_SPI2              = 13,
    PcvModule_SPI3              = 14,
    PcvModule_SPI4              = 15,
    PcvModule_DISP1             = 16,
    PcvModule_DISP2             = 17,
    PcvModule_ISP               = 18,
    PcvModule_VI                = 19,
    PcvModule_SDMMC1            = 20,
    PcvModule_SDMMC2            = 21,
    PcvModule_SDMMC3            = 22,
    PcvModule_SDMMC4            = 23,
    PcvModule_OWR               = 24,
    PcvModule_CSITE             = 25,
    PcvModule_TSEC              = 26,
    PcvModule_MSELECT           = 27,
    PcvModule_HDA2CODEC_2X      = 28,
    PcvModule_ACTMON            = 29,
    PcvModule_I2C_SLOW          = 30,
    PcvModule_SOR1              = 31,
    PcvModule_SATA              = 32,
    PcvModule_HDA               = 33,
    PcvModule_XUSB_CORE_HOST    = 34,
    PcvModule_XUSB_FALCON       = 35,
    PcvModule_XUSB_FS           = 36,
    PcvModule_XUSB_CORE_DEV     = 37,
    PcvModule_XUSB_SS_HOSTDEV   = 38,
    PcvModule_UARTA             = 39,
    PcvModule_UARTB             = 40,
    PcvModule_UARTC             = 41,
    PcvModule_UARTD             = 42,
    PcvModule_HOST1X            = 43,
    PcvModule_ENTROPY           = 44,
    PcvModule_SOC_THERM         = 45,
    PcvModule_VIC               = 46,
    PcvModule_NVENC             = 47,
    PcvModule_NVJPG             = 48,
    PcvModule_NVDEC             = 49,
    PcvModule_QSPI              = 50,
    PcvModule_VI_I2C            = 51,
    PcvModule_TSECB             = 52,
    PcvModule_APE               = 53,
    PcvModule_ACLK              = 54,
    PcvModule_UARTAPE           = 55,
    PcvModule_EMC               = 56,
    PcvModule_PLLE0_0           = 57,
    PcvModule_PLLE0_1           = 58,
    PcvModule_DSI               = 59,
    PcvModule_MAUD              = 60,
    PcvModule_DPAUX1            = 61,
    PcvModule_MIPI_CAL          = 62,
    PcvModule_UART_FST_MIPI_CAL = 63,
    PcvModule_OSC               = 64,
    PcvModule_SCLK              = 65,
    PcvModule_SOR_SAFE          = 66,
    PcvModule_XUSB_SS           = 67,
    PcvModule_XUSB_HOST         = 68,
    PcvModule_XUSB_DEV          = 69,
    PcvModule_EXTPERIPH1        = 70,
    PcvModule_AHUB              = 71,
    PcvModule_HDA2HDMICODEC     = 72,
    PcvModule_PLLP5             = 73,
    PcvModule_USBD              = 74,
    PcvModule_USB2              = 75,
    PcvModule_PCIE              = 76,
    PcvModule_AFI               = 77,
    PcvModule_PCIEXCLK          = 78,
    PcvModule_PEX_USB_UPHY      = 79,
    PcvModule_XUSB_PADCTL       = 80,
    PcvModule_APBDMA            = 81,
    PcvModule_USB2_TRK          = 82,
    PcvModule_PLLE0_2           = 83,
    PcvModule_PLLE0_3           = 84,
    PcvModule_CEC               = 85,
    PcvModule_EXTPERIPH2        = 86,
    PcvModule_Count // Not a real module, used to know how many modules there are.
} PcvModule;

/// Module id returned by [8.0.0+] pcv services
/// See also: https://switchbrew.org/wiki/PCV_services#Modules
typedef enum {
    PcvModuleId_CpuBus              = 0x40000001,
    PcvModuleId_GPU                 = 0x40000002,
    PcvModuleId_I2S1                = 0x40000003,
    PcvModuleId_I2S2                = 0x40000004,
    PcvModuleId_I2S3                = 0x40000005,
    PcvModuleId_PWM                 = 0x40000006,
    PcvModuleId_I2C1                = 0x02000001,
    PcvModuleId_I2C2                = 0x02000002,
    PcvModuleId_I2C3                = 0x02000003,
    PcvModuleId_I2C4                = 0x02000004,
    PcvModuleId_I2C5                = 0x02000005,
    PcvModuleId_I2C6                = 0x02000006,
    PcvModuleId_SPI1                = 0x07000000,
    PcvModuleId_SPI2                = 0x07000001,
    PcvModuleId_SPI3                = 0x07000002,
    PcvModuleId_SPI4                = 0x07000003,
    PcvModuleId_DISP1               = 0x40000011,
    PcvModuleId_DISP2               = 0x40000012,
    PcvModuleId_ISP                 = 0x40000013,
    PcvModuleId_VI                  = 0x40000014,
    PcvModuleId_SDMMC1              = 0x40000015,
    PcvModuleId_SDMMC2              = 0x40000016,
    PcvModuleId_SDMMC3              = 0x40000017,
    PcvModuleId_SDMMC4              = 0x40000018,
    PcvModuleId_OWR                 = 0x40000019,
    PcvModuleId_CSITE               = 0x4000001A,
    PcvModuleId_TSEC                = 0x4000001B,
    PcvModuleId_MSELECT             = 0x4000001C,
    PcvModuleId_HDA2CODEC_2X        = 0x4000001D,
    PcvModuleId_ACTMON              = 0x4000001E,
    PcvModuleId_I2C_SLOW            = 0x4000001F,
    PcvModuleId_SOR1                = 0x40000020,
    PcvModuleId_SATA                = 0x40000021,
    PcvModuleId_HDA                 = 0x40000022,
    PcvModuleId_XUSB_CORE_HOST      = 0x40000023,
    PcvModuleId_XUSB_FALCON         = 0x40000024,
    PcvModuleId_XUSB_FS             = 0x40000025,
    PcvModuleId_XUSB_CORE_DEV       = 0x40000026,
    PcvModuleId_XUSB_SS_HOSTDEV     = 0x40000027,
    PcvModuleId_UARTA               = 0x03000001,
    PcvModuleId_UARTB               = 0x35000405,
    PcvModuleId_UARTC               = 0x3500040F,
    PcvModuleId_UARTD               = 0x37000001,
    PcvModuleId_HOST1X              = 0x4000002C,
    PcvModuleId_ENTROPY             = 0x4000002D,
    PcvModuleId_SOC_THERM           = 0x4000002E,
    PcvModuleId_VIC                 = 0x4000002F,
    PcvModuleId_NVENC               = 0x40000030,
    PcvModuleId_NVJPG               = 0x40000031,
    PcvModuleId_NVDEC               = 0x40000032,
    PcvModuleId_QSPI                = 0x40000033,
    PcvModuleId_VI_I2C              = 0x40000034,
    PcvModuleId_TSECB               = 0x40000035,
    PcvModuleId_APE                 = 0x40000036,
    PcvModuleId_ACLK                = 0x40000037,
    PcvModuleId_UARTAPE             = 0x40000038,
    PcvModuleId_EMC                 = 0x40000039,
    PcvModuleId_PLLE0_0             = 0x4000003A,
    PcvModuleId_PLLE0_1             = 0x4000003B,
    PcvModuleId_DSI                 = 0x4000003C,
    PcvModuleId_MAUD                = 0x4000003D,
    PcvModuleId_DPAUX1              = 0x4000003E,
    PcvModuleId_MIPI_CAL            = 0x4000003F,
    PcvModuleId_UART_FST_MIPI_CAL   = 0x40000040,
    PcvModuleId_OSC                 = 0x40000041,
    PcvModuleId_SCLK                = 0x40000042,
    PcvModuleId_SOR_SAFE            = 0x40000043,
    PcvModuleId_XUSB_SS             = 0x40000044,
    PcvModuleId_XUSB_HOST           = 0x40000045,
    PcvModuleId_XUSB_DEV            = 0x40000046,
    PcvModuleId_EXTPERIPH1          = 0x40000047,
    PcvModuleId_AHUB                = 0x40000048,
    PcvModuleId_HDA2HDMICODEC       = 0x40000049,
    PcvModuleId_PLLP5               = 0x4000004A,
    PcvModuleId_USBD                = 0x4000004B,
    PcvModuleId_USB2                = 0x4000004C,
    PcvModuleId_PCIE                = 0x4000004D,
    PcvModuleId_AFI                 = 0x4000004E,
    PcvModuleId_PCIEXCLK            = 0x4000004F,
    PcvModuleId_PEX_USB_UPHY        = 0x40000050,
    PcvModuleId_XUSB_PADCTL         = 0x40000051,
    PcvModuleId_APBDMA              = 0x40000052,
    PcvModuleId_USB2_TRK            = 0x40000053,
    PcvModuleId_PLLE0_2             = 0x40000054,
    PcvModuleId_PLLE0_3             = 0x40000055,
    PcvModuleId_CEC                 = 0x40000056,
    PcvModuleId_EXTPERIPH2          = 0x40000057,
} PcvModuleId;

// Clock list type returned by GetPossibleClockRates
typedef enum {
    PcvClockRatesListType_Invalid  = 0,
    PcvClockRatesListType_Discrete = 1,
    PcvClockRatesListType_Range    = 2,
} PcvClockRatesListType;

/// Initialize pcv.
Result pcvInitialize(void);

/// Exit pcv.
void pcvExit(void);

/// Gets the Service object for the actual pcv service session.
Service* pcvGetServiceSession(void);

Result pcvGetModuleId(PcvModuleId *module_id, PcvModule module);

/// Only available on [1.0.0-7.0.1].
Result pcvGetClockRate(PcvModule module, u32 *out_hz);
/// Only available on [1.0.0-7.0.1].
Result pcvSetClockRate(PcvModule module, u32 hz);
/// Only available on [1.0.0-7.0.1].
Result pcvSetVoltageEnabled(u32 power_domain, bool state);
/// Only available on [1.0.0-7.0.1].
Result pcvGetVoltageEnabled(bool *isEnabled, u32 power_domain);
/// Only available on [1.0.0-7.0.1].
Result pcvGetPossibleClockRates(PcvModule module, u32 *rates, s32 max_count, PcvClockRatesListType *out_type, s32 *out_count);
