/**
 * @file swkbd.h
 * @brief Wrapper for using the swkbd LibraryApplet.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/applet.h"

typedef struct {
    u8 data[0x3E0];//TODO: Fill this in.
} SwkbdArg;

typedef struct {
    SwkbdArg arg;
} SwkbdConfig;

/**
 * @brief Creates a SwkbdConfig struct.
 * @param c SwkbdConfig struct.
 */
void swkbdCreate(SwkbdConfig* c);

/**
 * @brief Launch swkbd with the specified config. This will return once swkbd is finished running.
 * @param c SwkbdConfig struct.
 */
Result swkbdShow(SwkbdConfig* c);

