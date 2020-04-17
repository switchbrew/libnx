#include <stdio.h>
#include <string.h>
#include <sys/iosupport.h>
#include "runtime/devices/console.h"
#include "kernel/svc.h"

//---------------------------------------------------------------------------------
static ssize_t debug_write(struct _reent *r, void *fd, const char *ptr, size_t len) {
//---------------------------------------------------------------------------------
	svcOutputDebugString(ptr,len);
	return len;
}

static const devoptab_t dotab_svc = {
	.name    = "svc",
	.write_r = debug_write,
};

static const devoptab_t dotab_null = {
	.name = "null",
};

__attribute__((weak)) const devoptab_t* __nx_get_console_dotab(void) {
	return &dotab_null;
}

//---------------------------------------------------------------------------------
void consoleDebugInit(debugDevice device) {
//---------------------------------------------------------------------------------

	int buffertype = _IONBF;

	switch(device) {

	case debugDevice_SVC:
		devoptab_list[STD_ERR] = &dotab_svc;
		buffertype = _IOLBF;
		break;
	case debugDevice_CONSOLE:
		devoptab_list[STD_ERR] = __nx_get_console_dotab();
		break;
	case debugDevice_NULL:
		devoptab_list[STD_ERR] = &dotab_null;
		break;
	}
	setvbuf(stderr, NULL, buffertype, 0);

}
