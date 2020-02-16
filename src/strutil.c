#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "../include/strutil.h"

/* ------------------------------------------------------------------------- */

uint32_t strip(char * Copy_pcStr, uint32_t Copy_u32StrLen)
{
    while(!IS_ALNUM(Copy_pcStr[Copy_u32StrLen - 1]))
    {
        Copy_pcStr[Copy_u32StrLen - 1] = '\0';
        Copy_u32StrLen--;
    }
    return Copy_u32StrLen;
}

/* ------------------------------------------------------------------------- */