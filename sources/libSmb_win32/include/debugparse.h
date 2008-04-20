#ifndef DEBUGPARSE_H
#define DEBUGPARSE_H
/* ========================================================================== **
 *                                debugparse.c
 *
 * Copyright (C) 1998 by Christopher R. Hertel
 *
 * Email: crh@ubiqx.mn.org
 *
 * -------------------------------------------------------------------------- **
 * This module is a very simple parser for Samba debug log files.
 * -------------------------------------------------------------------------- **
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * -------------------------------------------------------------------------- **
 * The important function in this module is dbg_char2token().  The rest is
 * basically fluff.  (Potentially useful fluff, but still fluff.)
 * ========================================================================== **
 */

#include "includes.h"

/* This module compiles quite nicely outside of the Samba environment.
 * You'll need the following headers:
#include <ctype.h>
#include <stdio.h>
#include <string.h>
 */

/* -------------------------------------------------------------------------- **
 * These are the tokens returned by dbg_char2token().
 */

typedef enum
  {
  dbg_null = 0,
  dbg_ignore,
  dbg_header,
  dbg_timestamp,
  dbg_level,
  dbg_sourcefile,
  dbg_function,
  dbg_lineno,
  dbg_message,
  dbg_eof
  } dbg_Token;

/* -------------------------------------------------------------------------- **
 * Function prototypes...
 */

 const char *dbg_token2string( dbg_Token tok );
  /* ------------------------------------------------------------------------ **
   * Given a token, return a string describing the token.
   *
   *  Input:  tok - One of the set of dbg_Tokens defined in debugparse.h.
   *
   *  Output: A string identifying the token.  This is useful for debugging,
   *          etc.
   *
   *  Note:   If the token is not known, this function will return the
   *          string "<unknown>".
   *
   * ------------------------------------------------------------------------ **
   */

 dbg_Token dbg_char2token( dbg_Token *state, int c );
  /* ------------------------------------------------------------------------ **
   * Parse input one character at a time.
   *
   *  Input:  state - A pointer to a token variable.  This is used to
   *                  maintain the parser state between calls.  For
   *                  each input stream, you should set up a separate
   *                  state variable and initialize it to dbg_null.
   *                  Pass a pointer to it into this function with each
   *                  character in the input stream.  See dbg_test()
   *                  for an example.
   *          c     - The "current" character in the input stream.
   *
   *  Output: A token.
   *          The token value will change when delimiters are found,
   *          which indicate a transition between syntactical objects.
   *          Possible return values are:
   *
   *          dbg_null        - The input character was an end-of-line.
   *                            This resets the parser to its initial state
   *                            in preparation for parsing the next line.
   *          dbg_eof         - Same as dbg_null, except that the character
   *                            was an end-of-file.
   *          dbg_ignore      - Returned for whitespace and delimiters.
   *                            These lexical tokens are only of interest
   *                            to the parser.
   *          dbg_header      - Indicates the start of a header line.  The
   *                            input character was '[' and was the first on
   *                            the line.
   *          dbg_timestamp   - Indicates that the input character was part
   *                            of a header timestamp.
   *          dbg_level       - Indicates that the input character was part
   *                            of the debug-level value in the header.
   *          dbg_sourcefile  - Indicates that the input character was part
   *                            of the sourcefile name in the header.
   *          dbg_function    - Indicates that the input character was part
   *                            of the function name in the header.
   *          dbg_lineno      - Indicates that the input character was part
   *                            of the DEBUG call line number in the header.
   *          dbg_message     - Indicates that the input character was part
   *                            of the DEBUG message text.
   *
   * ------------------------------------------------------------------------ **
   */


/* -------------------------------------------------------------------------- */
#endif /* DEBUGPARSE_H */
