/***************************************************************************/
/*                                                                         */
/*  ttkern.c                                                               */
/*                                                                         */
/*    Load the basic TrueType kerning table.  This doesn't handle          */
/*    kerning data within the GPOS table at the moment.                    */
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


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_TRUETYPE_TAGS_H
#include "ttkern.h"
#include "ttload.h"

#include "sferrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ttkern


#undef  TT_KERN_INDEX
#define TT_KERN_INDEX( g1, g2 )  ( ( (FT_ULong)(g1) << 16 ) | (g2) )


#ifdef FT_OPTIMIZE_MEMORY

  FT_LOCAL_DEF( FT_Error )
  tt_face_load_kern( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_ULong   table_size;
    FT_Byte*   p;
    FT_Byte*   p_limit;
    FT_UInt    nn, num_tables;
    FT_UInt32  avail = 0, ordered = 0;


    /* the kern table is optional; exit silently if it is missing */
    error = face->goto_table( face, TTAG_kern, stream, &table_size );
    if ( error )
      goto Exit;

    if ( table_size < 4 )  /* the case of a malformed table */
    {
      FT_ERROR(( "kerning table is too small - ignored\n" ));
      error = SFNT_Err_Table_Missing;
      goto Exit;
    }

    if ( FT_FRAME_EXTRACT( table_size, face->kern_table ) )
    {
      FT_ERROR(( "could not extract kerning table\n" ));
      goto Exit;
    }

    face->kern_table_size = table_size;

    p       = face->kern_table;
    p_limit = p + table_size;

    p         += 2; /* skip version */
    num_tables = FT_NEXT_USHORT( p );

    if ( num_tables > 32 ) /* we only support up to 32 sub-tables */
      num_tables = 32;

    for ( nn = 0; nn < num_tables; nn++ )
    {
      FT_UInt    num_pairs, version, length, coverage;
      FT_Byte*   p_next;
      FT_UInt32  mask = 1UL << nn;


      if ( p + 6 > p_limit )
        break;

      p_next = p;

      version  = FT_NEXT_USHORT( p );
      length   = FT_NEXT_USHORT( p );
      coverage = FT_NEXT_USHORT( p );

      if ( length <= 6 )
        break;

      p_next += length;

      /* only use horizontal kerning tables */
      if ( ( coverage & ~8 ) != 0x0001 ||
           p + 8 > p_limit             )
        goto NextTable;

      num_pairs = FT_NEXT_USHORT( p );
      p        += 6;

      if ( p + 6 * num_pairs > p_limit )
        goto NextTable;

      avail |= mask;

      /*
       *  Now check whether the pairs in this table are ordered.
       *  We then can use binary search.
       */
      if ( num_pairs > 0 )
      {
        FT_UInt  count;
        FT_UInt  old_pair;


        old_pair = FT_NEXT_ULONG( p );
        p       += 2;

        for ( count = num_pairs - 1; count > 0; count-- )
        {
          FT_UInt32  cur_pair;


          cur_pair = FT_NEXT_ULONG( p );
          if ( cur_pair <= old_pair )
            break;

          p += 2;
          old_pair = cur_pair;
        }

        if ( count == 0 )
          ordered |= mask;
      }

    NextTable:
      p = p_next;
    }

    face->num_kern_tables = nn;
    face->kern_avail_bits = avail;
    face->kern_order_bits = ordered;

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  tt_face_done_kern( TT_Face  face )
  {
    FT_Stream  stream = face->root.stream;


    FT_FRAME_RELEASE( face->kern_table );
    face->kern_table_size = 0;
    face->num_kern_tables = 0;
    face->kern_avail_bits = 0;
    face->kern_order_bits = 0;
  }


  FT_LOCAL_DEF( FT_Int )
  tt_face_get_kerning( TT_Face  face,
                       FT_UInt  left_glyph,
                       FT_UInt  right_glyph )
  {
    FT_Int    result = 0;
    FT_UInt   count, mask = 1;
    FT_Byte*  p       = face->kern_table;


    p   += 4;
    mask = 0x0001;

    for ( count = face->num_kern_tables; count > 0; count--, mask <<= 1 )
    {
      FT_Byte* base     = p;
      FT_Byte* next     = base;
      FT_UInt  version  = FT_NEXT_USHORT( p );
      FT_UInt  length   = FT_NEXT_USHORT( p );
      FT_UInt  coverage = FT_NEXT_USHORT( p );
      FT_Int   value    = 0;

      FT_UNUSED( version );


      next = base + length;

      if ( ( face->kern_avail_bits & mask ) == 0 )
        goto NextTable;

      if ( p + 8 > next )
        goto NextTable;

      switch ( coverage >> 8 )
      {
      case 0:
        {
          FT_UInt   num_pairs = FT_NEXT_USHORT( p );
          FT_ULong  key0      = TT_KERN_INDEX( left_glyph, right_glyph );


          p += 6;

          if ( face->kern_order_bits & mask )   /* binary search */
          {
            FT_UInt   min = 0;
            FT_UInt   max = num_pairs;


            while ( min < max )
            {
              FT_UInt   mid = ( min + max ) >> 1;
              FT_Byte*  q   = p + 6 * mid;
              FT_ULong  key;


              key = FT_NEXT_ULONG( q );

              if ( key == key0 )
              {
                value = FT_PEEK_SHORT( q );
                goto Found;
              }
              if ( key < key0 )
                min = mid + 1;
              else
                max = mid;
            }
          }
          else /* linear search */
          {
            FT_UInt  count2;


            for ( count2 = num_pairs; count2 > 0; count2-- )
            {
              FT_ULong  key = FT_NEXT_ULONG( p );


              if ( key == key0 )
              {
                value = FT_PEEK_SHORT( p );
                goto Found;
              }
              p += 2;
            }
          }
        }
        break;

       /*
        *  We don't support format 2 because we haven't seen a single font
        *  using it in real life...
        */

      default:
        ;
      }

      goto NextTable;

    Found:
      if ( coverage & 8 ) /* overide or add */
        result = value;
      else
        result += value;

    NextTable:
      p = next;
    }

    return result;
  }

#else /* !OPTIMIZE_MEMORY */

  FT_CALLBACK_DEF( int )
  tt_kern_pair_compare( const void*  a,
                        const void*  b );


  FT_LOCAL_DEF( FT_Error )
  tt_face_load_kern( TT_Face    face,
                     FT_Stream  stream )
  {
    FT_Error   error;
    FT_Memory  memory = stream->memory;

    FT_UInt    n, num_tables;


    /* the kern table is optional; exit silently if it is missing */
    error = face->goto_table( face, TTAG_kern, stream, 0 );
    if ( error )
      return SFNT_Err_Ok;

    if ( FT_FRAME_ENTER( 4L ) )
      goto Exit;

    (void)FT_GET_USHORT();         /* version */
    num_tables = FT_GET_USHORT();

    FT_FRAME_EXIT();

    for ( n = 0; n < num_tables; n++ )
    {
      FT_UInt  coverage;
      FT_UInt  length;


      if ( FT_FRAME_ENTER( 6L ) )
        goto Exit;

      (void)FT_GET_USHORT();           /* version                 */
      length   = FT_GET_USHORT() - 6;  /* substract header length */
      coverage = FT_GET_USHORT();

      FT_FRAME_EXIT();

      if ( coverage == 0x0001 )
      {
        FT_UInt        num_pairs;
        TT_Kern0_Pair  pair;
        TT_Kern0_Pair  limit;


        /* found a horizontal format 0 kerning table! */
        if ( FT_FRAME_ENTER( 8L ) )
          goto Exit;

        num_pairs = FT_GET_USHORT();

        /* skip the rest */

        FT_FRAME_EXIT();

        /* allocate array of kerning pairs */
        if ( FT_QNEW_ARRAY( face->kern_pairs, num_pairs ) ||
             FT_FRAME_ENTER( 6L * num_pairs )             )
          goto Exit;

        pair  = face->kern_pairs;
        limit = pair + num_pairs;
        for ( ; pair < limit; pair++ )
        {
          pair->left  = FT_GET_USHORT();
          pair->right = FT_GET_USHORT();
          pair->value = FT_GET_USHORT();
        }

        FT_FRAME_EXIT();

        face->num_kern_pairs   = num_pairs;
        face->kern_table_index = n;

        /* ensure that the kerning pair table is sorted (yes, some */
        /* fonts have unsorted tables!)                            */

        if ( num_pairs > 0 )
        {
          TT_Kern0_Pair  pair0 = face->kern_pairs;
          FT_ULong       prev  = TT_KERN_INDEX( pair0->left, pair0->right );


          for ( pair0++; pair0 < limit; pair0++ )
          {
            FT_ULong  next = TT_KERN_INDEX( pair0->left, pair0->right );


            if ( next < prev )
              goto SortIt;

            prev = next;
          }
          goto Exit;

        SortIt:
          ft_qsort( (void*)face->kern_pairs, (int)num_pairs,
                    sizeof ( TT_Kern0_PairRec ), tt_kern_pair_compare );
        }

        goto Exit;
      }

      if ( FT_STREAM_SKIP( length ) )
        goto Exit;
    }

    /* no kern table found -- doesn't matter */
    face->kern_table_index = -1;
    face->num_kern_pairs   = 0;
    face->kern_pairs       = NULL;

  Exit:
    return error;
  }


  FT_CALLBACK_DEF( int )
  tt_kern_pair_compare( const void*  a,
                        const void*  b )
  {
    TT_Kern0_Pair  pair1 = (TT_Kern0_Pair)a;
    TT_Kern0_Pair  pair2 = (TT_Kern0_Pair)b;

    FT_ULong  index1 = TT_KERN_INDEX( pair1->left, pair1->right );
    FT_ULong  index2 = TT_KERN_INDEX( pair2->left, pair2->right );

    return index1 < index2 ? -1
                           : ( index1 > index2 ? 1
                                               : 0 );
  }


  FT_LOCAL_DEF( void )
  tt_face_done_kern( TT_Face  face )
  {
    FT_Memory  memory = face->root.stream->memory;


    FT_FREE( face->kern_pairs );
    face->num_kern_pairs = 0;
  }


  FT_LOCAL_DEF( FT_Int )
  tt_face_get_kerning( TT_Face  face,
                       FT_UInt  left_glyph,
                       FT_UInt  right_glyph )
  {
    FT_Int         result = 0;
    TT_Kern0_Pair  pair;


    if ( face && face->kern_pairs )
    {
      /* there are some kerning pairs in this font file! */
      FT_ULong  search_tag = TT_KERN_INDEX( left_glyph, right_glyph );
      FT_Long   left, right;


      left  = 0;
      right = face->num_kern_pairs - 1;

      while ( left <= right )
      {
        FT_Long   middle = left + ( ( right - left ) >> 1 );
        FT_ULong  cur_pair;


        pair     = face->kern_pairs + middle;
        cur_pair = TT_KERN_INDEX( pair->left, pair->right );

        if ( cur_pair == search_tag )
          goto Found;

        if ( cur_pair < search_tag )
          left = middle + 1;
        else
          right = middle - 1;
      }
    }

  Exit:
    return result;

  Found:
    result = pair->value;
    goto Exit;
  }

#endif /* !OPTIMIZE_MEMORY */


#undef TT_KERN_INDEX

/* END */
