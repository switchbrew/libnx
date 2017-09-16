#include <sys/iosupport.h>
#include <sys/time.h>
#include <sys/lock.h>
#include <sys/reent.h>
#include <string.h>

#include <switch/types.h>
#include <switch/svc.h>

void __nx_exit(int rc);

//TODO
/*extern const u8 __tdata_lma[];
extern const u8 __tdata_lma_end[];
extern u8 __tls_start[];

static struct _reent* __nx_get_reent()
{
	ThreadVars* tv = getThreadVars();
	if (tv->magic != THREADVARS_MAGIC)
	{
		svcBreak(USERBREAK_PANIC);
		for (;;);
	}
	return tv->reent;
}
*/

void __system_initSyscalls(void)
{
	// Register newlib syscalls
	__syscalls.exit     = __nx_exit;
	//__syscalls.getreent = __nx_get_reent;

	// Initialize thread vars for the main thread (TODO)
	/*ThreadVars* tv = getThreadVars();
	tv->magic = THREADVARS_MAGIC;
	tv->reent = _impure_ptr;
	tv->thread_ptr = NULL;
	tv->tls_tp = __tls_start-8; // ARM ELF TLS ABI mandates an 8-byte header

	u32 tls_size = __tdata_lma_end - __tdata_lma;
	if (tls_size)
		memcpy(__tls_start, __tdata_lma, tls_size);*/
}

