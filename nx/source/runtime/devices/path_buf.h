#pragma once
#include <limits.h>

#include "services/fs.h"

typedef union {
    char   unix_path[PATH_MAX+1];
    FsPath nx_path;
} PathBuf;

extern __thread PathBuf __nx_dev_path_buf;
