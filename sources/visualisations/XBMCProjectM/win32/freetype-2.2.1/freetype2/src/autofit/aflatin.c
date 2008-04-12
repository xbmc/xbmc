/***************************************************************************/
/*                                                                         */
/*  aflatin.c                                                              */
/*                                                                         */
/*    Auto-fitter hinting routines for latin script (body).                */
/*                                                                         */
/*  Copyright 2003, 2004, 2005, 2006 by                                    */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "aflatin.h"
#include "aferrors.h"


#ifdef AF_USE_WARPER
#include "afwarp.h"
#endif


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****            L A T I N   G L O B A L   M E T R I C S            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( void )
  af_latin_metrics_init_widths( AF_LatinMetrics  metrics,
                                FT_Face          face,
                                FT_ULong         charcode )
  {
    /* scan the array of segments in each direction */
    AF_GlyphHintsRec  hints[1];


    af_glyph_hints_init( hints, face->memory );

    metrics->axis[AF_DIMENSION_HORZ].width_count = 0;
    metrics->axis[AF_DIMENSION_VERT].width_count = 0;

    {
      FT_Error             error;
      FT_UInt              glyph_index;
      int                  dim;
      AF_LatinMetricsRec   dummy[1];
      AF_Scaler            scaler = &dummy->root.scaler;


      glyph_index = FT_Get_Char_Index( face, charcode );
      if ( glyph_index == 0 )
        goto Exit;

      error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_SCALE );
      if ( error || face->glyph->outline.n_points <= 0 )
        goto Exit;

      FT_ZERO( dummy );

      dummy->units_per_em = metrics->units_per_em;
      scaler->x_scale     = scaler->y_scale = 0x10000L;
      scaler->x_delta     = scaler->y_delta = 0;
      scaler->face        = face;
      scaler->render_mode = FT_RENDER_MODE_NORMAL;
      scaler->flags       = 0;

      af_glyph_hints_rescale( hints, (AF_ScriptMetrics)dummy );

      error = af_glyph_hints_reload( hints, &face->glyph->outline );
      if ( error )
        goto Exit;

      for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
      {
        AF_LatinAxis  axis    = &metrics->axis[dim];
        AF_AxisHints  axhints = &hints->axis[dim];
        AF_Segment    seg, limit, link;
        FT_UInt       num_widths = 0;


        error = af_latin_hints_compute_segments( hints,
                                                 (AF_Dimension)dim );
        if ( error )
          goto Exit;

        af_latin_hints_link_segments( hints,
                                      (AF_Dimension)dim );

        seg   = axhints->segments;
        limit = seg + axhints->num_segments;

        for ( ; seg < limit; seg++ )
        {
          link = seg->link;

          /* we only consider stem segments there! */
          if ( link && link->link == seg && link > seg )
          {
            FT_Pos  dist;


            dist = seg->pos - link->pos;
            if ( dist < 0 )
              dist = -dist;

            if ( num_widths < AF_LATIN_MAX_WIDTHS )
              axis->widths[ num_widths++ ].org = dist;
          }
        }

        af_sort_widths( num_widths, axis->widths );
        axis->width_count = num_widths;
      }

  Exit:
      for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
      {
        AF_LatinAxis  axis = &metrics->axis[dim];
        FT_Pos        stdw;


        stdw = ( axis->width_count > 0 )
                 ? axis->widths[0].org
                 : AF_LATIN_CONSTANT( metrics, 50 );

        /* let's try 20% of the smallest width */
        axis->edge_distance_threshold = stdw / 5;
      }
    }

    af_glyph_hints_done( hints );
  }



#define AF_LATIN_MAX_TEST_CHARACTERS  12


  static const char* const  af_latin_blue_chars[AF_LATIN_MAX_BLUES] =
  {
    "THEZOCQS",
    "HEZLOCUS",
    "fijkdbh",
    "xzroesc",
    "xzroesc",
    "pqgjy"
  };


  static void
  af_latin_metrics_init_blues( AF_LatinMetrics  metrics,
                               FT_Face          face )
  {
    FT_Pos        flats [AF_LATIN_MAX_TEST_CHARACTERS];
    FT_Pos        rounds[AF_LATIN_MAX_TEST_CHARACTERS];
    FT_Int        num_flats;
    FT_Int        num_rounds;
    FT_Int        bb;
    AF_LatinBlue  blue;
    FT_Error      error;
    AF_LatinAxis  axis  = &metrics->axis[AF_DIMENSION_VERT];
    FT_GlyphSlot  glyph = face->glyph;


    /* we compute the blues simply by loading each character from the    */
    /* 'af_latin_blue_chars[blues]' string, then compute its top-most or */
    /* bottom-most points (depending on `AF_IS_TOP_BLUE')                */

    AF_LOG(( "blue zones computation\n" ));
    AF_LOG(( "------------------------------------------------\n" ));

    for ( bb = 0; bb < AF_LATIN_BLUE_MAX; bb++ )
    {
      const char*  p     = af_latin_blue_chars[bb];
      const char*  limit = p + AF_LATIN_MAX_TEST_CHARACTERS;
      FT_Pos*      blue_ref;
      FT_Pos*      blue_shoot;


      AF_LOG(( "blue %3d: ", bb ));

      num_flats  = 0;
      num_rounds = 0;

      for ( ; p < limit && *p; p++ )
      {
        FT_UInt     glyph_index;
        FT_Vector*  extremum;
        FT_Vector*  points;
        FT_Vector*  point_limit;
        FT_Vector*  point;
        FT_Bool     round;


        AF_LOG(( "'%c'", *p ));

        /* load the character in the face -- skip unknown or empty ones */
        glyph_index = FT_Get_Char_Index( face, (FT_UInt)*p );
        if ( glyph_index == 0 )
          continue;

        error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_SCALE );
        if ( error || glyph->outline.n_points <= 0 )
          continue;

        /* now compute min or max point indices and coordinates */
        points      = glyph->outline.points;
        point_limit = points + glyph->outline.n_points;
        point       = points;
        extremum    = point;
        point++;

        if ( AF_LATIN_IS_TOP_BLUE( bb ) )
        {
          for ( ; point < point_limit; point++ )
            if ( point->y > extremum->y )
              extremum = point;
        }
        else
        {
          for ( ; point < point_limit; point++ )
            if ( point->y < extremum->y )
              extremum = point;
        }

        AF_LOG(( "%5d", (int)extremum->y ));

        /* now, check whether the point belongs to a straight or round  */
        /* segment; we first need to find in which contour the extremum */
        /* lies, then see its previous and next points                  */
        {
          FT_Int  idx = (FT_Int)( extremum - points );
          FT_Int  n;
          FT_Int  first, last, prev, next, end;
          FT_Pos  dist;


          last  = -1;
          first = 0;

          for ( n = 0; n < glyph->outline.n_contours; n++ )
          {
            end = glyph->outline.contours[n];
            if ( end >= idx )
            {
              last = end;
              break;
            }
            first = end + 1;
          }

          /* XXX: should never happen! */
          if ( last < 0 )
            continue;

          /* now look for the previous and next points that are not on the */
          /* same Y coordinate.  Threshold the `closeness'...              */

          prev = idx;
          next = prev;

          do
          {
            if ( prev > first )
              prev--;
            else
              prev = last;

            dist = points[prev].y - extremum->y;
            if ( dist < -5 || dist > 5 )
              break;

          } while ( prev != idx );

          do
          {
            if ( next < last )
              next++;
            else
              next = first;

            dist = points[next].y - extremum->y;
            if ( dist < -5 || dist > 5 )
              break;

          } while ( next != idx );

          /* now, set the `round' flag depending on the segment's kind */
          round = FT_BOOL(
            FT_CURVE_TAG( glyph->outline.tags[prev] ) != FT_CURVE_TAG_ON ||
            FT_CURVE_TAG( glyph->outline.tags[next] ) != FT_CURVE_TAG_ON );

          AF_LOG(( "%c ", round ? 'r' : 'f' ));
        }

        if ( round )
          rounds[num_rounds++] = extremum->y;
        else
          flats[num_flats++] = extremum->y;
      }

      AF_LOG(( "\n" ));

      if ( num_flats == 0 && num_rounds == 0 )
      {
        /*
         *  we couldn't find a single glyph to compute this blue zone,
         *  we will simply ignore it then
         */
        AF_LOG(( "empty!\n" ));
        continue;
      }

      /* we have computed the contents of the `rounds' and `flats' tables, */
      /* now determine the reference and overshoot position of the blue -- */
      /* we simply take the median value after a simple sort               */
      af_sort_pos( num_rounds, rounds );
      af_sort_pos( num_flats,  flats );

      blue       = & axis->blues[axis->blue_count];
      blue_ref   = & blue->ref.org;
      blue_shoot = & blue->shoot.org;

      axis->blue_count++;

      if ( num_flats == 0 )
      {
        *blue_ref   =
        *blue_shoot = rounds[num_rounds / 2];
      }
      else if ( num_rounds == 0 )
      {
        *blue_ref   =
        *blue_shoot = flats[num_flats / 2];
      }
      else
      {
        *blue_ref   = flats[num_flats / 2];
        *blue_shoot = rounds[num_rounds / 2];
      }

      /* there are sometimes problems: if the overshoot position of top     */
      /* zones is under its reference position, or the opposite for bottom  */
      /* zones.  We must thus check everything there and correct the errors */
      if ( *blue_shoot != *blue_ref )
      {
        FT_Pos   ref      = *blue_ref;
        FT_Pos   shoot    = *blue_shoot;
        FT_Bool  over_ref = FT_BOOL( shoot > ref );


        if ( AF_LATIN_IS_TOP_BLUE( bb ) ^ over_ref )
          *blue_shoot = *blue_ref = ( shoot + ref ) / 2;
      }

      blue->flags = 0;
      if ( AF_LATIN_IS_TOP_BLUE( bb ) )
        blue->flags |= AF_LATIN_BLUE_TOP;

      /*
       * The following flags is used later to adjust the y and x scales
       * in order to optimize the pixel grid alignment of the top of small
       * letters.
       */
      if ( bb == AF_LATIN_BLUE_SMALL_TOP )
        blue->flags |= AF_LATIN_BLUE_ADJUSTMENT;

      AF_LOG(( "-- ref = %ld, shoot = %ld\n", *blue_ref, *blue_shoot ));
    }

    return;
  }


  FT_LOCAL_DEF( FT_Error )
  af_latin_metrics_init( AF_LatinMetrics  metrics,
                         FT_Face          face )
  {
    FT_Error    error = AF_Err_Ok;
    FT_CharMap  oldmap = face->charmap;
    FT_UInt     ee;

    static const FT_Encoding  latin_encodings[] =
    {
      FT_ENCODING_UNICODE,
      FT_ENCODING_APPLE_ROMAN,
      FT_ENCODING_ADOBE_STANDARD,
      FT_ENCODING_ADOBE_LATIN_1,
      FT_ENCODING_NONE  /* end of list */
    };


    metrics->units_per_em = face->units_per_EM;

    /* do we have a latin charmap in there? */
    for ( ee = 0; latin_encodings[ee] != FT_ENCODING_NONE; ee++ )
    {
      error = FT_Select_Charmap( face, latin_encodings[ee] );
      if ( !error )
        break;
    }

    if ( !error )
    {
      /* For now, compute the standard width and height from the `o'. */
      af_latin_metrics_init_widths( metrics, face, 'o' );
      af_latin_metrics_init_blues( metrics, face );
    }

    FT_Set_Charmap( face, oldmap );
    return AF_Err_Ok;
  }


  static void
  af_latin_metrics_scale_dim( AF_LatinMetrics  metrics,
                              AF_Scaler        scaler,
                              AF_Dimension     dim )
  {
    FT_Fixed      scale;
    FT_Pos        delta;
    AF_LatinAxis  axis;
    FT_UInt       nn;


    if ( dim == AF_DIMENSION_HORZ )
    {
      scale = scaler->x_scale;
      delta = scaler->x_delta;
    }
    else
    {
      scale = scaler->y_scale;
      delta = scaler->y_delta;
    }

    axis = &metrics->axis[dim];

    if ( axis->org_scale == scale && axis->org_delta == delta )
      return;

    axis->org_scale = scale;
    axis->org_delta = delta;

    /*
     * correct X and Y scale to optimize the alignment of the top of small
     * letters to the pixel grid
     */
    {
      AF_LatinAxis  Axis = &metrics->axis[AF_DIMENSION_VERT];
      AF_LatinBlue  blue = NULL;


      for ( nn = 0; nn < Axis->blue_count; nn++ )
      {
        if ( Axis->blues[nn].flags & AF_LATIN_BLUE_ADJUSTMENT )
        {
          blue = &Axis->blues[nn];
          break;
        }
      }

      if ( blue )
      {
        FT_Pos  scaled = FT_MulFix( blue->shoot.org, scaler->y_scale );
        FT_Pos  fitted = FT_PIX_ROUND( scaled );


        if ( scaled != fitted )
        {
          if ( dim == AF_DIMENSION_HORZ )
          {
            if ( fitted < scaled )
              scale -= scale/50;  /* x_scale = x_scale*0.98 */
          }
          else
          {
            scale = FT_MulDiv( scale, fitted, scaled );
          }
        }
      }
    }

    axis->scale = scale;
    axis->delta = delta;

    if ( dim == AF_DIMENSION_HORZ )
    {
      metrics->root.scaler.x_scale = scale;
      metrics->root.scaler.x_delta = delta;
    }
    else
    {
      metrics->root.scaler.y_scale = scale;
      metrics->root.scaler.y_delta = delta;
    }

    /* scale the standard widths */
    for ( nn = 0; nn < axis->width_count; nn++ )
    {
      AF_Width  width = axis->widths + nn;


      width->cur = FT_MulFix( width->org, scale );
      width->fit = width->cur;
    }

    if ( dim == AF_DIMENSION_VERT )
    {
      /* scale the blue zones */
      for ( nn = 0; nn < axis->blue_count; nn++ )
      {
        AF_LatinBlue  blue = &axis->blues[nn];
        FT_Pos        dist;


        blue->ref.cur   = FT_MulFix( blue->ref.org, scale ) + delta;
        blue->ref.fit   = blue->ref.cur;
        blue->shoot.cur = FT_MulFix( blue->shoot.org, scale ) + delta;
        blue->shoot.fit = blue->shoot.cur;
        blue->flags    &= ~AF_LATIN_BLUE_ACTIVE;

        /* a blue zone is only active if it is less than 3/4 pixels tall */
        dist = FT_MulFix( blue->ref.org - blue->shoot.org, scale );
        if ( dist <= 48 && dist >= -48 )
        {
          FT_Pos  delta1, delta2;


          delta1 = blue->shoot.org - blue->ref.org;
          delta2 = delta1;
          if ( delta1 < 0 )
            delta2 = -delta2;

          delta2 = FT_MulFix( delta2, scale );

          if ( delta2 < 32 )
            delta2 = 0;
          else if ( delta2 < 64 )
            delta2 = 32 + ( ( ( delta2 - 32 ) + 16 ) & ~31 );
          else
            delta2 = FT_PIX_ROUND( delta2 );

          if ( delta1 < 0 )
            delta2 = -delta2;

          blue->ref.fit   = FT_PIX_ROUND( blue->ref.cur );
          blue->shoot.fit = blue->ref.fit + delta2;

          blue->flags |= AF_LATIN_BLUE_ACTIVE;
        }
      }
    }
  }


  FT_LOCAL_DEF( void )
  af_latin_metrics_scale( AF_LatinMetrics  metrics,
                          AF_Scaler        scaler )
  {
    metrics->root.scaler.render_mode = scaler->render_mode;
    metrics->root.scaler.face        = scaler->face;

    af_latin_metrics_scale_dim( metrics, scaler, AF_DIMENSION_HORZ );
    af_latin_metrics_scale_dim( metrics, scaler, AF_DIMENSION_VERT );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****           L A T I N   G L Y P H   A N A L Y S I S             *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_LOCAL_DEF( FT_Error )
  af_latin_hints_compute_segments( AF_GlyphHints  hints,
                                   AF_Dimension   dim )
  {
    AF_AxisHints  axis          = &hints->axis[dim];
    FT_Memory     memory        = hints->memory;
    FT_Error      error         = AF_Err_Ok;
    AF_Segment    segment       = NULL;
    AF_Point*     contour       = hints->contours;
    AF_Point*     contour_limit = contour + hints->num_contours;
    AF_Direction  major_dir, segment_dir;

#ifdef AF_HINT_METRICS
    AF_Point  min_point =  0;
    AF_Point  max_point =  0;
    FT_Pos    min_coord =  32000;
    FT_Pos    max_coord = -32000;
#endif

    major_dir   = (AF_Direction)FT_ABS( axis->major_dir );
    segment_dir = major_dir;

    axis->num_segments = 0;

    /* set up (u,v) in each point */
    if ( dim == AF_DIMENSION_HORZ )
    {
      AF_Point  point = hints->points;
      AF_Point  limit = point + hints->num_points;


      for ( ; point < limit; point++ )
      {
        point->u = point->fx;
        point->v = point->fy;
      }
    }
    else
    {
      AF_Point  point = hints->points;
      AF_Point  limit = point + hints->num_points;


      for ( ; point < limit; point++ )
      {
        point->u = point->fy;
        point->v = point->fx;
      }
    }

    /* do each contour separately */
    for ( ; contour < contour_limit; contour++ )
    {
      AF_Point  point   =  contour[0];
      AF_Point  last    =  point->prev;
      int       on_edge =  0;
      FT_Pos    min_pos =  32000;  /* minimum segment pos != min_coord */
      FT_Pos    max_pos = -32000;  /* maximum segment pos != max_coord */
      FT_Bool   passed;


#ifdef AF_HINT_METRICS
      if ( point->u < min_coord )
      {
        min_coord = point->u;
        min_point = point;
      }
      if ( point->u > max_coord )
      {
        max_coord = point->u;
        max_point = point;
      }
#endif

      if ( point == last )  /* skip singletons -- just in case */
        continue;

      if ( FT_ABS( last->out_dir )  == major_dir &&
           FT_ABS( point->out_dir ) == major_dir )
      {
        /* we are already on an edge, try to locate its start */
        last = point;

        for (;;)
        {
          point = point->prev;
          if ( FT_ABS( point->out_dir ) != major_dir )
          {
            point = point->next;
            break;
          }
          if ( point == last )
            break;
        }
      }

      last   = point;
      passed = 0;

      for (;;)
      {
        FT_Pos  u, v;


        if ( on_edge )
        {
          u = point->u;
          if ( u < min_pos )
            min_pos = u;
          if ( u > max_pos )
            max_pos = u;

          if ( point->out_dir != segment_dir || point == last )
          {
            /* we are just leaving an edge; record a new segment! */
            segment->last = point;
            segment->pos  = (FT_Short)( ( min_pos + max_pos ) >> 1 );

            /* a segment is round if either its first or last point */
            /* is a control point                                   */
            if ( ( segment->first->flags | point->flags ) &
                   AF_FLAG_CONTROL                        )
              segment->flags |= AF_EDGE_ROUND;

            /* compute segment size */
            min_pos = max_pos = point->v;

            v = segment->first->v;
            if ( v < min_pos )
              min_pos = v;
            if ( v > max_pos )
              max_pos = v;

            segment->min_coord = (FT_Short)min_pos;
            segment->max_coord = (FT_Short)max_pos;

            on_edge = 0;
            segment = NULL;
            /* fallthrough */
          }
        }

        /* now exit if we are at the start/end point */
        if ( point == last )
        {
          if ( passed )
            break;
          passed = 1;
        }

        if ( !on_edge && FT_ABS( point->out_dir ) == major_dir )
        {
          /* this is the start of a new segment! */
          segment_dir = (AF_Direction)point->out_dir;

          /* clear all segment fields */
          error = af_axis_hints_new_segment( axis, memory, &segment );
          if ( error )
            goto Exit;

          segment->dir      = (FT_Char)segment_dir;
          segment->flags    = AF_EDGE_NORMAL;
          min_pos = max_pos = point->u;
          segment->first    = point;
          segment->last     = point;
          segment->contour  = contour;
          segment->score    = 32000;
          segment->len      = 0;
          segment->link     = NULL;
          on_edge           = 1;

#ifdef AF_HINT_METRICS
          if ( point == max_point )
            max_point = 0;

          if ( point == min_point )
            min_point = 0;
#endif
        }

        point = point->next;
      }

    } /* contours */

#ifdef AF_HINT_METRICS
    /* we need to ensure that there are edges on the left-most and  */
    /* right-most points of the glyph in order to hint the metrics; */
    /* we do this by inserting fake segments when needed            */

    if ( dim == AF_DIMENSION_HORZ )
    {
      AF_Point  point       = hints->points;
      AF_Point  point_limit = point + hints->num_points;

      FT_Pos    min_pos =  32000;
      FT_Pos    max_pos = -32000;


      min_point = 0;
      max_point = 0;

      /* compute minimum and maximum points */
      for ( ; point < point_limit; point++ )
      {
        FT_Pos  x = point->fx;


        if ( x < min_pos )
        {
          min_pos   = x;
          min_point = point;
        }
        if ( x > max_pos )
        {
          max_pos   = x;
          max_point = point;
        }
      }

      /* insert minimum segment */
      if ( min_point )
      {
        /* clear all segment fields */
        error = af_axis_hints_new_segment( axis, memory, &segment );
        if ( error )
          goto Exit;

        segment->dir   = segment_dir;
        segment->flags = AF_EDGE_NORMAL;
        segment->first = min_point;
        segment->last  = min_point;
        segment->pos   = min_pos;
        segment->score = 32000;
        segment->len   = 0;
        segment->link  = NULL;

        segment = NULL;
      }

      /* insert maximum segment */
      if ( max_point )
      {
        /* clear all segment fields */
        error = af_axis_hints_new_segment( axis, memory, &segment );
        if ( error )
          goto Exit;

        segment->dir   = segment_dir;
        segment->flags = AF_EDGE_NORMAL;
        segment->first = max_point;
        segment->last  = max_point;
        segment->pos   = max_pos;
        segment->score = 32000;
        segment->len   = 0;
        segment->link  = NULL;

        segment = NULL;
      }
    }
#endif /* AF_HINT_METRICS */

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  af_latin_hints_link_segments( AF_GlyphHints  hints,
                                AF_Dimension   dim )
  {
    AF_AxisHints  axis          = &hints->axis[dim];
    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    AF_Direction  major_dir     = axis->major_dir;
    FT_Pos        len_threshold, len_score;
    AF_Segment    seg1, seg2;


    len_threshold = AF_LATIN_CONSTANT( hints->metrics, 8 );
    if ( len_threshold == 0 )
      len_threshold = 1;

    len_score = AF_LATIN_CONSTANT( hints->metrics, 3000 );

    /* now compare each segment to the others */
    for ( seg1 = segments; seg1 < segment_limit; seg1++ )
    {
      /* the fake segments are introduced to hint the metrics -- */
      /* we must never link them to anything                     */
      if ( seg1->first == seg1->last || seg1->dir != major_dir )
        continue;

      for ( seg2 = segments; seg2 < segment_limit; seg2++ )
        if ( seg2 != seg1 && seg1->dir + seg2->dir == 0 )
        {
          FT_Pos  pos1 = seg1->pos;
          FT_Pos  pos2 = seg2->pos;
          FT_Pos  dist = pos2 - pos1;


          if ( dist < 0 )
            continue;

          {
            FT_Pos  min = seg1->min_coord;
            FT_Pos  max = seg1->max_coord;
            FT_Pos  len, score;


            if ( min < seg2->min_coord )
              min = seg2->min_coord;

            if ( max > seg2->max_coord )
              max = seg2->max_coord;

            len = max - min;
            if ( len >= len_threshold )
            {
              score = dist + len_score / len;

              if ( score < seg1->score )
              {
                seg1->score = score;
                seg1->link  = seg2;
              }

              if ( score < seg2->score )
              {
                seg2->score = score;
                seg2->link  = seg1;
              }
            }
          }
        }
    }

    /* now, compute the `serif' segments */
    for ( seg1 = segments; seg1 < segment_limit; seg1++ )
    {
      seg2 = seg1->link;

      if ( seg2 )
      {
        seg2->num_linked++;
        if ( seg2->link != seg1 )
        {
          seg1->link  = 0;
          seg1->serif = seg2->link;
        }
      }
    }
  }


  FT_LOCAL_DEF( FT_Error )
  af_latin_hints_compute_edges( AF_GlyphHints  hints,
                                AF_Dimension   dim )
  {
    AF_AxisHints  axis   = &hints->axis[dim];
    FT_Error      error  = AF_Err_Ok;
    FT_Memory     memory = hints->memory;
    AF_LatinAxis  laxis  = &((AF_LatinMetrics)hints->metrics)->axis[dim];

    AF_Segment    segments      = axis->segments;
    AF_Segment    segment_limit = segments + axis->num_segments;
    AF_Segment    seg;

    AF_Direction  up_dir;
    FT_Fixed      scale;
    FT_Pos        edge_distance_threshold;


    axis->num_edges = 0;

    scale = ( dim == AF_DIMENSION_HORZ ) ? hints->x_scale
                                         : hints->y_scale;

    up_dir = ( dim == AF_DIMENSION_HORZ ) ? AF_DIR_UP
                                          : AF_DIR_RIGHT;

    /*********************************************************************/
    /*                                                                   */
    /* We will begin by generating a sorted table of edges for the       */
    /* current direction.  To do so, we simply scan each segment and try */
    /* to find an edge in our table that corresponds to its position.    */
    /*                                                                   */
    /* If no edge is found, we create and insert a new edge in the       */
    /* sorted table.  Otherwise, we simply add the segment to the edge's */
    /* list which will be processed in the second step to compute the    */
    /* edge's properties.                                                */
    /*                                                                   */
    /* Note that the edges table is sorted along the segment/edge        */
    /* position.                                                         */
    /*                                                                   */
    /*********************************************************************/

    edge_distance_threshold = FT_MulFix( laxis->edge_distance_threshold,
                                         scale );
    if ( edge_distance_threshold > 64 / 4 )
      edge_distance_threshold = 64 / 4;

    edge_distance_threshold = FT_DivFix( edge_distance_threshold,
                                         scale );

    for ( seg = segments; seg < segment_limit; seg++ )
    {
      AF_Edge  found = 0;
      FT_Int   ee;


      /* look for an edge corresponding to the segment */
      for ( ee = 0; ee < axis->num_edges; ee++ )
      {
        AF_Edge  edge = axis->edges + ee;
        FT_Pos   dist;


        dist = seg->pos - edge->fpos;
        if ( dist < 0 )
          dist = -dist;

        if ( dist < edge_distance_threshold )
        {
          found = edge;
          break;
        }
      }

      if ( !found )
      {
        AF_Edge   edge;


        /* insert a new edge in the list and */
        /* sort according to the position    */
        error = af_axis_hints_new_edge( axis, seg->pos, memory, &edge );
        if ( error )
          goto Exit;

        /* add the segment to the new edge's list */
        FT_ZERO( edge );

        edge->first    = seg;
        edge->last     = seg;
        edge->fpos     = seg->pos;
        edge->opos     = edge->pos = FT_MulFix( seg->pos, scale );
        seg->edge_next = seg;
      }
      else
      {
        /* if an edge was found, simply add the segment to the edge's */
        /* list                                                       */
        seg->edge_next         = found->first;
        found->last->edge_next = seg;
        found->last            = seg;
      }
    }


    /*********************************************************************/
    /*                                                                   */
    /* Good, we will now compute each edge's properties according to     */
    /* segments found on its position.  Basically, these are:            */
    /*                                                                   */
    /*  - edge's main direction                                          */
    /*  - stem edge, serif edge or both (which defaults to stem then)    */
    /*  - rounded edge, straight or both (which defaults to straight)    */
    /*  - link for edge                                                  */
    /*                                                                   */
    /*********************************************************************/

    /* first of all, set the `edge' field in each segment -- this is */
    /* required in order to compute edge links                       */

    /*
     * Note that removing this loop and setting the `edge' field of each
     * segment directly in the code above slows down execution speed for
     * some reasons on platforms like the Sun.
     */
    {
      AF_Edge  edges      = axis->edges;
      AF_Edge  edge_limit = edges + axis->num_edges;
      AF_Edge  edge;


      for ( edge = edges; edge < edge_limit; edge++ )
      {
        seg = edge->first;
        if ( seg )
          do
          {
            seg->edge = edge;
            seg       = seg->edge_next;

          } while ( seg != edge->first );
      }

      /* now, compute each edge properties */
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        FT_Int  is_round    = 0;  /* does it contain round segments?    */
        FT_Int  is_straight = 0;  /* does it contain straight segments? */
        FT_Pos  ups         = 0;  /* number of upwards segments         */
        FT_Pos  downs       = 0;  /* number of downwards segments       */


        seg = edge->first;

        do
        {
          FT_Bool  is_serif;


          /* check for roundness of segment */
          if ( seg->flags & AF_EDGE_ROUND )
            is_round++;
          else
            is_straight++;

          /* check for segment direction */
          if ( seg->dir == up_dir )
            ups   += seg->max_coord-seg->min_coord;
          else
            downs += seg->max_coord-seg->min_coord;

          /* check for links -- if seg->serif is set, then seg->link must */
          /* be ignored                                                   */
          is_serif = (FT_Bool)( seg->serif && seg->serif->edge != edge );

          if ( seg->link || is_serif )
          {
            AF_Edge     edge2;
            AF_Segment  seg2;


            edge2 = edge->link;
            seg2  = seg->link;

            if ( is_serif )
            {
              seg2  = seg->serif;
              edge2 = edge->serif;
            }

            if ( edge2 )
            {
              FT_Pos  edge_delta;
              FT_Pos  seg_delta;


              edge_delta = edge->fpos - edge2->fpos;
              if ( edge_delta < 0 )
                edge_delta = -edge_delta;

              seg_delta = seg->pos - seg2->pos;
              if ( seg_delta < 0 )
                seg_delta = -seg_delta;

              if ( seg_delta < edge_delta )
                edge2 = seg2->edge;
            }
            else
              edge2 = seg2->edge;

            if ( is_serif )
            {
              edge->serif   = edge2;
              edge2->flags |= AF_EDGE_SERIF;
            }
            else
              edge->link  = edge2;
          }

          seg = seg->edge_next;

        } while ( seg != edge->first );

        /* set the round/straight flags */
        edge->flags = AF_EDGE_NORMAL;

        if ( is_round > 0 && is_round >= is_straight )
          edge->flags |= AF_EDGE_ROUND;

        /* set the edge's main direction */
        edge->dir = AF_DIR_NONE;

        if ( ups > downs )
          edge->dir = (FT_Char)up_dir;

        else if ( ups < downs )
          edge->dir = (FT_Char)-up_dir;

        else if ( ups == downs )
          edge->dir = 0;  /* both up and down! */

        /* gets rid of serifs if link is set                */
        /* XXX: This gets rid of many unpleasant artefacts! */
        /*      Example: the `c' in cour.pfa at size 13     */

        if ( edge->serif && edge->link )
          edge->serif = 0;
      }
    }

  Exit:
    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  af_latin_hints_detect_features( AF_GlyphHints  hints,
                                  AF_Dimension   dim )
  {
    FT_Error  error;


    error = af_latin_hints_compute_segments( hints, dim );
    if ( !error )
    {
      af_latin_hints_link_segments( hints, dim );

      error = af_latin_hints_compute_edges( hints, dim );
    }
    return error;
  }


  FT_LOCAL_DEF( void )
  af_latin_hints_compute_blue_edges( AF_GlyphHints    hints,
                                     AF_LatinMetrics  metrics )
  {
    AF_AxisHints  axis       = &hints->axis[ AF_DIMENSION_VERT ];
    AF_Edge       edge       = axis->edges;
    AF_Edge       edge_limit = edge + axis->num_edges;
    AF_LatinAxis  latin      = &metrics->axis[ AF_DIMENSION_VERT ];
    FT_Fixed      scale      = latin->scale;


    /* compute which blue zones are active, i.e. have their scaled */
    /* size < 3/4 pixels                                           */

    /* for each horizontal edge search the blue zone which is closest */
    for ( ; edge < edge_limit; edge++ )
    {
      FT_Int    bb;
      AF_Width  best_blue = NULL;
      FT_Pos    best_dist;  /* initial threshold */


      /* compute the initial threshold as a fraction of the EM size */
      best_dist = FT_MulFix( metrics->units_per_em / 40, scale );

      if ( best_dist > 64 / 2 )
        best_dist = 64 / 2;

      for ( bb = 0; bb < AF_LATIN_BLUE_MAX; bb++ )
      {
        AF_LatinBlue  blue = latin->blues + bb;
        FT_Bool       is_top_blue, is_major_dir;


        /* skip inactive blue zones (i.e., those that are too small) */
        if ( !( blue->flags & AF_LATIN_BLUE_ACTIVE ) )
          continue;

        /* if it is a top zone, check for right edges -- if it is a bottom */
        /* zone, check for left edges                                      */
        /*                                                                 */
        /* of course, that's for TrueType                                  */
        is_top_blue  = (FT_Byte)( ( blue->flags & AF_LATIN_BLUE_TOP ) != 0 );
        is_major_dir = FT_BOOL( edge->dir == axis->major_dir );

        /* if it is a top zone, the edge must be against the major    */
        /* direction; if it is a bottom zone, it must be in the major */
        /* direction                                                  */
        if ( is_top_blue ^ is_major_dir )
        {
          FT_Pos  dist;


          /* first of all, compare it to the reference position */
          dist = edge->fpos - blue->ref.org;
          if ( dist < 0 )
            dist = -dist;

          dist = FT_MulFix( dist, scale );
          if ( dist < best_dist )
          {
            best_dist = dist;
            best_blue = & blue->ref;
          }

          /* now, compare it to the overshoot position if the edge is     */
          /* rounded, and if the edge is over the reference position of a */
          /* top zone, or under the reference position of a bottom zone   */
          if ( edge->flags & AF_EDGE_ROUND && dist != 0 )
          {
            FT_Bool  is_under_ref = FT_BOOL( edge->fpos < blue->ref.org );


            if ( is_top_blue ^ is_under_ref )
            {
              blue = latin->blues + bb;
              dist = edge->fpos - blue->shoot.org;
              if ( dist < 0 )
                dist = -dist;

              dist = FT_MulFix( dist, scale );
              if ( dist < best_dist )
              {
                best_dist = dist;
                best_blue = & blue->shoot;
              }
            }
          }
        }
      }

      if ( best_blue )
        edge->blue_edge = best_blue;
    }
  }


  static FT_Error
  af_latin_hints_init( AF_GlyphHints    hints,
                       AF_LatinMetrics  metrics )
  {
    FT_Render_Mode  mode;
    FT_UInt32       scaler_flags, other_flags;
    FT_Face         face = metrics->root.scaler.face;


    af_glyph_hints_rescale( hints, (AF_ScriptMetrics)metrics );

    /*
     *  correct x_scale and y_scale when needed, since they may have
     *  been modified af_latin_scale_dim above
     */
    hints->x_scale = metrics->axis[AF_DIMENSION_HORZ].scale;
    hints->x_delta = metrics->axis[AF_DIMENSION_HORZ].delta;
    hints->y_scale = metrics->axis[AF_DIMENSION_VERT].scale;
    hints->y_delta = metrics->axis[AF_DIMENSION_VERT].delta;

    /* compute flags depending on render mode, etc. */
    mode = metrics->root.scaler.render_mode;

#ifdef AF_USE_WARPER
    if ( mode == FT_RENDER_MODE_LCD || mode == FT_RENDER_MODE_LCD_V )
    {
      metrics->root.scaler.render_mode = mode = FT_RENDER_MODE_NORMAL;
    }
#endif

    scaler_flags = hints->scaler_flags;
    other_flags  = 0;

    /*
     *  We snap the width of vertical stems for the monochrome and
     *  horizontal LCD rendering targets only.
     */
    if ( mode == FT_RENDER_MODE_MONO || mode == FT_RENDER_MODE_LCD )
      other_flags |= AF_LATIN_HINTS_HORZ_SNAP;

    /*
     *  We snap the width of horizontal stems for the monochrome and
     *  vertical LCD rendering targets only.
     */
    if ( mode == FT_RENDER_MODE_MONO || mode == FT_RENDER_MODE_LCD_V )
      other_flags |= AF_LATIN_HINTS_VERT_SNAP;

    /*
     *  We adjust stems to full pixels only if we don't use the `light' mode.
     */
    if ( mode != FT_RENDER_MODE_LIGHT )
      other_flags |= AF_LATIN_HINTS_STEM_ADJUST;

    if ( mode == FT_RENDER_MODE_MONO )
      other_flags |= AF_LATIN_HINTS_MONO;

    /*
     *  In `light' hinting mode we disable horizontal hinting completely.
     *  We also do it if the face is italic.
     */
    if ( mode == FT_RENDER_MODE_LIGHT                    ||
         (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0 )
      scaler_flags |= AF_SCALER_FLAG_NO_HORIZONTAL;

    hints->scaler_flags = scaler_flags;
    hints->other_flags  = other_flags;

    return 0;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****        L A T I N   G L Y P H   G R I D - F I T T I N G        *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* snap a given width in scaled coordinates to one of the */
  /* current standard widths                                */

  static FT_Pos
  af_latin_snap_width( AF_Width  widths,
                       FT_Int    count,
                       FT_Pos    width )
  {
    int     n;
    FT_Pos  best      = 64 + 32 + 2;
    FT_Pos  reference = width;
    FT_Pos  scaled;


    for ( n = 0; n < count; n++ )
    {
      FT_Pos  w;
      FT_Pos  dist;


      w = widths[n].cur;
      dist = width - w;
      if ( dist < 0 )
        dist = -dist;
      if ( dist < best )
      {
        best      = dist;
        reference = w;
      }
    }

    scaled = FT_PIX_ROUND( reference );

    if ( width >= reference )
    {
      if ( width < scaled + 48 )
        width = reference;
    }
    else
    {
      if ( width > scaled - 48 )
        width = reference;
    }

    return width;
  }


  /* compute the snapped width of a given stem */

  static FT_Pos
  af_latin_compute_stem_width( AF_GlyphHints  hints,
                               AF_Dimension   dim,
                               FT_Pos         width,
                               AF_Edge_Flags  base_flags,
                               AF_Edge_Flags  stem_flags )
  {
    AF_LatinMetrics  metrics  = (AF_LatinMetrics) hints->metrics;
    AF_LatinAxis     axis     = & metrics->axis[dim];
    FT_Pos           dist     = width;
    FT_Int           sign     = 0;
    FT_Int           vertical = ( dim == AF_DIMENSION_VERT );


    if ( !AF_LATIN_HINTS_DO_STEM_ADJUST( hints ) )
      return width;

    if ( dist < 0 )
    {
      dist = -width;
      sign = 1;
    }

    if ( (  vertical && !AF_LATIN_HINTS_DO_VERT_SNAP( hints ) ) ||
         ( !vertical && !AF_LATIN_HINTS_DO_HORZ_SNAP( hints ) ) )
    {
      /* smooth hinting process: very lightly quantize the stem width */

      /* leave the widths of serifs alone */

      if ( ( stem_flags & AF_EDGE_SERIF ) && vertical && ( dist < 3 * 64 ) )
        goto Done_Width;

      else if ( ( base_flags & AF_EDGE_ROUND ) )
      {
        if ( dist < 80 )
          dist = 64;
      }
      else if ( dist < 56 )
        dist = 56;

      if ( axis->width_count > 0 )
      {
        FT_Pos  delta;


        /* compare to standard width */
        if ( axis->width_count > 0 )
        {
          delta = dist - axis->widths[0].cur;

          if ( delta < 0 )
            delta = -delta;

          if ( delta < 40 )
          {
            dist = axis->widths[0].cur;
            if ( dist < 48 )
              dist = 48;

            goto Done_Width;
          }
        }

        if ( dist < 3 * 64 )
        {
          delta  = dist & 63;
          dist  &= -64;

          if ( delta < 10 )
            dist += delta;

          else if ( delta < 32 )
            dist += 10;

          else if ( delta < 54 )
            dist += 54;

          else
            dist += delta;
        }
        else
          dist = ( dist + 32 ) & ~63;
      }
    }
    else
    {
      /* strong hinting process: snap the stem width to integer pixels */

      dist = af_latin_snap_width( axis->widths, axis->width_count, dist );

      if ( vertical )
      {
        /* in the case of vertical hinting, always round */
        /* the stem heights to integer pixels            */

        if ( dist >= 64 )
          dist = ( dist + 16 ) & ~63;
        else
          dist = 64;
      }
      else
      {
        if ( AF_LATIN_HINTS_DO_MONO( hints ) )
        {
          /* monochrome horizontal hinting: snap widths to integer pixels */
          /* with a different threshold                                   */

          if ( dist < 64 )
            dist = 64;
          else
            dist = ( dist + 32 ) & ~63;
        }
        else
        {
          /* for horizontal anti-aliased hinting, we adopt a more subtle */
          /* approach: we strengthen small stems, round stems whose size */
          /* is between 1 and 2 pixels to an integer, otherwise nothing  */

          if ( dist < 48 )
            dist = ( dist + 64 ) >> 1;

          else if ( dist < 128 )
            dist = ( dist + 22 ) & ~63;
          else
            /* round otherwise to prevent color fringes in LCD mode */
            dist = ( dist + 32 ) & ~63;
        }
      }
    }

  Done_Width:
    if ( sign )
      dist = -dist;

    return dist;
  }


  /* align one stem edge relative to the previous stem edge */

  static void
  af_latin_align_linked_edge( AF_GlyphHints  hints,
                              AF_Dimension   dim,
                              AF_Edge        base_edge,
                              AF_Edge        stem_edge )
  {
    FT_Pos  dist = stem_edge->opos - base_edge->opos;

    FT_Pos  fitted_width = af_latin_compute_stem_width(
                             hints, dim, dist,
                             (AF_Edge_Flags)base_edge->flags,
                             (AF_Edge_Flags)stem_edge->flags );


    stem_edge->pos = base_edge->pos + fitted_width;
  }


  static void
  af_latin_align_serif_edge( AF_GlyphHints  hints,
                             AF_Edge        base,
                             AF_Edge        serif )
  {
    FT_UNUSED( hints );

    serif->pos = base->pos + (serif->opos - base->opos);
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                    E D G E   H I N T I N G                      ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  FT_LOCAL_DEF( void )
  af_latin_hint_edges( AF_GlyphHints  hints,
                       AF_Dimension   dim )
  {
    AF_AxisHints  axis       = &hints->axis[dim];
    AF_Edge       edges      = axis->edges;
    AF_Edge       edge_limit = edges + axis->num_edges;
    FT_Int        n_edges;
    AF_Edge       edge;
    AF_Edge       anchor     = 0;
    FT_Int        has_serifs = 0;


    /* we begin by aligning all stems relative to the blue zone */
    /* if needed -- that's only for horizontal edges            */

    if ( dim == AF_DIMENSION_VERT )
    {
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        AF_Width  blue;
        AF_Edge   edge1, edge2;


        if ( edge->flags & AF_EDGE_DONE )
          continue;

        blue  = edge->blue_edge;
        edge1 = NULL;
        edge2 = edge->link;

        if ( blue )
        {
          edge1 = edge;
        }
        else if ( edge2 && edge2->blue_edge )
        {
          blue  = edge2->blue_edge;
          edge1 = edge2;
          edge2 = edge;
        }

        if ( !edge1 )
          continue;

        edge1->pos    = blue->fit;
        edge1->flags |= AF_EDGE_DONE;

        if ( edge2 && !edge2->blue_edge )
        {
          af_latin_align_linked_edge( hints, dim, edge1, edge2 );
          edge2->flags |= AF_EDGE_DONE;
        }

        if ( !anchor )
          anchor = edge;
      }
    }

    /* now we will align all stem edges, trying to maintain the */
    /* relative order of stems in the glyph                     */
    for ( edge = edges; edge < edge_limit; edge++ )
    {
      AF_Edge  edge2;


      if ( edge->flags & AF_EDGE_DONE )
        continue;

      /* skip all non-stem edges */
      edge2 = edge->link;
      if ( !edge2 )
      {
        has_serifs++;
        continue;
      }

      /* now align the stem */

      /* this should not happen, but it's better to be safe */
      if ( edge2->blue_edge || edge2 < edge )
      {
        af_latin_align_linked_edge( hints, dim, edge2, edge );
        edge->flags |= AF_EDGE_DONE;
        continue;
      }

      if ( !anchor )
      {
        FT_Pos  org_len, org_center, cur_len;
        FT_Pos  cur_pos1, error1, error2, u_off, d_off;


        org_len = edge2->opos - edge->opos;
        cur_len = af_latin_compute_stem_width(
                    hints, dim, org_len,
                    (AF_Edge_Flags)edge->flags,
                    (AF_Edge_Flags)edge2->flags );
        if ( cur_len <= 64 )
          u_off = d_off = 32;
        else
        {
          u_off = 38;
          d_off = 26;
        }

        if ( cur_len < 96 )
        {
          org_center = edge->opos + ( org_len >> 1 );

          cur_pos1   = FT_PIX_ROUND( org_center );

          error1 = org_center - ( cur_pos1 - u_off );
          if ( error1 < 0 )
            error1 = -error1;

          error2 = org_center - ( cur_pos1 + d_off );
          if ( error2 < 0 )
            error2 = -error2;

          if ( error1 < error2 )
            cur_pos1 -= u_off;
          else
            cur_pos1 += d_off;

          edge->pos  = cur_pos1 - cur_len / 2;
          edge2->pos = cur_pos1 + cur_len / 2;

        }
        else
          edge->pos = FT_PIX_ROUND( edge->opos );

        anchor = edge;

        edge->flags |= AF_EDGE_DONE;

        af_latin_align_linked_edge( hints, dim, edge, edge2 );
      }
      else
      {
        FT_Pos  org_pos, org_len, org_center, cur_len;
        FT_Pos  cur_pos1, cur_pos2, delta1, delta2;


        org_pos    = anchor->pos + ( edge->opos - anchor->opos );
        org_len    = edge2->opos - edge->opos;
        org_center = org_pos + ( org_len >> 1 );

        cur_len = af_latin_compute_stem_width(
                   hints, dim, org_len,
                   (AF_Edge_Flags)edge->flags,
                   (AF_Edge_Flags)edge2->flags );

        if ( cur_len < 96 )
        {
          FT_Pos  u_off, d_off;


          cur_pos1 = FT_PIX_ROUND( org_center );

          if (cur_len <= 64 )
            u_off = d_off = 32;
          else
          {
            u_off = 38;
            d_off = 26;
          }

          delta1 = org_center - ( cur_pos1 - u_off );
          if ( delta1 < 0 )
            delta1 = -delta1;

          delta2 = org_center - ( cur_pos1 + d_off );
          if ( delta2 < 0 )
            delta2 = -delta2;

          if ( delta1 < delta2 )
            cur_pos1 -= u_off;
          else
            cur_pos1 += d_off;

          edge->pos  = cur_pos1 - cur_len / 2;
          edge2->pos = cur_pos1 + cur_len / 2;
        }
        else
        {
          org_pos    = anchor->pos + ( edge->opos - anchor->opos );
          org_len    = edge2->opos - edge->opos;
          org_center = org_pos + ( org_len >> 1 );

          cur_len    = af_latin_compute_stem_width(
                         hints, dim, org_len,
                         (AF_Edge_Flags)edge->flags,
                         (AF_Edge_Flags)edge2->flags );

          cur_pos1   = FT_PIX_ROUND( org_pos );
          delta1     = cur_pos1 + ( cur_len >> 1 ) - org_center;
          if ( delta1 < 0 )
            delta1 = -delta1;

          cur_pos2   = FT_PIX_ROUND( org_pos + org_len ) - cur_len;
          delta2     = cur_pos2 + ( cur_len >> 1 ) - org_center;
          if ( delta2 < 0 )
            delta2 = -delta2;

          edge->pos  = ( delta1 < delta2 ) ? cur_pos1 : cur_pos2;
          edge2->pos = edge->pos + cur_len;
        }

        edge->flags  |= AF_EDGE_DONE;
        edge2->flags |= AF_EDGE_DONE;

        if ( edge > edges && edge->pos < edge[-1].pos )
          edge->pos = edge[-1].pos;
      }
    }

    /* make sure that lowercase m's maintain their symmetry */

    /* In general, lowercase m's have six vertical edges if they are sans */
    /* serif, or twelve if they are with serifs.  This implementation is  */
    /* based on that assumption, and seems to work very well with most    */
    /* faces.  However, if for a certain face this assumption is not      */
    /* true, the m is just rendered like before.  In addition, any stem   */
    /* correction will only be applied to symmetrical glyphs (even if the */
    /* glyph is not an m), so the potential for unwanted distortion is    */
    /* relatively low.                                                    */

    /* We don't handle horizontal edges since we can't easily assure that */
    /* the third (lowest) stem aligns with the base line; it might end up */
    /* one pixel higher or lower.                                         */

    n_edges = edge_limit - edges;
    if ( dim == AF_DIMENSION_HORZ && ( n_edges == 6 || n_edges == 12 ) )
    {
      AF_Edge  edge1, edge2, edge3;
      FT_Pos   dist1, dist2, span, delta;


      if ( n_edges == 6 )
      {
        edge1 = edges;
        edge2 = edges + 2;
        edge3 = edges + 4;
      }
      else
      {
        edge1 = edges + 1;
        edge2 = edges + 5;
        edge3 = edges + 9;
      }

      dist1 = edge2->opos - edge1->opos;
      dist2 = edge3->opos - edge2->opos;

      span = dist1 - dist2;
      if ( span < 0 )
        span = -span;

      if ( span < 8 )
      {
        delta = edge3->pos - ( 2 * edge2->pos - edge1->pos );
        edge3->pos -= delta;
        if ( edge3->link )
          edge3->link->pos -= delta;

        /* move the serifs along with the stem */
        if ( n_edges == 12 )
        {
          ( edges + 8 )->pos -= delta;
          ( edges + 11 )->pos -= delta;
        }

        edge3->flags |= AF_EDGE_DONE;
        if ( edge3->link )
          edge3->link->flags |= AF_EDGE_DONE;
      }
    }

    if ( has_serifs || !anchor )
    {
      /*
       *  now hint the remaining edges (serifs and single) in order
       *  to complete our processing
       */
      for ( edge = edges; edge < edge_limit; edge++ )
      {
        if ( edge->flags & AF_EDGE_DONE )
          continue;

        if ( edge->serif )
          af_latin_align_serif_edge( hints, edge->serif, edge );
        else if ( !anchor )
        {
          edge->pos = FT_PIX_ROUND( edge->opos );
          anchor    = edge;
        }
        else
          edge->pos = anchor->pos +
                      FT_PIX_ROUND( edge->opos - anchor->opos );

        edge->flags |= AF_EDGE_DONE;

        if ( edge > edges && edge->pos < edge[-1].pos )
          edge->pos = edge[-1].pos;

        if ( edge + 1 < edge_limit        &&
             edge[1].flags & AF_EDGE_DONE &&
             edge->pos > edge[1].pos      )
          edge->pos = edge[1].pos;
      }
    }
  }


  static FT_Error
  af_latin_hints_apply( AF_GlyphHints    hints,
                        FT_Outline*      outline,
                        AF_LatinMetrics  metrics )
  {
    FT_Error  error;
    int       dim;


    error = af_glyph_hints_reload( hints, outline );
    if ( error )
      goto Exit;

    /* analyze glyph outline */
    if ( AF_HINTS_DO_HORIZONTAL( hints ) )
    {
      error = af_latin_hints_detect_features( hints, AF_DIMENSION_HORZ );
      if ( error )
        goto Exit;
    }

    if ( AF_HINTS_DO_VERTICAL( hints ) )
    {
      error = af_latin_hints_detect_features( hints, AF_DIMENSION_VERT );
      if ( error )
        goto Exit;

      af_latin_hints_compute_blue_edges( hints, metrics );
    }

    /* grid-fit the outline */
    for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
    {
      if ( ( dim == AF_DIMENSION_HORZ && AF_HINTS_DO_HORIZONTAL( hints ) ) ||
           ( dim == AF_DIMENSION_VERT && AF_HINTS_DO_VERTICAL( hints ) )   )
      {
#ifdef AF_USE_WARPER
        if ( dim == AF_DIMENSION_HORZ &&
             metrics->root.scaler.render_mode == FT_RENDER_MODE_NORMAL )
        {
          AF_WarperRec  warper;
          FT_Fixed      scale;
          FT_Pos        delta;


          af_warper_compute( &warper, hints, dim, &scale, &delta );
          af_glyph_hints_scale_dim( hints, dim, scale, delta );
          continue;
        }
#endif
        af_latin_hint_edges( hints, (AF_Dimension)dim );
        af_glyph_hints_align_edge_points( hints, (AF_Dimension)dim );
        af_glyph_hints_align_strong_points( hints, (AF_Dimension)dim );
        af_glyph_hints_align_weak_points( hints, (AF_Dimension)dim );
      }
    }
    af_glyph_hints_save( hints, outline );

  Exit:
    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****              L A T I N   S C R I P T   C L A S S              *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  static const AF_Script_UniRangeRec  af_latin_uniranges[] =
  {
    { 32,  127 },    /* XXX: TODO: Add new Unicode ranges here! */
    { 160, 255 },
    { 0,   0 }
  };


  FT_CALLBACK_TABLE_DEF const AF_ScriptClassRec
  af_latin_script_class =
  {
    AF_SCRIPT_LATIN,
    af_latin_uniranges,

    sizeof( AF_LatinMetricsRec ),

    (AF_Script_InitMetricsFunc) af_latin_metrics_init,
    (AF_Script_ScaleMetricsFunc)af_latin_metrics_scale,
    (AF_Script_DoneMetricsFunc) NULL,

    (AF_Script_InitHintsFunc)   af_latin_hints_init,
    (AF_Script_ApplyHintsFunc)  af_latin_hints_apply
  };


/* END */
