#include "runtime/hosversion.h"
#include "kernel/detect.h"

static u32 g_hosVersionKernelLowerBound;
static bool g_hasKernelLowerBound;

static u32 g_hosVersionExact;
static bool g_hasExact;


void hosversionSetup(void) {
    g_hosVersionKernelLowerBound = MAKEHOSVERSION(detectKernelVersion(), 0, 0);
    g_hasKernelLowerBound = true;
}

void hosversionSet(u32 version) {
    g_hosVersionExact = version;
    g_hasExact = true;
}

CompareResult hosversionAtLeast(u8 major, u8 minor, u8 micro)
{
    if (g_hasExact) {
	return (MAKEHOSVERSION(major, minor, micro) <= g_hosVersionExact) ? CompareResult_True : CompareResult_False;
    }

    if (g_hasKernelLowerBound) {
	if (MAKEHOSVERSION(major, minor, micro) <= g_hosVersionKernelLowerBound)
	    return CompareResult_True;
    }

    return CompareResult_Unknown;
}
