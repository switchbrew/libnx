#pragma once
#include <switch/types.h>

Result accountInitialize(void);
void accountExit(void);
Handle accountGetSessionService(void);

/// Get the userID for the currently active user. The output userID is only valid when the output account_selected==1, otherwise no user is currently selected.
Result accountGetActiveUser(u128 *userID, bool *account_selected);
