/***************************************************************************/
/*                                                                         */
/*  ftcalc.c                                                               */
/*                                                                         */
/*    Arithmetic computations (body).                                      */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2005, 2006 by                   */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* Support for 1-complement arithmetic has been totally dropped in this  */
  /* release.  You can still write your own code if you need it.           */
  /*                                                                       */
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* Implementing basic computation routines.                              */
  /*                                                                       */
  /* FT_MulDiv(), FT_MulFix(), FT_DivFix(), FT_RoundFix(), FT_CeilFix(),   */
  /* and FT_FloorFix() are declared in freetype.h.                         */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_OBJECTS_H


/* we need to define a 64-bits data type here */

#ifdef FT_LONG64

  typedef FT_INT64  FT_Int64;

#else

  typedef struct  FT_Int64_
  {
    FT_UInt32  lo;
    FT_UInt32  hi;

  } FT_Int64;

#endif /* FT_LONG64 */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_calc


  /* The following three functions are available regardless of whether */
  /* FT_LONG64 is defined.                                             */

  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Fixed )
  FT_RoundFix( FT_Fixed  a )
  {
    return ( a >= 0 ) ?   ( a + 0x8000L ) & ~0xFFFFL
                      : -((-a + 0x8000L ) & ~0xFFFFL );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Fixed )
  FT_CeilFix( FT_Fixed  a )
  {
    return ( a >= 0 ) ?   ( a + 0xFFFFL ) & ~0xFFFFL
                      : -((-a + 0xFFFFL ) & ~0xFFFFL );
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Fixed )
  FT_FloorFix( FT_Fixed  a )
  {
    return ( a >= 0 ) ?   a & ~0xFFFFL
                      : -((-a) & ~0xFFFFL );
  }


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( FT_Int32 )
  FT_Sqrt32( FT_Int32  x )
  {
    FT_ULong  val, root, newroot, mask;


    root = 0;
    mask = 0x40000000L;
    val  = (FT_ULong)x;

    do
    {
      newroot = root + mask;
      if ( newroot <= val )
      {
        val -= newroot;
        root = newroot + mask;
      }

      root >>= 1;
      mask >>= 2;

    } while ( mask != 0 );

    return root;
  }

#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */


#ifdef FT_LONG64


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )
  FT_MulDiv( FT_Long  a,
             FT_Long  b,
             FT_Long  c )
  {
    FT_Int   s;
    FT_Long  d;


    s = 1;
    if ( a < 0 ) { a = -a; s = -1; }
    if ( b < 0 ) { b = -b; s = -s; }
    if ( c < 0 ) { c = -c; s = -s; }

    d = (FT_Long)( c > 0 ? ( (FT_Int64)a * b + ( c >> 1 ) ) / c
                         : 0x7FFFFFFFL );

    return ( s > 0 ) ? d : -d;
  }


#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

  /* documentation is in ftcalc.h */

  FT_BASE_DEF( FT_Long )
  FT_MulDiv_No_Round( FT_Long  a,
                      FT_Long  b,
                      FT_Long  c )
  {
    FT_Int   s;
    FT_Long  d;


    s = 1;
    if ( a < 0 ) { a = -a; s = -1; }
    if ( b < 0 ) { b = -b; s = -s; }
    if ( c < 0 ) { c = -c; s = -s; }

    d = (FT_Long)( c > 0 ? (FT_Int64)a * b / c
                         : 0x7FFFFFFFL );

    return ( s > 0 ) ? d : -d;
  }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )
  FT_MulFix( FT_Long  a,
             FT_Long  b )
  {
    FT_Int   s = 1;
    FT_Long  c;


    if ( a < 0 ) { a = -a; s = -1; }
    if ( b < 0 ) { b = -b; s = -s; }

    c = (FT_Long)( ( (FT_Int64)a * b + 0x8000L ) >> 16 );
    return ( s > 0 ) ? c : -c ;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )
  FT_DivFix( FT_Long  a,
             FT_Long  b )
  {
    FT_Int32   s;
    FT_UInt32  q;

    s = 1;
    if ( a < 0 ) { a = -a; s = -1; }
    if ( b < 0 ) { b = -b; s = -s; }

    if ( b == 0 )
      /* check for division by 0 */
      q = 0x7FFFFFFFL;
    else
      /* compute result directly */
      q = (FT_UInt32)( ( ( (FT_Int64)a << 16 ) + ( b >> 1 ) ) / b );

    return ( s < 0 ? -(FT_Long)q : (FT_Long)q );
  }


#else /* FT_LONG64 */


  static void
  ft_multo64( FT_UInt32  x,
              FT_UInt32  y,
              FT_Int64  *z )
  {
    FT_UInt32  lo1, hi1, lo2, hi2, lo, hi, i1, i2;


    lo1 = x & 0x0000FFFFU;  hi1 = x >> 16;
    lo2 = y & 0x0000FFFFU;  hi2 = y >> 16;

    lo = lo1 * lo2;
    i1 = lo1 * hi2;
    i2 = lo2 * hi1;
    hi = hi1 * hi2;

    /* Check carry overflow of i1 + i2 */
    i1 += i2;
    hi += (FT_UInt32)( i1 < i2 ) << 16;

    hi += i1 >> 16;
    i1  = i1 << 16;

    /* Check carry overflow of i1 + lo */
    lo += i1;
    hi += ( lo < i1 );

    z->lo = lo;
    z->hi = hi;
  }


  static FT_UInt32
  ft_div64by32( FT_UInt32  hi,
                FT_UInt32  lo,
                FT_UInt32  y )
  {
    FT_UInt32  r, q;
    FT_Int     i;


    q = 0;
    r = hi;

    if ( r >= y )
      return (FT_UInt32)0x7FFFFFFFL;

    i = 32;
    do
    {
      r <<= 1;
      q <<= 1;
      r  |= lo >> 31;

      if ( r >= (FT_UInt32)y )
      {
        r -= y;
        q |= 1;
      }
      lo <<= 1;
    } while ( --i );

    return q;
  }


  static void
  FT_Add64( FT_Int64*  x,
            FT_Int64*  y,
            FT_Int64  *z )
  {
    register FT_UInt32  lo, hi, max;


    max = x->lo > y->lo ? x->lo : y->lo;
    lo  = x->lo + y->lo;
    hi  = x->hi + y->hi + ( lo < max );

    z->lo = lo;
    z->hi = hi;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )
  FT_MulDiv( FT_Long  a,
             FT_Long  b,
             FT_Long  c )
  {
    long  s;


    if ( a == 0 || b == c )
      return a;

    s  = a; a = FT_ABS( a );
    s ^= b; b = FT_ABS( b );
    s ^= c; c = FT_ABS( c );

    if ( a <= 46340L && b <= 46340L && c <= 176095L && c > 0 )
      a = ( a * b + ( c >> 1 ) ) / c;

    else if ( c > 0 )
    {
      FT_Int64  temp, temp2;


      ft_multo64( a, b, &temp );

      temp2.hi = 0;
      temp2.lo = (FT_UInt32)(c >> 1);
      FT_Add64( &temp, &temp2, &temp );
      a = ft_div64by32( temp.hi, temp.lo, c );
    }
    else
      a = 0x7FFFFFFFL;

    return ( s < 0 ? -a : a );
  }


#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER

  FT_BASE_DEF( FT_Long )
  FT_MulDiv_No_Round( FT_Long  a,
                      FT_Long  b,
                      FT_Long  c )
  {
    long  s;


    if ( a == 0 || b == c )
      return a;

    s  = a; a = FT_ABS( a );
    s ^= b; b = FT_ABS( b );
    s ^= c; c = FT_ABS( c );

    if ( a <= 46340L && b <= 46340L && c > 0 )
      a = a * b / c;

    else if ( c > 0 )
    {
      FT_Int64  temp;


      ft_multo64( a, b, &temp );
      a = ft_div64by32( temp.hi, temp.lo, c );
    }
    else
      a = 0x7FFFFFFFL;

    return ( s < 0 ? -a : a );
  }

#endif /* TT_CONFIG_OPTION_BYTECODE_INTERPRETER */


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )
  FT_MulFix( FT_Long  a,
             FT_Long  b )
  {
#if 1
    FT_Long   sa, sb;
    FT_ULong  ua, ub;


    if ( a == 0 || b == 0x10000L )
      return a;

    sa = ( a >> ( sizeof ( a ) * 8 - 1 ) );
     a = ( a ^ sa ) - sa;
    sb = ( b >> ( sizeof ( b ) * 8 - 1 ) );
     b = ( b ^ sb ) - sb;

    ua = (FT_ULong)a;
    ub = (FT_ULong)b;

    if ( ua <= 2048 && ub <= 1048576L )
    {
      ua = ( ua * ub + 0x8000 ) >> 16;
    }
    else
    {
      FT_ULong  al = ua & 0xFFFF;


      ua = ( ua >> 16 ) * ub +  al * ( ub >> 16 ) +
           ( ( al * ( ub & 0xFFFF ) + 0x8000 ) >> 16 );
    }

    sa ^= sb,
    ua  = (FT_ULong)(( ua ^ sa ) - sa);

    return (FT_Long)ua;

#else /* 0 */

    FT_Long   s;
    FT_ULong  ua, ub;


    if ( a == 0 || b == 0x10000L )
      return a;

    s  = a; a = FT_ABS(a);
    s ^= b; b = FT_ABS(b);

    ua = (FT_ULong)a;
    ub = (FT_ULong)b;

    if ( ua <= 2048 && ub <= 1048576L )
    {
      ua = ( ua * ub + 0x8000L ) >> 16;
    }
    else
    {
      FT_ULong  al = ua & 0xFFFFL;


      ua = ( ua >> 16 ) * ub +  al * ( ub >> 16 ) +
           ( ( al * ( ub & 0xFFFFL ) + 0x8000L ) >> 16 );
    }

    return ( s < 0 ? -(FT_Long)ua : (FT_Long)ua );

#endif /* 0 */

  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Long )
  FT_DivFix( FT_Long  a,
             FT_Long  b )
  {
    FT_Int32   s;
    FT_UInt32  q;


    s  = a; a = FT_ABS(a);
    s ^= b; b = FT_ABS(b);

    if ( b == 0 )
    {
      /* check for division by 0 */
      q = 0x7FFFFFFFL;
    }
    else if ( ( a >> 16 ) == 0 )
    {
      /* compute result directly */
      q = (FT_UInt32)( (a << 16) + (b >> 1) ) / (FT_UInt32)b;
    }
    else
    {
      /* we need more bits; we have to do it by hand */
      FT_Int64  temp, temp2;

      temp.hi  = (FT_Int32) (a >> 16);
      temp.lo  = (FT_UInt32)(a << 16);
      temp2.hi = 0;
      temp2.lo = (FT_UInt32)( b >> 1 );
      FT_Add64( &temp, &temp2, &temp );
      q = ft_div64by32( temp.hi, temp.lo, b );
    }

    return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
  }


#if 0

  /* documentation is in ftcalc.h */

  FT_EXPORT_DEF( void )
  FT_MulTo64( FT_Int32   x,
              FT_Int32   y,
              FT_Int64  *z )
  {
    FT_Int32  s;


    s  = x; x = FT_ABS( x );
    s ^= y; y = FT_ABS( y );

    ft_multo64( x, y, z );

    if ( s < 0 )
    {
      z->lo = (FT_UInt32)-(FT_Int32)z->lo;
      z->hi = ~z->hi + !( z->lo );
    }
  }


  /* apparently, the second version of this code is not compiled correctly */
  /* on Mac machines with the MPW C compiler..  tsk, tsk, tsk...         */

#if 1

  FT_EXPORT_DEF( FT_Int32 )
  FT_Div64by32( FT_Int64*  x,
                FT_Int32   y )
  {
    FT_Int32   s;
    FT_UInt32  q, r, i, lo;


    s  = x->hi;
    if ( s < 0 )
    {
      x->lo = (FT_UInt32)-(FT_Int32)x->lo;
      x->hi = ~x->hi + !x->lo;
    }
    s ^= y;  y = FT_ABS( y );

    /* Shortcut */
    if ( x->hi == 0 )
    {
      if ( y > 0 )
        q = x->lo / y;
      else
        q = 0x7FFFFFFFL;

      return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
    }

    r  = x->hi;
    lo = x->lo;

    if ( r >= (FT_UInt32)y ) /* we know y is to be treated as unsigned here */
      return ( s < 0 ? 0x80000001UL : 0x7FFFFFFFUL );
                             /* Return Max/Min Int32 if division overflow. */
                             /* This includes division by zero! */
    q = 0;
    for ( i = 0; i < 32; i++ )
    {
      r <<= 1;
      q <<= 1;
      r  |= lo >> 31;

      if ( r >= (FT_UInt32)y )
      {
        r -= y;
        q |= 1;
      }
      lo <<= 1;
    }

    return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
  }

#else /* 0 */

  FT_EXPORT_DEF( FT_Int32 )
  FT_Div64by32( FT_Int64*  x,
                FT_Int32   y )
  {
    FT_Int32   s;
    FT_UInt32  q;


    s  = x->hi;
    if ( s < 0 )
    {
      x->lo = (FT_UInt32)-(FT_Int32)x->lo;
      x->hi = ~x->hi + !x->lo;
    }
    s ^= y;  y = FT_ABS( y );

    /* Shortcut */
    if ( x->hi == 0 )
    {
      if ( y > 0 )
        q = ( x->lo + ( y >> 1 ) ) / y;
      else
        q = 0x7FFFFFFFL;

      return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
    }

    q = ft_div64by32( x->hi, x->lo, y );

    return ( s < 0 ? -(FT_Int32)q : (FT_Int32)q );
  }

#endif /* 0 */

#endif /* 0 */


#endif /* FT_LONG64 */


  /* documentation is in ftcalc.h */

  FT_BASE_DEF( FT_Int32 )
  FT_SqrtFixed( FT_Int32  x )
  {
    FT_UInt32  root, rem_hi, rem_lo, test_div;
    FT_Int     count;


    root = 0;

    if ( x > 0 )
    {
      rem_hi = 0;
      rem_lo = x;
      count  = 24;
      do
      {
        rem_hi   = ( rem_hi << 2 ) | ( rem_lo >> 30 );
        rem_lo <<= 2;
        root   <<= 1;
        test_div = ( root << 1 ) + 1;

        if ( rem_hi >= test_div )
        {
          rem_hi -= test_div;
          root   += 1;
        }
      } while ( --count );
    }

    return (FT_Int32)root;
  }


/* END */
