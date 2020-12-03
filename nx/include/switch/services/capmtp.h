/**
 * @file capmtp.h
 * @brief capmtp service IPC wrapper.
 * @note Only available on [11.0.0+].
 * @author Behemoth
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../sf/service.h"

Result capmtpInitialize(void* mem, size_t size, u32 app_count, u32 max_img, u32 max_vid, const char *other_name);
void capmtpExit(void);

Service* capmtpGetRootServiceSession(void);
Service* capmtpGetServiceSession(void);

Result capmtpStartCommandHandler(void);
Result capmtpStopCommandHandler(void);
bool capmtpIsRunning(void);
Event *capmtpGetConnectionEvent(void);
bool capmtpIsConnected(void);
Event *capmtpGetScanErrorEvent(void);
Result capmtpGetScanError(void);
