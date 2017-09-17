#include <switch/types.h>
#include <switch/svc.h>

void __attribute__((weak)) __attribute__((noreturn)) __libnx_exit(int rc)
{
	svcExitProcess();
}
