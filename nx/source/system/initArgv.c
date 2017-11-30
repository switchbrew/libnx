#include <switch.h>

#include <string.h>

// System globals we define here
int __system_argc;
char** __system_argv;

extern char* fake_heap_start;
extern char* fake_heap_end;

void __system_initArgv(void)
{
    //TODO: How to load args?
    __system_argc = 0;
    __system_argv = NULL;

    /*
    int i;
    const char* arglist = envGetSystemArgList();
    const char* temp = arglist;

    // Check if the argument list is present
    if (!temp)
        return;

    // Retrieve argc
    __system_argc = *(u32*)temp;
    temp += sizeof(u32);

    // Find the end of the argument data
    for (i = 0; i < __system_argc; i ++)
    {
        for (; *temp; temp ++);
        temp ++;
    }

    // Reserve heap memory for argv data
    u32 argSize = temp - arglist - sizeof(u32);
    __system_argv = (char**)fake_heap_start;
    fake_heap_start += sizeof(char**)*(__system_argc + 1);
    char* argCopy = fake_heap_start;
    fake_heap_start += argSize;

    // Fill argv array
    memcpy(argCopy, &arglist[4], argSize);
    temp = argCopy;
    for (i = 0; i < __system_argc; i ++)
    {
        __system_argv[i] = (char*)temp;
        for (; *temp; temp ++);
        temp ++;
    }
    __system_argv[__system_argc] = NULL;
    */
}

