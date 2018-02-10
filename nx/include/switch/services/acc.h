/**
 * @file acc.h
 * @brief Account (acc:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "sm.h"

Result accountInitialize(void);
void accountExit(void);
Service* accountGetService(void);

/// Get the userID for the currently active user. The output userID is only valid when the output account_selected==1, otherwise no user is currently selected.
Result accountGetActiveUser(u128 *userID, bool *account_selected);
