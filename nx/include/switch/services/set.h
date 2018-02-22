// Copyright 2018 plutoo
#include "result.h"

typedef enum {
    ColorSetId_Light=0,
    ColorSetId_Dark=1
} ColorSetId;

Result setsysInitialize(void);
void setsysExit(void);

/// Gets the current system theme.
Result setsysGetColorSetId(ColorSetId* out);
