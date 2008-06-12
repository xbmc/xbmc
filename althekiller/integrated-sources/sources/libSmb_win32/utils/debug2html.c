/* ========================================================================== **
 *                                debug2html.c
 *
 * Copyright (C) 1998 by Christopher R. Hertel
 *
 * Email: crh@ubiqx.mn.org
 *
 * -------------------------------------------------------------------------- **
 * Parse Samba debug logs (2.0 & greater) and output the results as HTML.
 * -------------------------------------------------------------------------- **
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * -------------------------------------------------------------------------- **
 * This program provides an example of the use of debugparse.c, and also
 * does a decent job of converting Samba logs into HTML.
 * -------------------------------------------------------------------------- **
 *
 * Revision 1.4  1998/11/13 03:37:01  tridge
 * fixes for OSF1 compilation
 *
 * Revision 1.3  1998/10/28 20:33:35  crh
 * I've moved the debugparse module files into the ubiqx directory because I
 * know that 'make proto' will ignore them there.  The debugparse.h header
 * file is included in includes.h, and includes.h is included in debugparse.c,
 * so all of the pieces "see" each other.  I've compiled and tested this,
 * and it does seem to work.  It's the same compromise model I used when
 * adding the ubiqx modules into the system, which is why I put it all into
 * the same directory.
 *
 * Chris -)-----
 *
 * Revision 1.1  1998/10/26 23:21:37  crh
 * Here is the simple debug parser and the debug2html converter.  Still to do:
 *
 *   * Debug message filtering.
 *   * I need to add all this to Makefile.in
 *     (If it looks at all strange I'll ask for help.)
 *
 * If you want to compile debug2html, you'll need to do it by hand until I
 * make the changes to Makefile.in.  Sorry.
 *
 * Chris -)-----
 *
 * ========================================================================== **
 */

#include "debugparse.h"

/* -------------------------------------------------------------------------- **
 * The size of the read buffer.
 */

#define DBG_BSIZE 1024

/* -------------------------------------------------------------------------- **
 * Functions...
 */

static dbg_Token modechange( dbg_Token newmode, dbg_Token mode )
  /* ------------------------------------------------------------------------ **
   * Handle a switch between header and message printing.
   *
   *  Input:  new   - The token value of the current token.  This indicates
   *                  the lexical item currently being recognized.
   *          mode  - The current mode.  This is either dbg_null or
   *                  dbg_message.  It could really be any toggle
   *                  (true/false, etc.)
   *
   *  Output: The new mode.  This will be the same as the input mode unless
   *          there was a transition in or out of message processing.
   *
   *  Notes:  The purpose of the mode value is to mark the beginning and end
   *          of the message text block.  In order to show the text in its
   *          correct format, it must be included within a <PRE></PRE> block.
   *
   * ------------------------------------------------------------------------ **
   */
  {
  switch( newmode )
    {
    case dbg_null:
    case dbg_ignore:
      return( mode );
    case dbg_message:
      if( dbg_message != mode )
        {
        /* Switching to message mode. */
        (void)printf( "<PRE>\n" );
        return( dbg_message );
        }
      break;
    default:
      if( dbg_message == mode )
        {
        /* Switching out of message mode. */
        (void)printf( "</PRE>\n\n" );
        return( dbg_null );
        }
    }

  return( mode );
  } /* modechange */

static void newblock( dbg_Token old, dbg_Token newtok )
  /* ------------------------------------------------------------------------ **
   * Handle the transition between tokens.
   *
   *  Input:  old - The previous token.
   *          new - The current token.
   *
   *  Output: none.
   *
   *  Notes:  This is called whenever there is a transition from one token
   *          type to another.  It first prints the markup tags that close
   *          the previous token, and then the markup tags for the new
   *          token.
   *
   * ------------------------------------------------------------------------ **
   */
  {
  switch( old )
    {
    case dbg_timestamp:
      (void)printf( ",</B>" );
      break;
    case dbg_level:
      (void)printf( "</FONT>]</B>\n   " );
      break;
    case dbg_sourcefile:
      (void)printf( ":" );
      break;
    case dbg_lineno:
      (void)printf( ")" );
      break;
    }

  switch( newtok )
    {
    case dbg_timestamp:
      (void)printf( "<B>[" );
      break;
    case dbg_level:
      (void)printf( " <B><FONT COLOR=MAROON>" );
      break;
    case dbg_lineno:
      (void)printf( "(" );
      break;
    }
  } /* newblock */

static void charprint( dbg_Token tok, int c )
  /* ------------------------------------------------------------------------ **
   * Filter the input characters to determine what goes to output.
   *
   *  Input:  tok - The token value of the current character.
   *          c   - The current character.
   *
   *  Output: none.
   *
   * ------------------------------------------------------------------------ **
   */
  {
  switch( tok )
    {
    case dbg_ignore:
    case dbg_header:
      break;
    case dbg_null:
    case dbg_eof:
      (void)putchar( '\n' );
      break;
    default:
      switch( c )
        {
        case '<':
          (void)printf( "&lt;" );
          break;
        case '>':
          (void)printf( "&gt;" );
          break;
        case '&':
          (void)printf( "&amp;" );
          break;
        case '\"':
          (void)printf( "&#34;" );
          break;
        default:
          (void)putchar( c );
          break;
        }
    }
  } /* charprint */

int main( int argc, char *argv[] )
  /* ------------------------------------------------------------------------ **
   * This simple program scans and parses Samba debug logs, and produces HTML
   * output.
   *
   *  Input:  argc  - Currently ignored.
   *          argv  - Currently ignored.
   *
   *  Output: Always zero.
   *
   *  Notes:  The HTML output is sent to stdout.
   *
   * ------------------------------------------------------------------------ **
   */
  {
  int       i;
  int       len;
  char      bufr[DBG_BSIZE];
  dbg_Token old   = dbg_null,
            newtok = dbg_null,
            state = dbg_null,
            mode  = dbg_null;

  (void)printf( "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\n" );
  (void)printf( "<HTML>\n<HEAD>\n" );
  (void)printf( "  <TITLE>Samba Debug Output</TITLE>\n</HEAD>\n\n<BODY>\n" );

  while( (!feof( stdin ))
      && ((len = fread( bufr, 1, DBG_BSIZE, stdin )) > 0) )
    {
    for( i = 0; i < len; i++ )
      {
      old = newtok;
      newtok = dbg_char2token( &state, bufr[i] );
      if( newtok != old )
        {
        mode = modechange( newtok, mode );
        newblock( old, newtok );
        }
      charprint( newtok, bufr[i] );
      }
    }
  (void)modechange( dbg_eof, mode );

  (void)printf( "</BODY>\n</HTML>\n" );
  return( 0 );
  } /* main */
