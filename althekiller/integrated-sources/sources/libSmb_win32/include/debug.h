/* 
   Unix SMB/CIFS implementation.
   SMB debug stuff
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) John H Terpstra 1996-1998
   Copyright (C) Luke Kenneth Casson Leighton 1996-1998
   Copyright (C) Paul Ashton 1998
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _DEBUG_H
#define _DEBUG_H

/* -------------------------------------------------------------------------- **
 * Debugging code.  See also debug.c
 */

/* mkproto.awk has trouble with ifdef'd function definitions (it ignores
 * the #ifdef directive and will read both definitions, thus creating two
 * diffferent prototype declarations), so we must do these by hand.
 */
/* I know the __attribute__ stuff is ugly, but it does ensure we get the 
   arguments to DEBUG() right. We have got them wrong too often in the 
   past.
   The PRINTFLIKE comment does the equivalent for SGI MIPSPro.
 */
/* PRINTFLIKE1 */
int  Debug1( const char *, ... ) PRINTF_ATTRIBUTE(1,2);
/* PRINTFLIKE1 */
BOOL dbgtext( const char *, ... ) PRINTF_ATTRIBUTE(1,2);
BOOL dbghdr( int level, const char *file, const char *func, int line );

#if defined(sgi) && (_COMPILER_VERSION >= 730)
#pragma mips_frequency_hint NEVER Debug1
#pragma mips_frequency_hint NEVER dbgtext
#pragma mips_frequency_hint NEVER dbghdr
#endif

extern XFILE *dbf;
extern pstring debugf;

/* If we have these macros, we can add additional info to the header. */

#ifdef HAVE_FUNCTION_MACRO
#define FUNCTION_MACRO  (__FUNCTION__)
#else
#define FUNCTION_MACRO  ("")
#endif

/* 
 * Redefine DEBUGLEVEL because so we don't have to change every source file
 * that *unnecessarily* references it. Source files neeed not extern reference 
 * DEBUGLEVEL, as it's extern in includes.h (which all source files include).
 * Eventually, all these references should be removed, and all references to
 * DEBUGLEVEL should be references to DEBUGLEVEL_CLASS[DBGC_ALL]. This could
 * still be through a macro still called DEBUGLEVEL. This cannot be done now
 * because some references would expand incorrectly.
 */
#define DEBUGLEVEL *debug_level
extern int DEBUGLEVEL;

/*
 * Define all new debug classes here. A class is represented by an entry in
 * the DEBUGLEVEL_CLASS array. Index zero of this arrray is equivalent to the
 * old DEBUGLEVEL. Any source file that does NOT add the following lines:
 *
 *   #undef  DBGC_CLASS
 *   #define DBGC_CLASS DBGC_<your class name here>
 *
 * at the start of the file (after #include "includes.h") will default to
 * using index zero, so it will behaive just like it always has. 
 */
#define DBGC_ALL		0 /* index equivalent to DEBUGLEVEL */

#define DBGC_TDB		1
#define DBGC_PRINTDRIVERS	2
#define DBGC_LANMAN		3
#define DBGC_SMB		4
#define DBGC_RPC_PARSE		5
#define DBGC_RPC_SRV		6
#define DBGC_RPC_CLI		7
#define DBGC_PASSDB		8
#define DBGC_SAM		9
#define DBGC_AUTH		10
#define DBGC_WINBIND		11
#define DBGC_VFS		12
#define DBGC_IDMAP		13
#define DBGC_QUOTA		14
#define DBGC_ACLS		15
#define DBGC_LOCKING		16
#define DBGC_MSDFS		17
#define DBGC_DMAPI		18

/* So you can define DBGC_CLASS before including debug.h */
#ifndef DBGC_CLASS
#define DBGC_CLASS            0     /* override as shown above */
#endif

extern int  *DEBUGLEVEL_CLASS;
extern BOOL *DEBUGLEVEL_CLASS_ISSET;

/* Debugging macros
 *
 * DEBUGLVL()
 *   If the 'file specific' debug class level >= level OR the system-wide 
 *   DEBUGLEVEL (synomym for DEBUGLEVEL_CLASS[ DBGC_ALL ]) >= level then
 *   generate a header using the default macros for file, line, and 
 *   function name. Returns True if the debug level was <= DEBUGLEVEL.
 * 
 *   Example: if( DEBUGLVL( 2 ) ) dbgtext( "Some text.\n" );
 *
 * DEBUGLVLC()
 *   If the 'macro specified' debug class level >= level OR the system-wide 
 *   DEBUGLEVEL (synomym for DEBUGLEVEL_CLASS[ DBGC_ALL ]) >= level then 
 *   generate a header using the default macros for file, line, and 
 *   function name. Returns True if the debug level was <= DEBUGLEVEL.
 * 
 *   Example: if( DEBUGLVLC( DBGC_TDB, 2 ) ) dbgtext( "Some text.\n" );
 *
 * DEBUG()
 *   If the 'file specific' debug class level >= level OR the system-wide 
 *   DEBUGLEVEL (synomym for DEBUGLEVEL_CLASS[ DBGC_ALL ]) >= level then 
 *   generate a header using the default macros for file, line, and 
 *   function name. Each call to DEBUG() generates a new header *unless* the 
 *   previous debug output was unterminated (i.e. no '\n').
 *   See debug.c:dbghdr() for more info.
 *
 *   Example: DEBUG( 2, ("Some text and a value %d.\n", value) );
 *
 * DEBUGC()
 *   If the 'macro specified' debug class level >= level OR the system-wide 
 *   DEBUGLEVEL (synomym for DEBUGLEVEL_CLASS[ DBGC_ALL ]) >= level then 
 *   generate a header using the default macros for file, line, and 
 *   function name. Each call to DEBUG() generates a new header *unless* the 
 *   previous debug output was unterminated (i.e. no '\n').
 *   See debug.c:dbghdr() for more info.
 *
 *   Example: DEBUGC( DBGC_TDB, 2, ("Some text and a value %d.\n", value) );
 *
 *  DEBUGADD(), DEBUGADDC()
 *    Same as DEBUG() and DEBUGC() except the text is appended to the previous
 *    DEBUG(), DEBUGC(), DEBUGADD(), DEBUGADDC() with out another interviening 
 *    header.
 *
 *    Example: DEBUGADD( 2, ("Some text and a value %d.\n", value) );
 *             DEBUGADDC( DBGC_TDB, 2, ("Some text and a value %d.\n", value) );
 *
 * Note: If the debug class has not be redeined (see above) then the optimizer 
 * will remove the extra conditional test.
 */

#define DEBUGLVL( level ) \
  ( ((level) <= MAX_DEBUG_LEVEL) && \
     ((DEBUGLEVEL_CLASS[ DBGC_CLASS ] >= (level))||  \
     (!DEBUGLEVEL_CLASS_ISSET[ DBGC_CLASS ] && \
      DEBUGLEVEL_CLASS[ DBGC_ALL   ] >= (level))  ) \
   && dbghdr( level, __FILE__, FUNCTION_MACRO, (__LINE__) ) )


#define DEBUGLVLC( dbgc_class, level ) \
  ( ((level) <= MAX_DEBUG_LEVEL) && \
     ((DEBUGLEVEL_CLASS[ dbgc_class ] >= (level))||  \
     (!DEBUGLEVEL_CLASS_ISSET[ dbgc_class ] && \
      DEBUGLEVEL_CLASS[ DBGC_ALL   ] >= (level))  ) \
   && dbghdr( level, __FILE__, FUNCTION_MACRO, (__LINE__) ) )


#define DEBUG( level, body ) \
  (void)( ((level) <= MAX_DEBUG_LEVEL) && \
           ((DEBUGLEVEL_CLASS[ DBGC_CLASS ] >= (level))||  \
           (!DEBUGLEVEL_CLASS_ISSET[ DBGC_CLASS ] && \
            DEBUGLEVEL_CLASS[ DBGC_ALL   ] >= (level))  ) \
       && (dbghdr( level, __FILE__, FUNCTION_MACRO, (__LINE__) )) \
       && (dbgtext body) )

#define DEBUGC( dbgc_class, level, body ) \
  (void)( ((level) <= MAX_DEBUG_LEVEL) && \
           ((DEBUGLEVEL_CLASS[ dbgc_class ] >= (level))||  \
           (!DEBUGLEVEL_CLASS_ISSET[ dbgc_class ] && \
	    DEBUGLEVEL_CLASS[ DBGC_ALL   ] >= (level))  ) \
       && (dbghdr( level, __FILE__, FUNCTION_MACRO, (__LINE__) )) \
       && (dbgtext body) )

#define DEBUGADD( level, body ) \
  (void)( ((level) <= MAX_DEBUG_LEVEL) && \
           ((DEBUGLEVEL_CLASS[ DBGC_CLASS ] >= (level))||  \
           (!DEBUGLEVEL_CLASS_ISSET[ DBGC_CLASS ] && \
            DEBUGLEVEL_CLASS[ DBGC_ALL   ] >= (level))  ) \
       && (dbgtext body) )

#define DEBUGADDC( dbgc_class, level, body ) \
  (void)( ((level) <= MAX_DEBUG_LEVEL) && \
          ((DEBUGLEVEL_CLASS[ dbgc_class ] >= (level))||  \
           (!DEBUGLEVEL_CLASS_ISSET[ dbgc_class ] && \
            DEBUGLEVEL_CLASS[ DBGC_ALL   ] >= (level))  ) \
       && (dbgtext body) )

/* Print a separator to the debug log. */
#define DEBUGSEP(level)\
	DEBUG((level),("===============================================================\n"))

#endif
