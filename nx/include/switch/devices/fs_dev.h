/**
 * @file fs_dev.h
 * @brief FS driver.
 */
#pragma once

//NOTE: This is currently not usable.

/// Initializes the FS driver.
Result fsdevInit(void);

/// Enable/disable copy in fsdev_write
void fsdevWriteSafe(bool enable);

/// Exits the FS driver.
Result fsdevExit(void);

