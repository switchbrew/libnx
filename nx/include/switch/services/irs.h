/**
 * @file irs.h
 * @brief HID IR sensor (irs) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once

#include "../types.h"
#include "../sf/service.h"
#include "../services/hid.h"

#define IRS_MAX_CAMERAS 0x9

/// IrCameraStatus
typedef enum {
    IrsIrCameraStatus_Available   = 0,    ///< Available
    IrsIrCameraStatus_Unsupported = 1,    ///< Unsupported
    IrsIrCameraStatus_Unconnected = 2,    ///< Unconnected
} IrsIrCameraStatus;

/// IrCameraInternalStatus
typedef enum {
    IrsIrCameraInternalStatus_Stopped                  = 0,        ///< Stopped
    IrsIrCameraInternalStatus_FirmwareUpdateNeeded     = 1,        ///< FirmwareUpdateNeeded
    IrsIrCameraInternalStatus_Unknown2                 = 2,        ///< Unknown
    IrsIrCameraInternalStatus_Unknown3                 = 3,        ///< Unknown
    IrsIrCameraInternalStatus_Unknown4                 = 4,        ///< Unknown
    IrsIrCameraInternalStatus_FirmwareVersionRequested = 5,        ///< FirmwareVersionRequested
    IrsIrCameraInternalStatus_FirmwareVersionIsInvalid = 6,        ///< FirmwareVersionIsInvalid
    IrsIrCameraInternalStatus_Ready                    = 7,        ///< [4.0.0+] Ready
    IrsIrCameraInternalStatus_Setting                  = 8,        ///< [4.0.0+] Setting
} IrsIrCameraInternalStatus;

/// IrSensorMode
typedef enum {
    IrsIrSensorMode_None                   = 0,    ///< None
    IrsIrSensorMode_MomentProcessor        = 1,    ///< MomentProcessor
    IrsIrSensorMode_ClusteringProcessor    = 2,    ///< ClusteringProcessor
    IrsIrSensorMode_ImageTransferProcessor = 3,    ///< ImageTransferProcessor
    IrsIrSensorMode_PointingProcessor      = 4,    ///< PointingProcessor
    IrsIrSensorMode_TeraPluginProcessor    = 5,    ///< TeraPluginProcessor
    IrsIrSensorMode_IrLedProcessor         = 6,    ///< IrLedProcessor (doesn't apply to IrsDeviceFormat::ir_sensor_mode)
} IrsIrSensorMode;

/// ImageProcessorStatus
typedef enum {
    IrsImageProcessorStatus_Stopped = 0,           ///< Stopped
    IrsImageProcessorStatus_Running = 1,           ///< Running
} IrsImageProcessorStatus;

/// ImageTransferProcessorFormat. IR Sensor image resolution.
typedef enum {
    IrsImageTransferProcessorFormat_320x240  = 0,  ///< 320x240
    IrsImageTransferProcessorFormat_160x120  = 1,  ///< 160x120
    IrsImageTransferProcessorFormat_80x60    = 2,  ///< 80x60
    IrsImageTransferProcessorFormat_40x30    = 3,  ///< [4.0.0+] 40x30
    IrsImageTransferProcessorFormat_20x15    = 4,  ///< [4.0.0+] 20x15
} IrsImageTransferProcessorFormat;

/// AdaptiveClusteringMode
typedef enum {
    IrsAdaptiveClusteringMode_StaticFov    = 0,  ///< StaticFov
    IrsAdaptiveClusteringMode_DynamicFov   = 1,  ///< DynamicFov
} IrsAdaptiveClusteringMode;

/// AdaptiveClusteringTargetDistance
typedef enum {
    IrsAdaptiveClusteringTargetDistance_Near   = 0,   ///< Near
    IrsAdaptiveClusteringTargetDistance_Middle = 1,   ///< Middle
    IrsAdaptiveClusteringTargetDistance_Far    = 2,   ///< Far
} IrsAdaptiveClusteringTargetDistance;

/// HandAnalysisMode
typedef enum {
    IrsHandAnalysisMode_Silhouette         = 1,   ///< Silhouette
    IrsHandAnalysisMode_Image              = 2,   ///< Image
    IrsHandAnalysisMode_SilhouetteAndImage = 3,   ///< SilhouetteAndImage
    IrsHandAnalysisMode_SilhouetteOnly     = 4,   ///< [4.0.0+] SilhouetteOnly
} IrsHandAnalysisMode;

/// Internal validation callblack.
typedef bool (*IrsValidationCb)(void* userdata, void* arg);

/// IrCameraHandle
typedef struct {
    u8 player_number;       ///< PlayerNumber
    u8 device_type;         ///< DeviceType
    u8 reserved[0x2];       ///< Reserved
} IrsIrCameraHandle;

/// PackedMcuVersion
typedef struct {
    u16 major_version;      ///< MajorVersion
    u16 minor_version;      ///< MinorVersion
} IrsPackedMcuVersion;

/// PackedFunctionLevel
typedef struct {
    u8 ir_sensor_function_level;  ///< IrSensorFunctionLevel
    u8 reserved[0x3];             ///< Reserved
} IrsPackedFunctionLevel;

/// Rect
typedef struct {
    s16 x;                                      ///< X
    s16 y;                                      ///< Y
    s16 width;                                  ///< Width
    s16 height;                                 ///< Height
} IrsRect;

/// IrsMomentProcessorConfig
typedef struct {
    u64 exposure_time;                          ///< IR Sensor exposure time in nanoseconds.
    u32 light_target;                           ///< Controls the IR leds. 0: All leds, 1: Bright group, 2: Dim group, 3: None.
    u32 gain;                                   ///< IR sensor signal's digital gain.
    u8 is_negative_image_used;                  ///< Inverts the colors of the captured image. 0: Normal image, 1: Negative image.
    u8 reserved[0x7];                           ///< Reserved.
    IrsRect window_of_interest;                 ///< WindowOfInterest
    u32 preprocess;                             ///< Preprocess
    u32 preprocess_intensity_threshold;         ///< PreprocessIntensityThreshold
} IrsMomentProcessorConfig;

/// PackedMomentProcessorConfig
typedef struct {
    u64 exposure_time;                          ///< IR Sensor exposure time in nanoseconds.
    u8 light_target;                            ///< Controls the IR leds. 0: All leds, 1: Bright group, 2: Dim group, 3: None.
    u8 gain;                                    ///< IR sensor signal's digital gain.
    u8 is_negative_image_used;                  ///< Inverts the colors of the captured image. 0: Normal image, 1: Negative image.
    u8 reserved[0x5];                           ///< Reserved.
    IrsRect window_of_interest;                 ///< WindowOfInterest
    IrsPackedMcuVersion required_mcu_version;   ///< RequiredMcuVersion
    u8 preprocess;                              ///< Preprocess
    u8 preprocess_intensity_threshold;          ///< PreprocessIntensityThreshold
    u8 reserved2[0x2];                          ///< Reserved.
} IrsPackedMomentProcessorConfig;

/// ClusteringProcessorConfig
typedef struct {
    u64 exposure_time;                          ///< IR Sensor exposure time in nanoseconds.
    u32 light_target;                           ///< Controls the IR leds. 0: All leds, 1: Bright group, 2: Dim group, 3: None.
    u32 gain;                                   ///< IR sensor signal's digital gain.
    u8 is_negative_image_used;                  ///< Inverts the colors of the captured image. 0: Normal image, 1: Negative image.
    u8 reserved[0x7];                           ///< Reserved.
    IrsRect window_of_interest;                 ///< WindowOfInterest
    u32 object_pixel_count_min;                 ///< ObjectPixelCountMin
    u32 object_pixel_count_max;                 ///< ObjectPixelCountMax
    u32 object_intensity_min;                   ///< ObjectIntensityMin
    u8 is_external_light_filter_enabled;        ///< IsExternalLightFilterEnabled
} IrsClusteringProcessorConfig;

/// PackedClusteringProcessorConfig
typedef struct {
    u64 exposure_time;                          ///< IR Sensor exposure time in nanoseconds.
    u8 light_target;                            ///< Controls the IR leds. 0: All leds, 1: Bright group, 2: Dim group, 3: None.
    u8 gain;                                    ///< IR sensor signal's digital gain.
    u8 is_negative_image_used;                  ///< Inverts the colors of the captured image. 0: Normal image, 1: Negative image.
    u8 reserved[0x5];                           ///< Reserved.
    IrsRect window_of_interest;                 ///< WindowOfInterest
    IrsPackedMcuVersion required_mcu_version;   ///< RequiredMcuVersion
    u32 object_pixel_count_min;                 ///< ObjectPixelCountMin
    u32 object_pixel_count_max;                 ///< ObjectPixelCountMax
    u8 object_intensity_min;                    ///< ObjectIntensityMin
    u8 is_external_light_filter_enabled;        ///< IsExternalLightFilterEnabled
    u8 reserved2[0x2];                          ///< Reserved.
} IrsPackedClusteringProcessorConfig;

/// ImageTransferProcessorConfig
typedef struct {
    u64 exposure_time;          ///< IR Sensor exposure time in nanoseconds.
    u32 light_target;           ///< Controls the IR leds. 0: All leds, 1: Bright group, 2: Dim group, 3: None.
    u32 gain;                   ///< IR sensor signal's digital gain.
    u8 is_negative_image_used;  ///< Inverts the colors of the captured image. 0: Normal image, 1: Negative image.
    u8 reserved[0x7];           ///< Reserved.
    u32 format;                 ///< \ref IrsImageTransferProcessorFormat
} IrsImageTransferProcessorConfig;

/// ImageTransferProcessorExConfig
typedef struct {
    u64 exposure_time;                          ///< IR Sensor exposure time in nanoseconds.
    u32 light_target;                           ///< Controls the IR leds. 0: All leds, 1: Bright group, 2: Dim group, 3: None.
    u32 gain;                                   ///< IR sensor signal's digital gain.
    u8 is_negative_image_used;                  ///< Inverts the colors of the captured image. 0: Normal image, 1: Negative image.
    u8 reserved[0x7];                           ///< Reserved.
    u32 orig_format;                            ///< OrigFormat \ref IrsImageTransferProcessorFormat
    u32 trimming_format;                        ///< TrimmingFormat \ref IrsImageTransferProcessorFormat
    u16 trimming_start_x;                       ///< TrimmingStartX
    u16 trimming_start_y;                       ///< TrimmingStartY
    u8 is_external_light_filter_enabled;        ///< IsExternalLightFilterEnabled
} IrsImageTransferProcessorExConfig;

/// PackedImageTransferProcessorConfig
typedef struct {
    u64 exposure_time;                          ///< IR Sensor exposure time in nanoseconds.
    u8 light_target;                            ///< Controls the IR leds. 0: All leds, 1: Bright group, 2: Dim group, 3: None.
    u8 gain;                                    ///< IR sensor signal's digital gain.
    u8 is_negative_image_used;                  ///< Inverts the colors of the captured image. 0: Normal image, 1: Negative image.
    u8 reserved[0x5];                           ///< Reserved.
    IrsPackedMcuVersion required_mcu_version;   ///< RequiredMcuVersion
    u8 format;                                  ///< \ref IrsImageTransferProcessorFormat
    u8 reserved2[0x3];                          ///< Reserved.
} IrsPackedImageTransferProcessorConfig;

/// PackedImageTransferProcessorExConfig
typedef struct {
    u64 exposure_time;                          ///< IR Sensor exposure time in nanoseconds.
    u8 light_target;                            ///< Controls the IR leds. 0: All leds, 1: Bright group, 2: Dim group, 3: None.
    u8 gain;                                    ///< IR sensor signal's digital gain.
    u8 is_negative_image_used;                  ///< Inverts the colors of the captured image. 0: Normal image, 1: Negative image.
    u8 reserved[0x5];                           ///< Reserved.
    IrsPackedMcuVersion required_mcu_version;   ///< RequiredMcuVersion
    u8 orig_format;                             ///< OrigFormat \ref IrsImageTransferProcessorFormat
    u8 trimming_format;                         ///< TrimmingFormat \ref IrsImageTransferProcessorFormat
    u16 trimming_start_x;                       ///< TrimmingStartX
    u16 trimming_start_y;                       ///< TrimmingStartY
    u8 is_external_light_filter_enabled;        ///< IsExternalLightFilterEnabled
    u8 reserved2[0x5];                          ///< Reserved.
} IrsPackedImageTransferProcessorExConfig;

/// ImageTransferProcessorState
typedef struct {
    u64 sampling_number;        ///< SamplingNumber
    u32 ambient_noise_level;    ///< AmbientNoiseLevel
    u8 reserved[0x4];           ///< Reserved
} IrsImageTransferProcessorState;

/// PackedPointingProcessorConfig
typedef struct {
    IrsRect window_of_interest;                ///< WindowOfInterest
    IrsPackedMcuVersion required_mcu_version;  ///< RequiredMcuVersion
} IrsPackedPointingProcessorConfig;

/// TeraPluginProcessorConfig
typedef struct {
    u8 mode;                                   ///< Mode
    u8 unk_x1;                                 ///< [6.0.0+] Unknown
    u8 unk_x2;                                 ///< [6.0.0+] Unknown
    u8 unk_x3;                                 ///< [6.0.0+] Unknown
} IrsTeraPluginProcessorConfig;

/// PackedTeraPluginProcessorConfig
typedef struct {
    IrsPackedMcuVersion required_mcu_version;  ///< RequiredMcuVersion
    u8 mode;                                   ///< Mode
    u8 unk_x5;                                 ///< [6.0.0+] This is set to 0x2 | (IrsTeraPluginProcessorConfig::unk_x1 << 7).
    u8 unk_x6;                                 ///< [6.0.0+] IrsTeraPluginProcessorConfig::unk_x2
    u8 unk_x7;                                 ///< [6.0.0+] IrsTeraPluginProcessorConfig::unk_x3
} IrsPackedTeraPluginProcessorConfig;

/// IrLedProcessorConfig
typedef struct {
    u32 light_target;                          ///< Controls the IR leds. 0: All leds, 1: Bright group, 2: Dim group, 3: None.
} IrsIrLedProcessorConfig;

/// PackedIrLedProcessorConfig
typedef struct {
    IrsPackedMcuVersion required_mcu_version;  ///< RequiredMcuVersion
    u8 light_target;                           ///< Controls the IR leds. 0: All leds, 1: Bright group, 2: Dim group, 3: None.
    u8 pad[0x3];                               ///< Padding
} IrsPackedIrLedProcessorConfig;

/// AdaptiveClusteringProcessorConfig
typedef struct {
    u32 mode;                                  ///< \ref IrsAdaptiveClusteringMode
    u32 target_distance;                       ///< [6.0.0+] \ref IrsAdaptiveClusteringTargetDistance
} IrsAdaptiveClusteringProcessorConfig;

/// HandAnalysisConfig
typedef struct {
    u32 mode;                                  ///< \ref IrsHandAnalysisMode
} IrsHandAnalysisConfig;

/// MomentStatistic
typedef struct {
    float average_intensity;              ///< AverageIntensity
    float centroid_x;                     ///< CentroidX
    float centroid_y;                     ///< CentroidY
} IrsMomentStatistic;

/// MomentProcessorState
typedef struct {
    s64 sampling_number;                  ///< SamplingNumber
    u64 timestamp;                        ///< TimeStamp

    u32 ambient_noise_level;              ///< AmbientNoiseLevel
    u8 reserved[0x4];                     ///< Reserved
    IrsMomentStatistic statistic[0x30];   ///< \ref IrsMomentStatistic
} IrsMomentProcessorState;

/// ClusteringData
typedef struct {
    float average_intensity; ///< AverageIntensity
    float centroid_x;        ///< CentroidX
    float centroid_y;        ///< CentroidY
    u32 pixel_count;         ///< PixelCount
    u16 bound_x;             ///< BoundX
    u16 bound_y;             ///< BoundY
    u16 boundt_width;        ///< BoundtWidth
    u16 bound_height;        ///< BoundHeight
} IrsClusteringData;

/// ClusteringProcessorState
typedef struct {
    s64 sampling_number;                  ///< SamplingNumber
    u64 timestamp;                        ///< TimeStamp

    u8 object_count;                      ///< ObjectCount
    u8 reserved[0x3];                     ///< Reserved
    u32 ambient_noise_level;              ///< AmbientNoiseLevel
    IrsClusteringData data[0x10];         ///< \ref IrsClusteringData
} IrsClusteringProcessorState;

/// PointingProcessorMarkerState
typedef struct {
    s64 sampling_number;                  ///< SamplingNumber
    u64 timestamp;                        ///< TimeStamp

    struct {
        u8 pointing_status;               ///< PointingStatus
        u8 reserved[0x3];                 ///< Reserved
        u8 unk_x4[0x4];                   ///< Unknown
        float unk_x8;                     ///< Unknown
        float position_x;                 ///< PositionX
        float position_y;                 ///< PositionY
        float unk_x14;                    ///< Unknown
        IrsRect window_of_interest;       ///< WindowOfInterest
    } data[3];
} IrsPointingProcessorMarkerState;

/// PointingProcessorState
typedef struct {
    s64 sampling_number;                  ///< SamplingNumber
    u64 timestamp;                        ///< TimeStamp

    u32 pointing_status;                  ///< PointingStatus
    float position_x;                     ///< PositionX
    float position_y;                     ///< PositionY
    u8 reserved[0x4];                     ///< Reserved
} IrsPointingProcessorState;

/// TeraPluginProcessorState
typedef struct {
    s64 sampling_number;                  ///< SamplingNumber
    u64 timestamp;                        ///< TimeStamp
    u32 ambient_noise_level;              ///< AmbientNoiseLevel
    u8 plugin_data[0x12c];                ///< PluginData
} IrsTeraPluginProcessorState;

/// ProcessorState
typedef struct {
    s64 start;                                          ///< Start
    u32 count;                                          ///< Count
    u32 pad;                                            ///< Padding

    u8 data[0xe10];                                     ///< Contains an array of *ProcessorState, depending on IrsDeviceFormat::ir_sensor_mode.
} IrsProcessorState;

/// DeviceFormat
typedef struct {
    u32 ir_camera_status;                                   ///< \ref IrsIrCameraStatus
    u32 ir_camera_internal_status;                          ///< \ref IrsIrCameraInternalStatus
    u32 ir_sensor_mode;                                     ///< \ref IrsIrSensorMode
    u32 pad;                                                ///< Padding

    IrsProcessorState processor_state;                      ///< \ref IrsProcessorState
} IrsDeviceFormat;

/// AruidFormat
typedef struct {
    u64 ir_sensor_aruid;        ///< IrSensorAruid
    u32 ir_sensor_aruid_status; ///< IrSensorAruidStatus
    u32 pad;                    ///< Padding
} IrsAruidFormat;

/// StatusManager
typedef struct {
    IrsDeviceFormat device_format[IRS_MAX_CAMERAS];
    IrsAruidFormat aruid_format[0x5];
} IrsStatusManager;

/// Initialize irs.
Result irsInitialize(void);

/// Exit irs.
void irsExit(void);

/// Gets the Service object for the actual irs service session.
Service* irsGetServiceSession(void);

/// Gets the address of the SharedMemory (\ref IrsStatusManager).
void* irsGetSharedmemAddr(void);

/// Gets the \ref IrsIrCameraHandle for the specified controller.
Result irsGetIrCameraHandle(IrsIrCameraHandle *handle, HidNpadIdType id);

/// GetIrCameraStatus
Result irsGetIrCameraStatus(IrsIrCameraHandle handle, IrsIrCameraStatus *out);

/// CheckFirmwareUpdateNecessity
/// When successful where the output flag is set, the user should use \ref hidLaShowControllerFirmwareUpdate.
/// Only available on [4.0.0+].
Result irsCheckFirmwareUpdateNecessity(IrsIrCameraHandle handle, bool *out);

/// GetImageProcessorStatus
/// Only available on [4.0.0+].
Result irsGetImageProcessorStatus(IrsIrCameraHandle handle, IrsImageProcessorStatus *out);

/// Stop the current Processor.
/// \ref irsExit calls this with all IrCameraHandles which were not already used with \ref irsStopImageProcessor.
Result irsStopImageProcessor(IrsIrCameraHandle handle);

/// Stop the current Processor, async.
/// Only available on [4.0.0+].
Result irsStopImageProcessorAsync(IrsIrCameraHandle handle);

/**
 * @brief Run the MomentProcessor.
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[in] config Input config.
 */
Result irsRunMomentProcessor(IrsIrCameraHandle handle, const IrsMomentProcessorConfig *config);

/**
 * @brief Gets the states for MomentProcessor or IrLedProcessor.
 * @note The official GetIrLedProcessorState is essentially the same as this, except it uses hard-coded count=1 with output-array on stack, without returning that data. Hence we don't implement a seperate func for that.
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[out] states Output array of \ref IrsMomentProcessorState.
 * @param[in] count Size of the states array in entries. Must be 1-5.
 * @param[out] total_out Total output entries.
 */
Result irsGetMomentProcessorStates(IrsIrCameraHandle handle, IrsMomentProcessorState *states, s32 count, s32 *total_out);

/**
 * @brief Calculates an \ref IrsMomentStatistic from the specified region in the input \ref IrsMomentProcessorState.
 * @param[in] state \ref IrsMomentProcessorState
 * @param[in] rect \ref IrsRect, containing the image width and height.
 * @param[in] region_x Region x, must be 0-5 (clamped to this range otherwise). region_x = image_x/6.
 * @param[in] region_y Region y, must be 0-7 (clamped to this range otherwise). region_y = image_y/8.
 * @param[in] region_width Region width. region_x+region_width must be <=6 (clamped to this range otherwise).
 * @param[in] region_height Region height.  region_y+region_height must be <=8 (clamped to this range otherwise).
 */
IrsMomentStatistic irsCalculateMomentRegionStatistic(const IrsMomentProcessorState *state, IrsRect rect, s32 region_x, s32 region_y, s32 region_width, s32 region_height);

/**
 * @brief Run the ClusteringProcessor.
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[in] config Input config.
 */
Result irsRunClusteringProcessor(IrsIrCameraHandle handle, const IrsClusteringProcessorConfig *config);

/**
 * @brief Gets the states for ClusteringProcessor.
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[out] states Output array of \ref IrsClusteringProcessorState.
 * @param[in] count Size of the states array in entries. Must be 1-5.
 * @param[out] total_out Total output entries.
 */
Result irsGetClusteringProcessorStates(IrsIrCameraHandle handle, IrsClusteringProcessorState *states, s32 count, s32 *total_out);

/**
 * @brief Run the ImageTransferProcessor.
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[in] config Input config.
 * @param[in] size Work-buffer size, must be 0x1000-byte aligned.
 */
Result irsRunImageTransferProcessor(IrsIrCameraHandle handle, const IrsImageTransferProcessorConfig *config, size_t size);

/**
 * @brief Run the ImageTransferExProcessor.
 * @note Only available on [4.0.0+].
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[in] config Input config.
 * @param[in] size Work-buffer size, must be 0x1000-byte aligned.
 */
Result irsRunImageTransferExProcessor(IrsIrCameraHandle handle, const IrsImageTransferProcessorExConfig *config, size_t size);

/// GetImageTransferProcessorState
Result irsGetImageTransferProcessorState(IrsIrCameraHandle handle, void* buffer, size_t size, IrsImageTransferProcessorState *state);

/**
 * @brief Run the PointingProcessor.
 * @param[in] handle \ref IrsIrCameraHandle
 */
Result irsRunPointingProcessor(IrsIrCameraHandle handle);

/**
 * @brief Gets the states for PointingProcessor.
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[out] states Output array of \ref IrsPointingProcessorMarkerState.
 * @param[in] count Size of the states array in entries. Must be 1-6.
 * @param[out] total_out Total output entries.
 */
Result irsGetPointingProcessorMarkerStates(IrsIrCameraHandle handle, IrsPointingProcessorMarkerState *states, s32 count, s32 *total_out);

/**
 * @brief Gets the states for \ref IrsPointingProcessorState.
 * @note This uses \ref irsGetPointingProcessorMarkerStates, then converts the output to \ref IrsPointingProcessorState.
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[out] states Output array of \ref IrsPointingProcessorState.
 * @param[in] count Size of the states array in entries. Must be 1-6.
 * @param[out] total_out Total output entries.
 */
Result irsGetPointingProcessorStates(IrsIrCameraHandle handle, IrsPointingProcessorState *states, s32 count, s32 *total_out);

/**
 * @brief Run the TeraPluginProcessor.
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[in] config Input config.
 */
Result irsRunTeraPluginProcessor(IrsIrCameraHandle handle, const IrsTeraPluginProcessorConfig *config);

/**
 * @brief Gets the states for TeraPluginProcessor, filtered using the input params.
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[out] states Output array of \ref IrsTeraPluginProcessorState.
 * @param[in] count Size of the states array in entries. Must be 1-5.
 * @param[in] sampling_number Minimum value for IrsTeraPluginProcessorState::sampling_number.
 * @param[in] prefix_data Only used when prefix_bitcount is not 0. The first prefix_bitcount bits from prefix_data must match the first prefix_bitcount bits in IrsTeraPluginProcessorState::plugin_data.
 * @param[in] prefix_bitcount Total bits for prefix_data.
 * @param[out] total_out Total output entries.
 */
Result irsGetTeraPluginProcessorStates(IrsIrCameraHandle handle, IrsTeraPluginProcessorState *states, s32 count, s64 sampling_number, u32 prefix_data, u32 prefix_bitcount, s32 *total_out);

/**
 * @brief Run the IrLedProcessor.
 * @note Only available on [4.0.0+].
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[in] config Input config.
 */
Result irsRunIrLedProcessor(IrsIrCameraHandle handle, const IrsIrLedProcessorConfig *config);

/**
 * @brief Run the AdaptiveClusteringProcessor.
 * @note Only available on [5.0.0+].
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[in] config Input config.
 */
Result irsRunAdaptiveClusteringProcessor(IrsIrCameraHandle handle, const IrsAdaptiveClusteringProcessorConfig *config);

/**
 * @brief Run HandAnalysis.
 * @param[in] handle \ref IrsIrCameraHandle
 * @param[in] config Input config.
 */
Result irsRunHandAnalysis(IrsIrCameraHandle handle, const IrsHandAnalysisConfig *config);

/**
 * Gets the default configuration for MomentProcessor.
 */
void irsGetMomentProcessorDefaultConfig(IrsMomentProcessorConfig *config);

/**
 * Gets the default configuration for ClusteringProcessor.
 */
void irsGetClusteringProcessorDefaultConfig(IrsClusteringProcessorConfig *config);

/**
 * Gets the default configuration for ImageTransferProcessor.
 * Defaults are exposure 300us, 8x digital gain, the rest is all-zero. Format is ::IrsImageTransferProcessorFormat_320x240.
 */
void irsGetDefaultImageTransferProcessorConfig(IrsImageTransferProcessorConfig *config);

/**
 * Gets the default configuration for ImageTransferProcessorEx.
 * Defaults are exposure 300us, 8x digital gain, the rest is all-zero. OrigFormat/TrimmingFormat are ::IrsImageTransferProcessorFormat_320x240.
 */
void irsGetDefaultImageTransferProcessorExConfig(IrsImageTransferProcessorExConfig *config);

/**
 * Gets the default configuration for IrLedProcessor.
 */
NX_CONSTEXPR void irsGetIrLedProcessorDefaultConfig(IrsIrLedProcessorConfig *config) {
    config->light_target = 0;
}
