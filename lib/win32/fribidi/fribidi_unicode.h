/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 2001,2002,2005 Behdad Esfahbod.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public  
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,  
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License  
 * along with this library, in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA
 * 
 * For licensing issues, contact <fwpg@sharif.edu>.
 */

#ifndef FRIBIDI_UNICODE_H
#define FRIBIDI_UNICODE_H

#include "fribidi_config.h"
#include "fribidi_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Unicode version */
#define FRIBIDI_UNICODE_CHARS	(sizeof(FriBidiChar) >= 4 ? 0x110000 : 0x10000)
#define FRIBIDI_UNICODE_VERSION	"5.0.0"

/* UAX#9 Unicode BiDirectional Algorithm */
#define UNI_MAX_BIDI_LEVEL 61

/* BiDirectional marks */
#define UNI_LRM		0x200E
#define UNI_RLM		0x200F
#define UNI_LRE		0x202A
#define UNI_RLE		0x202B
#define UNI_PDF		0x202C
#define UNI_LRO		0x202D
#define UNI_RLO		0x202E

/* Line and Paragraph separators */
#define UNI_LS		0x2028
#define UNI_PS		0x2029

/* Joining marks */
#define UNI_ZWNJ	0x200C
#define UNI_ZWJ		0x200D

/* Hebrew and Arabic */
#define UNI_HEBREW_ALEF	0x05D0
#define UNI_ARABIC_ALEF	0x0627
#define UNI_ARABIC_ZERO	0x0660
#define UNI_FARSI_ZERO	0x06F0

/* wcwidth functions */
  FRIBIDI_API int fribidi_wcwidth (FriBidiChar ch);
  FRIBIDI_API int fribidi_wcswidth (const FriBidiChar *str,
				    FriBidiStrIndex len);
  FRIBIDI_API int fribidi_wcswidth_cjk (const FriBidiChar *str,
					FriBidiStrIndex len);

#ifdef	__cplusplus
}
#endif

#endif				/* FRIBIDI_UNICODE_H */
