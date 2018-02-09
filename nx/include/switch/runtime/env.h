/**
 * @file env.h
 * @brief Homebrew environment definitions and utilities.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Structure representing an entry in the homebrew environment configuration.
typedef struct {
    u32 Key;      ///< Type of entry
    u32 Flags;    ///< Entry flags
    u64 Value[2]; ///< Entry arguments (type-specific)
} ConfigEntry;

/// Entry flags
enum {
    EntryFlag_IsMandatory = BIT(0), ///< Specifies that the entry **must** be processed by the homebrew application.
};

///< Types of entry
enum {
    EntryType_EndOfList=0,            ///< Entry list terminator.
    EntryType_MainThreadHandle=1,     ///< Provides the handle to the main thread.
    EntryType_NextLoadPath=2,         ///< Provides a buffer containing information about the next homebrew application to load.
    EntryType_OverrideHeap=3,         ///< Provides heap override information.
    EntryType_OverrideService=4,      ///< Provides service override information.
    EntryType_Argv=5,                 ///< Provides argv.
    EntryType_SyscallAvailableHint=6, ///< Provides syscall availability hints.
    EntryType_AppletType=7,           ///< Provides APT applet type.
    EntryType_AppletWorkaround=8,     ///< Indicates that APT is broken and should not be used.
    EntryType_StdioSockets=9,         ///< Provides socket-based standard stream redirection information.
    EntryType_ProcessHandle=10,       ///< Provides the process handle.
    EntryType_LastLoadResult=11       ///< Provides the last load result.
};

/// Loader return function.
typedef void NORETURN (*LoaderReturnFn)(int result_code);

/**
 * @brief Sets up the homebrew environment (internally called).
 * @param ctx Reserved.
 * @param main_thread Reserved.
 * @param saved_lr Reserved.
 */
void envSetup(void* ctx, Handle main_thread, LoaderReturnFn saved_lr);

/// Retrieves the handle to the main thread.
Handle envGetMainThreadHandle(void);
/// Returns true if the application is running as NSO, otherwise NRO.
bool envIsNso(void);

/// Returns true if the environment has a heap override.
bool  envHasHeapOverride(void);
/// Returns the address of the overriden heap.
void* envGetHeapOverrideAddr(void);
/// Returns the size of the overriden heap.
u64   envGetHeapOverrideSize(void);

/// Returns true if the environment has an argv array.
bool  envHasArgv(void);
/// Returns the number of arguments in the argv array.
u64   envGetArgc(void);
/// Returns the pointer to the argv array.
void* envGetArgv(void);

/**
 * @brief Returns whether a syscall is hinted to be available.
 * @param svc Syscall number to test.
 * @returns true if the syscall is available.
 */
bool envIsSyscallHinted(u8 svc);

/// Returns the handle to the running homebrew process.
Handle envGetOwnProcessHandle(void);

/// Returns the loader's return function, to be called on program exit.
LoaderReturnFn envGetExitFuncPtr(void);

/**
 * @brief Configures the next homebrew application to load.
 * @param path Path to the next homebrew application to load (.nro).
 * @param argv Argument string to pass.
 */
Result envSetNextLoad(const char* path, const char* argv);

/// Returns true if the environment supports envSetNextLoad.
bool envHasNextLoad(void);
