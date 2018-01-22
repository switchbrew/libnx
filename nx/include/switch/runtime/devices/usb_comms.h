#pragma once
#include <switch/types.h>

Result usbCommsInitialize(void);
void usbCommsExit(void);

size_t usbCommsRead(void* buffer, size_t size);
size_t usbCommsWrite(const void* buffer, size_t size);

