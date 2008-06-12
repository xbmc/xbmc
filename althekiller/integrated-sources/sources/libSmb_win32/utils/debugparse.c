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

#include "debugparse.h"

/* -------------------------------------------------------------------------- **
 * Constants...
 *
 *  DBG_BSIZE - This internal constant is used only by dbg_test().  It is the
 *          size of the read buffer.  I've tested the function using a
 *          DBG_BSIZE value of 2.
 */

#define DBG_BSIZE 128

/* -------------------------------------------------------------------------- **
 * Functions...
 */

const char *dbg_token2string( dbg_Token tok )
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
  {
  switch( tok )
    {
    case dbg_null:
      return( "null" );
    case dbg_ignore:
      return( "ignore" );
    case dbg_header:
      return( "header" );
    case dbg_timestamp:
      return( "time stamp" );
    case dbg_level:
      return( "level" );
    case dbg_sourcefile:
      return( "source file" );
    case dbg_function:
      return( "function" );
    case dbg_lineno:
      return( "line number" );
    case dbg_message:
      return( "message" );
    case dbg_eof:
      return( "[EOF]" );
    }
  return( "<unknown>" );
  } /* dbg_token2string */

dbg_Token dbg_char2token( dbg_Token *state, int c )
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
  {
  /* The terminating characters that we see will greatly depend upon
   * how they are read.  For example, if gets() is used instead of
   * fgets(), then we will not see newline characters.  A lot also
   * depends on the calling function, which may handle terminators
   * itself.
   *
   * '\n', '\0', and EOF are all considered line terminators.  The
   * dbg_eof token is sent back if an EOF is encountered.
   *
   * Warning:  only allow the '\0' character to be sent if you are
   *           using gets() to read whole lines (thus replacing '\n'
   *           with '\0').  Sending '\0' at the wrong time will mess
   *           up the parsing.
   */
  switch( c )
    {
    case EOF:
      *state = dbg_null;   /* Set state to null (initial state) so */
      return( dbg_eof );   /* that we can restart with new input.  */
    case '\n':
    case '\0':
      *state = dbg_null;   /* A newline or eoln resets to the null state. */
      return( dbg_null );
    }

  /* When within the body of the message, only a line terminator
   * can cause a change of state.  We've already checked for line
   * terminators, so if the current state is dbg_msgtxt, simply
   * return that as our current token.
   */
  if( dbg_message == *state )
    return( dbg_message );

  /* If we are at the start of a new line, and the input character 
   * is an opening bracket, then the line is a header line, otherwise
   * it's a message body line.
   */
  if( dbg_null == *state )
    {
    if( '[' == c )
      {
      *state = dbg_timestamp;
      return( dbg_header );
      }
    *state = dbg_message;
    return( dbg_message );
    }

  /* We've taken care of terminators, text blocks and new lines.
   * The remaining possibilities are all within the header line
   * itself.
   */

  /* Within the header line, whitespace can be ignored *except*
   * within the timestamp.
   */
  if( isspace( c ) )
    {
    /* Fudge.  The timestamp may contain space characters. */
    if( (' ' == c) && (dbg_timestamp == *state) )
      return( dbg_timestamp );
    /* Otherwise, ignore whitespace. */
    return( dbg_ignore );
    }

  /* Okay, at this point we know we're somewhere in the header.
   * Valid header *states* are: dbg_timestamp, dbg_level,
   * dbg_sourcefile, dbg_function, and dbg_lineno.
   */
  switch( c )
    {
    case ',':
      if( dbg_timestamp == *state )
        {
        *state = dbg_level;
        return( dbg_ignore );
        }
      break;
    case ']':
      if( dbg_level == *state )
        {
        *state = dbg_sourcefile;
        return( dbg_ignore );
        }
      break;
    case ':':
      if( dbg_sourcefile == *state )
        {
        *state = dbg_function;
        return( dbg_ignore );
        }
      break;
    case '(':
      if( dbg_function == *state )
        {
        *state = dbg_lineno;
        return( dbg_ignore );
        }
      break;
    case ')':
      if( dbg_lineno == *state )
        {
        *state = dbg_null;
        return( dbg_ignore );
        }
      break;
    }

  /* If the previous block did not result in a state change, then
   * return the current state as the current token.
   */
  return( *state );
  } /* dbg_char2token */

void dbg_test( void )
  /* ------------------------------------------------------------------------ **
   * Simple test function.
   *
   *  Input:  none.
   *  Output: none.
   *  Notes:  This function was used to test dbg_char2token().  It reads a
   *          Samba log file from stdin and prints parsing info to stdout.
   *          It also serves as a simple example.
   *
   * ------------------------------------------------------------------------ **
   */
  {
  char bufr[DBG_BSIZE];
  int  i;
  int  linecount  = 1;
  dbg_Token old   = dbg_null,
            newtok= dbg_null,
            state = dbg_null;

  while( fgets( bufr, DBG_BSIZE, stdin ) )
    {
    for( i = 0; bufr[i]; i++ )
      {
      old = newtok;
      newtok = dbg_char2token( &state, bufr[i] );
      switch( newtok )
        {
        case dbg_header:
          if( linecount > 1 )
            (void)putchar( '\n' );
          break;
        case dbg_null:
          linecount++;
          break;
        case dbg_ignore:
          break;
        default:
          if( old != newtok )
            (void)printf( "\n[%05d]%12s: ", linecount, dbg_token2string(newtok) );
          (void)putchar( bufr[i] );
        }
      }
    }
  (void)putchar( '\n' );
  } /* dbg_test */


/* -------------------------------------------------------------------------- **
 * This simple main line can be uncommented and used to test the parser.
 */

/*
 * int main( void )
 *  {
 *  dbg_test();
 *  return( 0 );
 *  }
 */

/* ========================================================================== */
