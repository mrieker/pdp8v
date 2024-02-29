#ifndef _BINLOADER_H
#define _BINLOADER_H

#include <stdio.h>

#include "miscdefs.h"

int binloader (char const *loadname, FILE *loadfile, uint16_t *start_r);

#endif
