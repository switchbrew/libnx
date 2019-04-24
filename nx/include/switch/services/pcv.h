/**
 * @file pcv.h
 * @brief PCV service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

typedef enum {
    PcvModule_Cpu = 0,
    PcvModule_Gpu = 1,
    PcvModule_Emc = 56,
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
    PcvModuleId_SDMMC1              = 0x40000015,
    PcvModuleId_SDMMC2              = 0x40000016,
    PcvModuleId_SDMMC3              = 0x40000017,
    PcvModuleId_SDMMC4              = 0x40000018,
    PcvModuleId_CSITE               = 0x4000001A,
    PcvModuleId_TSEC                = 0x4000001B,
    PcvModuleId_MSELECT             = 0x4000001C,
    PcvModuleId_HDA2CODEC_2X        = 0x4000001D,
    PcvModuleId_ACTMON              = 0x4000001E,
    PcvModuleId_I2C_SLOW            = 0x4000001F,
    PcvModuleId_SOR1                = 0x40000020,
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

Result pcvInitialize(void);
void pcvExit(void);

Result pcvGetClockRate(PcvModule module, u32 *out_hz);
Result pcvSetClockRate(PcvModule module, u32 hz);
Result pcvSetVoltageEnabled(bool state, u32 voltage);
Result pcvGetVoltageEnabled(bool *isEnabled, u32 voltage);
