#include <switch/types.h>
#include <switch/services/sm.h>

void __attribute__((weak)) __appInit(void) {
	// Initialize services
	smInitialize();
}
