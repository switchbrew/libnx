/**
 * @file uart.h
 * @brief UART service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../sf/service.h"

/// UartPort
typedef enum {
    UartPort_Bluetooth                      =     1,    ///< Bluetooth
    UartPort_JoyConR                        =     2,    ///< Joy-Con(R)
    UartPort_JoyConL                        =     3,    ///< Joy-Con(L)
    UartPort_MCU                            =     4,    ///< MCU
} UartPort;

/// UartPortForDev
typedef enum {
    UartPortForDev_JoyConR                  =     1,    ///< Joy-Con(R)
    UartPortForDev_JoyConL                  =     2,    ///< Joy-Con(L)
    UartPortForDev_Bluetooth                =     3,    ///< Bluetooth
} UartPortForDev;

/// FlowControlMode
typedef enum {
    UartFlowControlMode_None                =     0,    ///< None
    UartFlowControlMode_Hardware            =     1,    ///< Hardware
} UartFlowControlMode;

/// PortEventType
typedef enum {
    UartPortEventType_SendBufferEmpty       =     0,    ///< SendBufferEmpty
    UartPortEventType_SendBufferReady       =     1,    ///< SendBufferReady
    UartPortEventType_ReceiveBufferReady    =     2,    ///< ReceiveBufferReady
    UartPortEventType_ReceiveEnd            =     3,    ///< ReceiveEnd
} UartPortEventType;

/// PortSession
typedef struct {
    Service s;                                          ///< IPortSession
} UartPortSession;

/// Initialize uart.
Result uartInitialize(void);

/// Exit uart.
void uartExit(void);

/// Gets the Service object for the actual uart service session.
Service* uartGetServiceSession(void);

/**
 * @brief HasPort
 * @note Only available on [1.0.0-16.1.0].
 * @param[in] port \ref UartPort
 * @param[out] out Output success flag.
 */
Result uartHasPort(UartPort port, bool *out);

/**
 * @brief HasPortForDev
 * @note Only available on [1.0.0-16.1.0].
 * @param[in] port \ref UartPortForDev
 * @param[out] out Output success flag.
 */
Result uartHasPortForDev(UartPortForDev port, bool *out);

/**
 * @brief IsSupportedBaudRate
 * @note Only available on [1.0.0-16.1.0].
 * @param[in] port \ref UartPort
 * @param[in] baud_rate BaudRate
 * @param[out] out Output success flag.
 */
Result uartIsSupportedBaudRate(UartPort port, u32 baud_rate, bool *out);

/**
 * @brief IsSupportedBaudRateForDev
 * @note Only available on [1.0.0-16.1.0].
 * @param[in] port \ref UartPortForDev
 * @param[in] baud_rate BaudRate
 * @param[out] out Output success flag.
 */
Result uartIsSupportedBaudRateForDev(UartPortForDev port, u32 baud_rate, bool *out);

/**
 * @brief IsSupportedFlowControlMode
 * @note Only available on [1.0.0-16.1.0].
 * @param[in] port \ref UartPort
 * @param[in] flow_control_mode \ref UartFlowControlMode
 * @param[out] out Output success flag.
 */
Result uartIsSupportedFlowControlMode(UartPort port, UartFlowControlMode flow_control_mode, bool *out);

/**
 * @brief IsSupportedFlowControlModeForDev
 * @note Only available on [1.0.0-16.1.0].
 * @param[in] port \ref UartPortForDev
 * @param[in] flow_control_mode \ref UartFlowControlMode
 * @param[out] out Output success flag.
 */
Result uartIsSupportedFlowControlModeForDev(UartPortForDev port, UartFlowControlMode flow_control_mode, bool *out);

/**
 * @brief Creates an \ref UartPortSession.
 * @note Use \ref uartPortSessionOpenPort or \ref uartPortSessionOpenPortForDev before using any other cmds.
 * @param[out] s \ref UartPortSession
 */
Result uartCreatePortSession(UartPortSession *s);

/**
 * @brief IsSupportedPortEvent
 * @note Only available on [1.0.0-16.1.0].
 * @param[in] port \ref UartPort
 * @param[in] port_event_type \ref UartPortEventType
 * @param[out] out Output success flag.
 */
Result uartIsSupportedPortEvent(UartPort port, UartPortEventType port_event_type, bool *out);

/**
 * @brief IsSupportedPortEventForDev
 * @note Only available on [1.0.0-16.1.0].
 * @param[in] port \ref UartPortForDev
 * @param[in] port_event_type \ref UartPortEventType
 * @param[out] out Output success flag.
 */
Result uartIsSupportedPortEventForDev(UartPortForDev port, UartPortEventType port_event_type, bool *out);

/**
 * @brief IsSupportedDeviceVariation
 * @note Only available on [7.0.0-16.1.0].
 * @param[in] port \ref UartPort
 * @param[in] device_variation DeviceVariation
 * @param[out] out Output success flag.
 */
Result uartIsSupportedDeviceVariation(UartPort port, u32 device_variation, bool *out);

/**
 * @brief IsSupportedDeviceVariationForDev
 * @note Only available on [7.0.0-16.1.0].
 * @param[in] port \ref UartPortForDev
 * @param[in] device_variation DeviceVariation
 * @param[out] out Output success flag.
 */
Result uartIsSupportedDeviceVariationForDev(UartPortForDev port, u32 device_variation, bool *out);

///@name IPortSession
///@{

/**
 * @brief Close an \ref UartPortSession.
 * @param s \ref UartPortSession
 */
void uartPortSessionClose(UartPortSession* s);

/**
 * @brief OpenPort
 * @note This is not usable when the specified \ref UartPort is already being used.
 * @param s \ref UartPortSession
 * @param[out] out Output success flag.
 * @param[in] port \ref UartPort
 * @param[in] baud_rate BaudRate
 * @param[in] flow_control_mode \ref UartFlowControlMode
 * @param[in] device_variation [7.0.0+] DeviceVariation
 * @param[in] is_invert_tx [6.0.0+] IsInvertTx
 * @param[in] is_invert_rx [6.0.0+] IsInvertRx
 * @param[in] is_invert_rts [6.0.0+] IsInvertRts
 * @param[in] is_invert_cts [6.0.0+] IsInvertCts
 * @param[in] send_buffer Send buffer, must be 0x1000-byte aligned.
 * @param[in] send_buffer_length Send buffer size, must be 0x1000-byte aligned.
 * @param[in] receive_buffer Receive buffer, must be 0x1000-byte aligned.
 * @param[in] receive_buffer_length Receive buffer size, must be 0x1000-byte aligned.
 */
Result uartPortSessionOpenPort(UartPortSession* s, bool *out, UartPort port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, void* send_buffer, u64 send_buffer_length, void* receive_buffer, u64 receive_buffer_length);

/**
 * @brief OpenPortForDev
 * @note See the notes for \ref uartPortSessionOpenPort.
 * @param s \ref UartPortSession
 * @param[out] out Output success flag.
 * @param[in] port \ref UartPortForDev
 * @param[in] baud_rate BaudRate
 * @param[in] flow_control_mode \ref UartFlowControlMode
 * @param[in] device_variation [7.0.0+] DeviceVariation
 * @param[in] is_invert_tx [6.0.0+] IsInvertTx
 * @param[in] is_invert_rx [6.0.0+] IsInvertRx
 * @param[in] is_invert_rts [6.0.0+] IsInvertRts
 * @param[in] is_invert_cts [6.0.0+] IsInvertCts
 * @param[in] send_buffer Send buffer, must be 0x1000-byte aligned.
 * @param[in] send_buffer_length Send buffer size, must be 0x1000-byte aligned.
 * @param[in] receive_buffer Receive buffer, must be 0x1000-byte aligned.
 * @param[in] receive_buffer_length Receive buffer size, must be 0x1000-byte aligned.
 */
Result uartPortSessionOpenPortForDev(UartPortSession* s, bool *out, UartPortForDev port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, void* send_buffer, u64 send_buffer_length, void* receive_buffer, u64 receive_buffer_length);

/**
 * @brief GetWritableLength
 * @param s \ref UartPortSession
 * @param[out] out Output WritableLength.
 */
Result uartPortSessionGetWritableLength(UartPortSession* s, u64 *out);

/**
 * @brief Send
 * @param s \ref UartPortSession
 * @param[in] in_data Input data buffer.
 * @param[in] size Input data buffer size.
 * @param[out] out Output size.
 */
Result uartPortSessionSend(UartPortSession* s, const void* in_data, size_t size, u64 *out);

/**
 * @brief GetReadableLength
 * @param s \ref UartPortSession
 * @param[out] out Output ReadableLength.
 */
Result uartPortSessionGetReadableLength(UartPortSession* s, u64 *out);

/**
 * @brief Receive
 * @param s \ref UartPortSession
 * @param[out] out_data Output data buffer.
 * @param[in] size Output data buffer size.
 * @param[out] out Output size.
 */
Result uartPortSessionReceive(UartPortSession* s, void* out_data, size_t size, u64 *out);

/**
 * @brief BindPortEvent
 * @note The Event must be closed by the user after using \ref uartPortSessionUnbindPortEvent.
 * @param s \ref UartPortSession
 * @param[in] port_event_type \ref UartPortEventType
 * @param[in] threshold Threshold
 * @param[out] out Output success flag.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result uartPortSessionBindPortEvent(UartPortSession* s, UartPortEventType port_event_type, s64 threshold, bool *out, Event *out_event);

/**
 * @brief UnbindPortEvent
 * @param s \ref UartPortSession
 * @param[in] port_event_type \ref UartPortEventType
 * @param[out] out Output success flag.
 */
Result uartPortSessionUnbindPortEvent(UartPortSession* s, UartPortEventType port_event_type, bool *out);

///@}

