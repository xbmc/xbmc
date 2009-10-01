#ifndef CPU_INFO_H
#define CPU_INFO_H

/*
 *  cpu_info.h
 *  Goom
 *
 *  Created by Guillaume Borios on Sun Dec 28 2003.
 *  Copyright (c) 2003 iOS. All rights reserved.
 *
 */

#ifdef HAVE_MMX
#ifndef CPU_X86
#define CPU_X86
#endif
#endif

/* Returns the CPU flavour described with the constants below */
unsigned int cpu_flavour (void);

#define CPU_OPTION_ALTIVEC  0x1
#define CPU_OPTION_64_BITS  0x2
#define CPU_OPTION_MMX      0x4
#define CPU_OPTION_XMMX     0x8
#define CPU_OPTION_SSE      0x10
#define CPU_OPTION_SSE2     0x20
#define CPU_OPTION_3DNOW    0x40


/* Returns the CPU number */
unsigned int cpu_number (void);

#endif
