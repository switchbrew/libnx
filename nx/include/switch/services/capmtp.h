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

Result capmtpStartCommandHandler(void);
Result capmtpStopCommandHandler(void);
Event *capmtpGetEvent1(void);
Event *capmtpGetEvent2(void);
Result capmtpIsRunning(bool *out);
Result capmtpUnkBool(bool *out);
Result capmtpGetResult(void);
