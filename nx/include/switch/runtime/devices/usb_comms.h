/**
 * @file usb_comms.h
 * @brief USB comms.
 * @author yellows8
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../../types.h"

Result usbCommsInitialize(void);
void usbCommsExit(void);

size_t usbCommsRead(void* buffer, size_t size);
size_t usbCommsWrite(const void* buffer, size_t size);
