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

static inline CompareResult cmpresNot(CompareResult in) {
    // Does a boolean NOT operation.
    switch (in) {
    case CompareResult_False:
	return CompareResult_True;
    case CompareResult_True:
	return CompareResult_False;
    default:
	return CompareResult_Unknown;
    }
}

static inline CompareResult cmpresAnd(CompareResult a, CompareResult b) {
    switch (a) {
    case CompareResult_False:
	return CompareResult_False;
    case CompareResult_True:
	return b;
    case CompareResult_Unknown:
	if (b != CompareResult_True)
	    return b;
	return CompareResult_Unknown;
    }
}

/// Returns true if the current HOS version is equal to or above the specified major/minor/micro version.
CompareResult hosversionAtLeast(u8 major, u8 minor, u8 micro);

/// Returns true if the current HOS version is earlier than the specified major/minor/micro version.
static inline CompareResult hosversionBefore(u8 major, u8 minor, u8 micro) {
    return cmpresNot(hosversionAtLeast(major, minor, micro));
}

/// Returns true if the current HOS version is between the two specified major versions, i.e. [major1, major2).
static inline CompareResult hosversionBetween(u8 major0, u8 minor0, u8 micro0, u8 major1, u8 minor1, u8 micro1) {
    return cmpresAnd(hosversionAtLeast(major0, minor0, micro0), hosversionBefore(major1, minor1, micro1));
}
