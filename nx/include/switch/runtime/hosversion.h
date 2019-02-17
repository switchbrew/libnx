/**
 * @file hosversion.h
 * @brief Horizon OS (HOS) version detection utilities.
 * @author fincs
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Builds a HOS version value from its constituent components.
#define MAKEHOSVERSION(_major,_minor,_micro) (((u32)(_major) << 16) | ((u32)(_minor) << 8) | (u32)(_micro))

/// Extracts the major number from a HOS version value.
#define HOSVER_MAJOR(_version) (((_version) >> 16) & 0xFF)

/// Extracts the minor number from a HOS version value.
#define HOSVER_MINOR(_version) (((_version) >>  8) & 0xFF)

/// Extracts the micro number from a HOS version value.
#define HOSVER_MICRO(_version) ( (_version)        & 0xFF)

/// Returns the current HOS version that was previously set with \ref hosversionSet. If version initialization fails during startup (such as in the case set:sys is not available), this function returns zero.
u32 hosversionGet(void);

/// Sets or overrides the current HOS version. This function is normally called automatically by libnx on startup with the version info obtained with \ref setsysGetFirmwareVersion.
void hosversionSet(u32 version);

/// Returns true if the current HOS version is equal to or above the specified major/minor/micro version.
static inline bool hosversionAtLeast(u8 major, u8 minor, u8 micro) {
    return hosversionGet() >= MAKEHOSVERSION(major,minor,micro);
}

/// Returns true if the current HOS version is earlier than the specified major/minor/micro version.
static inline bool hosversionBefore(u8 major, u8 minor, u8 micro) {
    return !hosversionAtLeast(major, minor, micro);
}

/// Returns true if the current HOS version is between the two specified major versions, i.e. [major1, major2).
static inline bool hosversionBetween(u8 major1, u8 major2) {
    u32 ver = hosversionGet();
    return ver >= MAKEHOSVERSION(major1,0,0) && ver < MAKEHOSVERSION(major2,0,0);
}
