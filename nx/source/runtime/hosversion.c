#include "runtime/hosversion.h"
#include "kernel/detect.h"
#include "services/fatal.h"

static u32 g_kernelLowerBound;
static u32 g_kernelUpperBound;

static u32 g_hosVersion;
static bool g_hasHosVersion;

void hosversionSetup(void)
{
    g_kernelLowerBound = detectKernelVersion();
    g_kernelUpperBound = detectKernelVersionUpperBound();
}

void hosversionSet(u32 version)
{
    g_hosVersion = version;
    g_hasHosVersion = true;
}

bool hosversionAtLeast(u8 major, u8 minor, u8 micro)
{
    u32 ver = MAKEHOSVERSION(major, minor, micro);

    if (g_hasHosVersion)
	return g_hosVersion >= ver;

    if (g_kernelLowerBound >= ver)
	return true;

    fatalSimple(-1);
}

bool hosversionBefore(u8 major, u8 minor, u8 micro)
{
    u32 ver = MAKEHOSVERSION(major, minor, micro);

    if (g_hasHosVersion)
	return g_hosVersion < ver;

    if (g_kernelUpperBound < ver)
	return true;

    fatalSimple(-1);
}
