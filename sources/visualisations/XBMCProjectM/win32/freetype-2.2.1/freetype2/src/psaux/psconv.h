/***************************************************************************/
/*                                                                         */
/*  psconv.h                                                               */
/*                                                                         */
/*    Some convenience conversions (specification).                        */
/*                                                                         */
/*  Copyright 2006 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __PSCONV_H__
#define __PSCONV_H__


#include <ft2build.h>
#include FT_INTERNAL_POSTSCRIPT_AUX_H

FT_BEGIN_HEADER


  FT_LOCAL( FT_Int )
  PS_Conv_Strtol( FT_Byte**  cursor,
                  FT_Byte*   limit,
                  FT_Int     base );


  FT_LOCAL( FT_Int )
  PS_Conv_ToInt( FT_Byte**  cursor,
                 FT_Byte*   limit );

  FT_LOCAL( FT_Fixed )
  PS_Conv_ToFixed( FT_Byte**  cursor,
                   FT_Byte*   limit,
                   FT_Int     power_ten );

#if 0
  FT_LOCAL( FT_UInt )
  PS_Conv_StringDecode( FT_Byte**  cursor,
                        FT_Byte*   limit,
                        FT_Byte*   buffer,
                        FT_UInt    n );
#endif

  FT_LOCAL( FT_UInt )
  PS_Conv_ASCIIHexDecode( FT_Byte**  cursor,
                          FT_Byte*   limit,
                          FT_Byte*   buffer,
                          FT_UInt    n );

  FT_LOCAL( FT_UInt )
  PS_Conv_EexecDecode( FT_Byte**   cursor,
                       FT_Byte*    limit,
                       FT_Byte*    buffer,
                       FT_UInt     n,
                       FT_UShort*  seed );


#define IS_PS_NEWLINE( ch ) \
  ( ( ch ) == '\r' ||       \
    ( ch ) == '\n' )

#define IS_PS_SPACE( ch )  \
  ( ( ch ) == ' '       || \
    IS_PS_NEWLINE( ch ) || \
    ( ch ) == '\t'      || \
    ( ch ) == '\f'      || \
    ( ch ) == '\0' )

#define IS_PS_SPECIAL( ch ) \
  ( ( ch ) == '/' ||        \
    ( ch ) == '(' ||        \
    ( ch ) == ')' ||        \
    ( ch ) == '<' ||        \
    ( ch ) == '>' ||        \
    ( ch ) == '[' ||        \
    ( ch ) == ']' ||        \
    ( ch ) == '{' ||        \
    ( ch ) == '}' ||        \
    ( ch ) == '%' )

#define IS_PS_DELIM( ch )  \
  ( IS_PS_SPACE( ch )   || \
    IS_PS_SPECIAL( ch ) )

#define IS_PS_DIGIT( ch )  ( ( ch ) >= '0' && ( ch ) <= '9' )

#define IS_PS_XDIGIT( ch )                \
  ( IS_PS_DIGIT( ( ch ) )              || \
    ( ( ch ) >= 'A' && ( ch ) <= 'F' ) || \
    ( ( ch ) >= 'a' && ( ch ) <= 'f' ) )

#define IS_PS_BASE85( ch ) ( ( ch ) >= '!' && ( ch ) <= 'u' )

FT_END_HEADER

#endif /* __PSCONV_H__ */


/* END */
