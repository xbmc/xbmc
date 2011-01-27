/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 1999,2000 Dov Grobgeld, and
 * Copyright (C) 2001,2002 Behdad Esfahbod. 
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
 * For licensing issues, contact <dov@imagic.weizmann.ac.il> and 
 * <fwpg@sharif.edu>. 
 */
#ifndef FRIBIDI_TYPES_H
#define FRIBIDI_TYPES_H

#include "fribidi_config.h"

#define FRIBIDI_INT8	char
#if FRIBIDI_SIZEOF_INT+0 == 2
# define FRIBIDI_INT16	int
#elif FRIBIDI_SIZEOF_SHORT+0 == 2
# define FRIBIDI_INT16	short
#else
# error cannot determine a 16-bit integer type.  check fribidi_config.h
#endif
#if FRIBIDI_SIZEOF_INT+0 == 4
# define FRIBIDI_INT32	int
#elif FRIBIDI_SIZEOF_LONG+0 == 4
# define FRIBIDI_INT32	long
#else
# error cannot determine a 32-bit integer type.  check fribidi_config.h
#endif


#ifdef __cplusplus
extern "C"
{
#endif

  typedef int fribidi_boolean;

  typedef signed FRIBIDI_INT8 fribidi_int8;
  typedef unsigned FRIBIDI_INT8 fribidi_uint8;
  typedef signed FRIBIDI_INT16 fribidi_int16;
  typedef unsigned FRIBIDI_INT16 fribidi_uint16;
  typedef signed FRIBIDI_INT32 fribidi_int32;
  typedef unsigned FRIBIDI_INT32 fribidi_uint32;

  typedef signed int fribidi_int;
  typedef unsigned int fribidi_uint;


  typedef fribidi_int8 FriBidiLevel;
  typedef fribidi_uint32 FriBidiChar;
  typedef fribidi_int FriBidiStrIndex;
  typedef fribidi_int32 FriBidiMaskType;
  typedef FriBidiMaskType FriBidiCharType;

  char *fribidi_type_name (FriBidiCharType c);

/* The following type is used by fribidi_utils */
  typedef struct
  {
    FriBidiStrIndex length;
    void *attribute;
  }
  FriBidiRunType;

/* The following type is used by fribdi_utils */
  typedef struct _FriBidiList FriBidiList;
  struct _FriBidiList
  {
    void *data;
    FriBidiList *next;
    FriBidiList *prev;
  };

#ifndef FRIBIDI_MAX_STRING_LENGTH
#define FRIBIDI_MAX_STRING_LENGTH (FriBidiStrIndex) \
				  (sizeof (FriBidiStrIndex) == 2 ?	\
    				   0x7FFE : (sizeof (FriBidiStrIndex) == 1 ? \
					     0x7E : 0x7FFFFFFEL))
#endif


/* 
 * Define some bit masks, that character types are based on, each one has
 * only one bit on.
 */

/* Do not use enum, because 16bit processors do not allow 32bit enum values. */

#define FRIBIDI_MASK_RTL	0x00000001L	/* Is right to left */
#define FRIBIDI_MASK_ARABIC	0x00000002L	/* Is arabic */

/* Each char can be only one of the three following. */
#define FRIBIDI_MASK_STRONG	0x00000010L	/* Is strong */
#define FRIBIDI_MASK_WEAK	0x00000020L	/* Is weak */
#define FRIBIDI_MASK_NEUTRAL	0x00000040L	/* Is neutral */
#define FRIBIDI_MASK_SENTINEL	0x00000080L	/* Is sentinel: SOT, EOT */
/* Sentinels are not valid chars, just identify the start and end of strings. */

/* Each char can be only one of the five following. */
#define FRIBIDI_MASK_LETTER	0x00000100L	/* Is letter: L, R, AL */
#define FRIBIDI_MASK_NUMBER	0x00000200L	/* Is number: EN, AN */
#define FRIBIDI_MASK_NUMSEPTER	0x00000400L	/* Is number separator or terminator: ES, ET, CS */
#define FRIBIDI_MASK_SPACE	0x00000800L	/* Is space: BN, BS, SS, WS */
#define FRIBIDI_MASK_EXPLICIT	0x00001000L	/* Is expilict mark: LRE, RLE, LRO, RLO, PDF */

/* Can be on only if FRIBIDI_MASK_SPACE is also on. */
#define FRIBIDI_MASK_SEPARATOR	0x00002000L	/* Is test separator: BS, SS */
/* Can be on only if FRIBIDI_MASK_EXPLICIT is also on. */
#define FRIBIDI_MASK_OVERRIDE	0x00004000L	/* Is explicit override: LRO, RLO */

/* The following must be to make types pairwise different, some of them can
   be removed but are here because of efficiency (make queries faster). */

#define FRIBIDI_MASK_ES		0x00010000L
#define FRIBIDI_MASK_ET		0x00020000L
#define FRIBIDI_MASK_CS		0x00040000L

#define FRIBIDI_MASK_NSM	0x00080000L
#define FRIBIDI_MASK_BN		0x00100000L

#define FRIBIDI_MASK_BS		0x00200000L
#define FRIBIDI_MASK_SS		0x00400000L
#define FRIBIDI_MASK_WS		0x00800000L

/* We reserve the sign bit for user's private use: we will never use it,
   then negative character types will be never assigned. */


/*
 * Define values for FriBidiCharType
 */

/* Strong left to right */
#define FRIBIDI_TYPE_LTR	( FRIBIDI_MASK_STRONG + FRIBIDI_MASK_LETTER )
/* Right to left characters */
#define FRIBIDI_TYPE_RTL	( FRIBIDI_MASK_STRONG + FRIBIDI_MASK_LETTER \
				+ FRIBIDI_MASK_RTL)
/* Arabic characters */
#define FRIBIDI_TYPE_AL		( FRIBIDI_MASK_STRONG + FRIBIDI_MASK_LETTER \
				+ FRIBIDI_MASK_RTL + FRIBIDI_MASK_ARABIC )
/* Left-To-Right embedding */
#define FRIBIDI_TYPE_LRE	(FRIBIDI_MASK_STRONG + FRIBIDI_MASK_EXPLICIT)
/* Right-To-Left embedding */
#define FRIBIDI_TYPE_RLE	( FRIBIDI_MASK_STRONG + FRIBIDI_MASK_EXPLICIT \
				+ FRIBIDI_MASK_RTL )
/* Left-To-Right override */
#define FRIBIDI_TYPE_LRO	( FRIBIDI_MASK_STRONG + FRIBIDI_MASK_EXPLICIT \
				+ FRIBIDI_MASK_OVERRIDE )
/* Right-To-Left override */
#define FRIBIDI_TYPE_RLO	( FRIBIDI_MASK_STRONG + FRIBIDI_MASK_EXPLICIT \
				+ FRIBIDI_MASK_RTL + FRIBIDI_MASK_OVERRIDE )

/* Pop directional override */
#define FRIBIDI_TYPE_PDF	( FRIBIDI_MASK_WEAK + FRIBIDI_MASK_EXPLICIT )
/* European digit */
#define FRIBIDI_TYPE_EN		( FRIBIDI_MASK_WEAK + FRIBIDI_MASK_NUMBER )
/* Arabic digit */
#define FRIBIDI_TYPE_AN		( FRIBIDI_MASK_WEAK + FRIBIDI_MASK_NUMBER \
				+ FRIBIDI_MASK_ARABIC )
/* European number separator */
#define FRIBIDI_TYPE_ES		( FRIBIDI_MASK_WEAK + FRIBIDI_MASK_NUMSEPTER \
				+ FRIBIDI_MASK_ES )
/* European number terminator */
#define FRIBIDI_TYPE_ET		( FRIBIDI_MASK_WEAK + FRIBIDI_MASK_NUMSEPTER \
				+ FRIBIDI_MASK_ET )
/* Common Separator */
#define FRIBIDI_TYPE_CS		( FRIBIDI_MASK_WEAK + FRIBIDI_MASK_NUMSEPTER \
				+ FRIBIDI_MASK_CS )
/* Non spacing mark */
#define FRIBIDI_TYPE_NSM	( FRIBIDI_MASK_WEAK + FRIBIDI_MASK_NSM )
/* Boundary neutral */
#define FRIBIDI_TYPE_BN		(  FRIBIDI_MASK_WEAK + FRIBIDI_MASK_SPACE \
				+ FRIBIDI_MASK_BN )

/* Block separator */
#define FRIBIDI_TYPE_BS		( FRIBIDI_MASK_NEUTRAL + FRIBIDI_MASK_SPACE \
				+ FRIBIDI_MASK_SEPARATOR + FRIBIDI_MASK_BS )
/* Segment separator */
#define FRIBIDI_TYPE_SS		( FRIBIDI_MASK_NEUTRAL + FRIBIDI_MASK_SPACE \
				+ FRIBIDI_MASK_SEPARATOR + FRIBIDI_MASK_SS )
/* Whitespace */
#define FRIBIDI_TYPE_WS		( FRIBIDI_MASK_NEUTRAL + FRIBIDI_MASK_SPACE \
				+ FRIBIDI_MASK_WS )
/* Other Neutral */
#define FRIBIDI_TYPE_ON		( FRIBIDI_MASK_NEUTRAL )

/* The following are used to identify the paragraph direction,
   types L, R, N are not used internally anymore, and recommended to use
   LTR, RTL and ON instead, didn't removed because of compatability. */
#define FRIBIDI_TYPE_L		( FRIBIDI_TYPE_LTR )
#define FRIBIDI_TYPE_R		( FRIBIDI_TYPE_RTL )
#define FRIBIDI_TYPE_N		( FRIBIDI_TYPE_ON )
/* Weak left to right */
#define FRIBIDI_TYPE_WL		( FRIBIDI_MASK_WEAK )
/* Weak right to left */
#define FRIBIDI_TYPE_WR		( FRIBIDI_MASK_WEAK + FRIBIDI_MASK_RTL )

/* The following are only used internally */

/* Start of text */
#define FRIBIDI_TYPE_SOT	( FRIBIDI_MASK_SENTINEL )
/* End of text */
#define FRIBIDI_TYPE_EOT	( FRIBIDI_MASK_SENTINEL + FRIBIDI_MASK_RTL )

/*
 * End of define values for FriBidiCharType
 */


/*
 * Defining macros for needed queries, It is fully dependent on the 
 * implementation of FriBidiCharType.
 */


/* Is private-use value? */
#define FRIBIDI_TYPE_PRIVATE(p)	((p) < 0)

/* Return the direction of the level number, FRIBIDI_TYPE_LTR for even and
   FRIBIDI_TYPE_RTL for odds. */
#define FRIBIDI_LEVEL_TO_DIR(lev) (FRIBIDI_TYPE_LTR | (lev & 1))

/* Return the minimum level of the direction, 0 for FRIBIDI_TYPE_LTR and
   1 for FRIBIDI_TYPE_RTL and FRIBIDI_TYPE_AL. */
#define FRIBIDI_DIR_TO_LEVEL(dir) ((FriBidiLevel)(dir & 1))

/* Is right to left? */
#define FRIBIDI_IS_RTL(p)      ((p) & FRIBIDI_MASK_RTL)
/* Is arabic? */
#define FRIBIDI_IS_ARABIC(p)   ((p) & FRIBIDI_MASK_ARABIC)

/* Is strong? */
#define FRIBIDI_IS_STRONG(p)   ((p) & FRIBIDI_MASK_STRONG)
/* Is weak? */
#define FRIBIDI_IS_WEAK(p)     ((p) & FRIBIDI_MASK_WEAK)
/* Is neutral? */
#define FRIBIDI_IS_NEUTRAL(p)  ((p) & FRIBIDI_MASK_NEUTRAL)
/* Is sentinel? */
#define FRIBIDI_IS_SENTINEL(p) ((p) & FRIBIDI_MASK_SENTINEL)

/* Is letter: L, R, AL? */
#define FRIBIDI_IS_LETTER(p)   ((p) & FRIBIDI_MASK_LETTER)
/* Is number: EN, AN? */
#define FRIBIDI_IS_NUMBER(p)   ((p) & FRIBIDI_MASK_NUMBER)
/* Is number separator or terminator: ES, ET, CS? */
#define FRIBIDI_IS_NUMBER_SEPARATOR_OR_TERMINATOR(p) \
	((p) & FRIBIDI_MASK_NUMSEPTER)
/* Is space: BN, BS, SS, WS? */
#define FRIBIDI_IS_SPACE(p)    ((p) & FRIBIDI_MASK_SPACE)
/* Is explicit mark: LRE, RLE, LRO, RLO, PDF? */
#define FRIBIDI_IS_EXPLICIT(p) ((p) & FRIBIDI_MASK_EXPLICIT)

/* Is test separator: BS, SS? */
#define FRIBIDI_IS_SEPARATOR(p) ((p) & FRIBIDI_MASK_SEPARATOR)

/* Is explicit override: LRO, RLO? */
#define FRIBIDI_IS_OVERRIDE(p) ((p) & FRIBIDI_MASK_OVERRIDE)

/* Some more: */

/* Is left to right letter: LTR? */
#define FRIBIDI_IS_LTR_LETTER(p) \
	((p) & (FRIBIDI_MASK_LETTER | FRIBIDI_MASK_RTL) == FRIBIDI_MASK_LETTER)

/* Is right to left letter: RTL, AL? */
#define FRIBIDI_IS_RTL_LETTER(p) \
	((p) & (FRIBIDI_MASK_LETTER | FRIBIDI_MASK_RTL) \
	== (FRIBIDI_MASK_LETTER | FRIBIDI_MASK_RTL))

/* Is ES or CS: ES, CS? */
#define FRIBIDI_IS_ES_OR_CS(p) \
	((p) & (FRIBIDI_MASK_ES | FRIBIDI_MASK_CS))

/* Is explicit or BN: LRE, RLE, LRO, RLO, PDF, BN? */
#define FRIBIDI_IS_EXPLICIT_OR_BN(p) \
	((p) & (FRIBIDI_MASK_EXPLICIT | FRIBIDI_MASK_BN))

/* Is explicit or separator or BN or WS: LRE, RLE, LRO, RLO, PDF, BS, SS, BN, WS? */
#define FRIBIDI_IS_EXPLICIT_OR_SEPARATOR_OR_BN_OR_WS(p) \
	((p) & (FRIBIDI_MASK_EXPLICIT | FRIBIDI_MASK_SEPARATOR \
		| FRIBIDI_MASK_BN | FRIBIDI_MASK_WS))

/* Define some conversions. */

/* Change numbers: EN, AN to RTL. */
#define FRIBIDI_CHANGE_NUMBER_TO_RTL(p) \
	(FRIBIDI_IS_NUMBER(p) ? FRIBIDI_TYPE_RTL : (p))

/* Override status of an explicit mark: LRO->LTR, RLO->RTL, otherwise->ON. */
#define FRIBIDI_EXPLICIT_TO_OVERRIDE_DIR(p) \
	(FRIBIDI_IS_OVERRIDE(p) ? FRIBIDI_LEVEL_TO_DIR(FRIBIDI_DIR_TO_LEVEL(p)) \
				: FRIBIDI_TYPE_ON)


/*
 * Define character types that char_type_tables use.
 * define them to be 0, 1, 2, ... and then in fribidi_char_type.c map them
 * to FriBidiCharTypes.
 */
  typedef char FriBidiPropCharType;

  enum FriBidiPropEnum
  {
#define _FRIBIDI_ADD_TYPE(TYPE) FRIBIDI_PROP_TYPE_##TYPE,
#include "fribidi_types.i"
#undef _FRIBIDI_ADD_TYPE
    FRIBIDI_TYPES_COUNT		/* Number of different character types */
  };

/* Map fribidi_prop_types to fribidi_types */
  extern const FriBidiCharType fribidi_prop_to_type[];

#ifdef	__cplusplus
}
#endif

#endif
