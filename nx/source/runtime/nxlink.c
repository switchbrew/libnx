#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>


// System globals we define here
extern int __system_argc;
extern char** __system_argv;

struct in_addr __nxlink_host;


void nxlinkSetup(void)
{
    if ( __system_argc > 1 &&
         strlen(__system_argv[__system_argc - 1]) == 16 &&
         strncmp(&__system_argv[__system_argc - 1][8], "_NXLINK_", 8) == 0 )
    {
        __system_argc--;
        __nxlink_host.s_addr = strtoul(__system_argv[__system_argc], NULL, 16);
    }
    __system_argv[__system_argc] = NULL;
}
