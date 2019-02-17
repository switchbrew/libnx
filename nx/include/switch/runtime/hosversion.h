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

void hosversionSetup(void);
void hosversionSet(u32 version);

typedef enum {
    CompareResult_False,
    CompareResult_True,
    CompareResult_Unknown
} CompareResult;

CompareResult cmpresNot(CompareResult in) {
    // Does a not-operation. True maps to False, False maps to True. Unknown maps to Unknown.
    switch (in) {
    case CompareResult_False:
	return CompareResult_True;
    case CompareResult_True:
	return CompareResult_False;
    default:
	return CompareResult_Unknown;
    }
}

/// Returns true if the current HOS version is equal to or above the specified major/minor/micro version.
CompareResult hosversionAtLeast(u8 major, u8 minor, u8 micro);

/// Returns true if the current HOS version is earlier than the specified major/minor/micro version.
static inline CompareResult hosversionBefore(u8 major, u8 minor, u8 micro) {
    return cmpresNot(hosversionAtLeast(major, minor, micro));
}
