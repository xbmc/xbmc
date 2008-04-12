/***************************************************************************/
/*                                                                         */
/*  psconv.c                                                               */
/*                                                                         */
/*    Some convenience conversions (body).                                 */
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


#include <ft2build.h>
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_INTERNAL_DEBUG_H

#include "psconv.h"
#include "psobjs.h"
#include "psauxerr.h"


  /* The following array is used by various functions to quickly convert */
  /* digits (both decimal and non-decimal) into numbers.                 */

#if 'A' == 65
  /* ASCII */

  static const FT_Char  ft_char_table[128] =
  {
    /* 0x00 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
  };

  /* no character >= 0x80 can represent a valid number */
#define OP  >=

#endif /* 'A' == 65 */

#if 'A' == 193
  /* EBCDIC */

  static const FT_Char  ft_char_table[128] =
  {
    /* 0x80 */
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, -1, -1, -1, -1, -1, -1,
    -1, 19, 20, 21, 22, 23, 24, 25, 26, 27, -1, -1, -1, -1, -1, -1,
    -1, -1, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, -1, -1, -1, -1, -1, -1,
    -1, 19, 20, 21, 22, 23, 24, 25, 26, 27, -1, -1, -1, -1, -1, -1,
    -1, -1, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
  };

  /* no character < 0x80 can represent a valid number */
#define OP  <

#endif /* 'A' == 193 */


  FT_LOCAL_DEF( FT_Int )
  PS_Conv_Strtol( FT_Byte**  cursor,
                  FT_Byte*   limit,
                  FT_Int     base )
  {
    FT_Byte*  p = *cursor;
    FT_Int    num = 0;
    FT_Bool   sign = 0;


    if ( p == limit || base < 2 || base > 36 )
      return 0;

    if ( *p == '-' || *p == '+' )
    {
      sign = FT_BOOL( *p == '-' );

      p++;
      if ( p == limit )
        return 0;
    }

    for ( ; p < limit; p++ )
    {
      FT_Char  c;


      if ( IS_PS_SPACE( *p ) || *p OP 0x80 )
        break;

      c = ft_char_table[*p & 0x7f];

      if ( c < 0 || c >= base )
        break;

      num = num * base + c;
    }

    if ( sign )
      num = -num;

    *cursor = p;

    return num;
  }


  FT_LOCAL_DEF( FT_Int )
  PS_Conv_ToInt( FT_Byte**  cursor,
                 FT_Byte*   limit )

  {
    FT_Byte*  p;
    FT_Int    num;


    num = PS_Conv_Strtol( cursor, limit, 10 );
    p   = *cursor;

    if ( p < limit && *p == '#' )
    {
      *cursor = p + 1;

      return PS_Conv_Strtol( cursor, limit, num );
    }
    else
      return num;
  }


  FT_LOCAL_DEF( FT_Fixed )
  PS_Conv_ToFixed( FT_Byte**  cursor,
                   FT_Byte*   limit,
                   FT_Int     power_ten )
  {
    FT_Byte*  p = *cursor;
    FT_Fixed  integral;
    FT_Long   decimal = 0, divider = 1;
    FT_Bool   sign = 0;


    if ( p == limit )
      return 0;

    if ( *p == '-' || *p == '+' )
    {
      sign = FT_BOOL( *p == '-' );

      p++;
      if ( p == limit )
        return 0;
    }

    if ( *p != '.' )
      integral = PS_Conv_ToInt( &p, limit ) << 16;
    else
      integral = 0;

    /* read the decimal part */
    if ( p < limit && *p == '.' )
    {
      p++;

      for ( ; p < limit; p++ )
      {
        FT_Char  c;


        if ( IS_PS_SPACE( *p ) || *p OP 0x80 )
          break;

        c = ft_char_table[*p & 0x7f];

        if ( c < 0 || c >= 10 )
          break;

        if ( divider < 10000000L )
        {
          decimal = decimal * 10 + c;
          divider *= 10;
        }
      }
    }

    /* read exponent, if any */
    if ( p + 1 < limit && ( *p == 'e' || *p == 'E' ) )
    {
      p++;
      power_ten += PS_Conv_ToInt( &p, limit );
    }

    while ( power_ten > 0 )
    {
      integral *= 10;
      decimal  *= 10;
      power_ten--;
    }

    while ( power_ten < 0 )
    {
      integral /= 10;
      divider  *= 10;
      power_ten++;
    }

    if ( decimal )
      integral += FT_DivFix( decimal, divider );

    if ( sign )
      integral = -integral;

    *cursor = p;

    return integral;
  }


#if 0
  FT_LOCAL_DEF( FT_UInt )
  PS_Conv_StringDecode( FT_Byte**  cursor,
                        FT_Byte*   limit,
                        FT_Byte*   buffer,
                        FT_UInt    n )
  {
    FT_Byte*  p;
    FT_UInt   r = 0;


    for ( p = *cursor; r < n && p < limit; p++ )
    {
      FT_Byte  b;


      if ( *p != '\\' )
      {
        buffer[r++] = *p;

        continue;
      }

      p++;

      switch ( *p )
      {
      case 'n':
        b = '\n';
        break;
      case 'r':
        b = '\r';
        break;
      case 't':
        b = '\t';
        break;
      case 'b':
        b = '\b';
        break;
      case 'f':
        b = '\f';
        break;
      case '\r':
        p++;
        if ( *p != '\n' )
        {
          b = *p;

          break;
        }
        /* no break */
      case '\n':
        continue;
        break;
      default:
        if ( IS_PS_DIGIT( *p ) )
        {
          b = *p - '0';

          p++;

          if ( IS_PS_DIGIT( *p ) )
          {
            b = b * 8 + *p - '0';

            p++;

            if ( IS_PS_DIGIT( *p ) )
              b = b * 8 + *p - '0';
            else
            {
              buffer[r++] = b;
              b = *p;
            }
          }
          else
          {
            buffer[r++] = b;
            b = *p;
          }
        }
        else
          b = *p;
        break;
      }

      buffer[r++] = b;
    }

    *cursor = p;

    return r;
  }
#endif /* 0 */


  FT_LOCAL_DEF( FT_UInt )
  PS_Conv_ASCIIHexDecode( FT_Byte**  cursor,
                          FT_Byte*   limit,
                          FT_Byte*   buffer,
                          FT_UInt    n )
  {
    FT_Byte*  p;
    FT_UInt   r = 0;


    n *= 2;
    for ( p = *cursor; r < n && p < limit; p++ )
    {
      FT_Char  c;


      if ( IS_PS_SPACE( *p ) )
        continue;

      if ( *p OP 0x80 )
        break;

      c = ft_char_table[*p & 0x7f];

      if ( c < 0 || c >= 16 )
        break;

      if ( r % 2 )
      {
        *buffer = (FT_Byte)(*buffer + c);
        buffer++;
      }
      else
        *buffer = (FT_Byte)(c << 4);

      r++;
    }

    *cursor = p;

    return ( r + 1 ) / 2;
  }


  FT_LOCAL_DEF( FT_UInt )
  PS_Conv_EexecDecode( FT_Byte**   cursor,
                       FT_Byte*    limit,
                       FT_Byte*    buffer,
                       FT_UInt     n,
                       FT_UShort*  seed )
  {
    FT_Byte*   p;
    FT_UInt    r;
    FT_UShort  s = *seed;


    for ( r = 0, p = *cursor; r < n && p < limit; r++, p++ )
    {
      FT_Byte  b = (FT_Byte)( *p ^ ( s >> 8 ) );


      s = (FT_UShort)( ( *p + s ) * 52845U + 22719 );
      *buffer++ = b;
    }

    *cursor = p;
    *seed   = s;

    return r;
  }


/* END */
