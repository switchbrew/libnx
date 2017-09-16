#include <sys/iosupport.h>
#include <sys/time.h>
#include <string.h>

#include <switch/types.h>
#include <switch/svc.h>

void __system_initSyscalls(void);

void heapSetup();

void __attribute__((weak)) __libnx_init(void)
{
	// Initialize newlib support system calls
	__system_initSyscalls();

	heapSetup();
}
