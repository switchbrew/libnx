#include <string.h>
#include <ctype.h>
#include "result.h"
#include "runtime/env.h"
#include "kernel/svc.h"

// System globals we define here
int __system_argc;
char** __system_argv;

extern char* fake_heap_start;
extern char* fake_heap_end;

extern u32 __argdata__;

void argvSetup(void)
{
    Result rc=0;
    MemoryInfo meminfo;
    u32 pageinfo=0;

    u8 *argdata = (u8*)&__argdata__;
    u32 *arg32 = (u32*)argdata;
    u64 argdata_allocsize;
    u64 argdata_strsize;
    u32 argvptr_pos;
    u32 max_argv;
    u32 argi;
    u32 arglen=0;
    bool quote_flag=0;
    bool end_flag=0;
    char *args;
    char *argstart;
    char *argstorage;

    __system_argc = 0;
    __system_argv = NULL;

    // TODO: Use envHasArgv()/envGetArgv() here for the NRO case.

    if (envIsNso()) {
        memset(&meminfo, 0, sizeof(meminfo));
        rc = svcQueryMemory(&meminfo, &pageinfo, (u64)argdata);

        // This memory is only mapped when arguments were passed.
        if (R_FAILED(rc) || meminfo.perm != 0x3)
            return;

        argdata_allocsize = (u64)arg32[0];
        argdata_strsize = (u64)arg32[1];
        args = (char*)&argdata[0x20];

        if (argdata_allocsize==0 || argdata_strsize==0) return;

        if ((u64)argdata < meminfo.addr) return;
        if (((u64)argdata - meminfo.addr) + argdata_allocsize > meminfo.size) return;

        argvptr_pos = 0x20 + argdata_strsize+1;
        if (argvptr_pos >= argdata_allocsize) return;
        argstorage = (char*)&argdata[argvptr_pos];

        argvptr_pos += (argdata_strsize+1 + 0x9) & ~0x7;
        if (argvptr_pos >= argdata_allocsize) return;

        max_argv = (argdata_allocsize - argvptr_pos) >> 3;
        if (max_argv < 2) return;

        __system_argv = (char**)&argdata[argvptr_pos];

        argstart = NULL;

        for(argi=0; argi<argdata_strsize; argi++) {
            if (argstart == NULL && isspace(args[argi])) continue;

            if (argstart == NULL) {
                if (args[argi] == '"') {
                    argstart = &args[argi+1];
                    quote_flag = 1;
                }
                else if(args[argi]!=0) {
                    argstart = &args[argi];
                    arglen++;
                }
            }
            else {
                end_flag = 0;

                if (quote_flag && args[argi] == '"') {
                    end_flag = 1;
                }
                else if (isspace(args[argi])) {
                    end_flag = 1;
                }
                else if(args[argi]!=0) {
                    arglen++;
                }

                if ((args[argi]==0 || end_flag) && arglen) {
                    strncpy(argstorage, argstart, arglen);
                    argstorage[arglen] = 0;
                    __system_argv[__system_argc] = argstorage;
                    __system_argc++;
                    argstart = NULL;
                    quote_flag = 0;
                    argstorage+= arglen+1;
                    arglen = 0;

                    if (__system_argc >= max_argv) break;
                }
            }
        }

        if (arglen && __system_argc < max_argv) {
            strncpy(argstorage, argstart, arglen);
            argstorage[arglen] = 0;
            __system_argv[__system_argc] = argstorage;
            __system_argc++;
        }

        __system_argv[__system_argc] = NULL;
    }
}

