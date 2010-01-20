/*
 *  cpu_info.c
 *  Goom
 *
 *  Created by Guillaume Borios on Sun Dec 28 2003.
 *  Copyright (c) 2003 iOS. All rights reserved.
 *
 */

#include "cpu_info.h"

#ifdef CPU_X86
#include "mmx.h"
#endif

#ifdef CPU_POWERPC
#include <sys/types.h>
#include <stdlib.h>
#endif

static unsigned int CPU_FLAVOUR = 0;
static unsigned int CPU_NUMBER = 1;
static unsigned int CPU_DETECTED = 0;

static void autoset_cpu_info (void)
{
    CPU_DETECTED = 1;
    
#ifdef CPU_POWERPC
    int result;
    size_t size;
    
    result = 0;
    size = 4;
    if (sysctlbyname("hw.optional.altivec",&result,&size,NULL,NULL) == 0)
    {
        if (result != 0) CPU_FLAVOUR |= CPU_OPTION_ALTIVEC;
    }
    
    result = 0;
    size = 4;
    if (sysctlbyname("hw.optional.64bitops",&result,&size,NULL,NULL) == 0)
    {
        if (result != 0) CPU_FLAVOUR |= CPU_OPTION_64_BITS;
    }
    
    result = 0;
    size = 4;
    if (sysctlbyname("hw.ncpu",&result,&size,NULL,NULL) == 0)
    {
        if (result != 0) CPU_NUMBER = result;
    }
#endif /* CPU_POWERPC */
    
#ifdef CPU_X86
    if (mmx_supported()) CPU_FLAVOUR |= CPU_OPTION_MMX;
    if (xmmx_supported()) CPU_FLAVOUR |= CPU_OPTION_XMMX;
#endif /* CPU_X86 */
}

unsigned int cpu_flavour (void)
{
    if (CPU_DETECTED == 0) autoset_cpu_info();
    return CPU_FLAVOUR;
}

unsigned int cpu_number (void)
{
    if (CPU_DETECTED == 0) autoset_cpu_info();
    return CPU_NUMBER;
}
