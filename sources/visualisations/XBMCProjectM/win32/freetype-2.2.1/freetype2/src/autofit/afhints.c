/***************************************************************************/
/*                                                                         */
/*  afhints.c                                                              */
/*                                                                         */
/*    Auto-fitter hinting routines (body).                                 */
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


#include "afhints.h"
#include "aferrors.h"


  FT_LOCAL_DEF( FT_Error )
  af_axis_hints_new_segment( AF_AxisHints  axis,
                             FT_Memory     memory,
                             AF_Segment   *asegment )
  {
    FT_Error    error   = AF_Err_Ok;
    AF_Segment  segment = NULL;


    if ( axis->num_segments >= axis->max_segments )
    {
      FT_Int  old_max = axis->max_segments;
      FT_Int  new_max = old_max;
      FT_Int  big_max = FT_INT_MAX / sizeof ( *segment );


      if ( old_max >= big_max )
      {
        error = AF_Err_Out_Of_Memory;
        goto Exit;
      }

      new_max += ( new_max >> 2 ) + 4;
      if ( new_max < old_max || new_max > big_max )
        new_max = big_max;

      if ( FT_RENEW_ARRAY( axis->segments, old_max, new_max ) )
        goto Exit;

      axis->max_segments = new_max;
    }

    segment = axis->segments + axis->num_segments++;
    FT_ZERO( segment );

  Exit:
    *asegment = segment;
    return error;
  }


  FT_LOCAL( FT_Error )
  af_axis_hints_new_edge( AF_AxisHints  axis,
                          FT_Int        fpos,
                          FT_Memory     memory,
                          AF_Edge      *aedge )
  {
    FT_Error  error = AF_Err_Ok;
    AF_Edge   edge  = NULL;
    AF_Edge   edges;


    if ( axis->num_edges >= axis->max_edges )
    {
      FT_Int  old_max = axis->max_edges;
      FT_Int  new_max = old_max;
      FT_Int  big_max = FT_INT_MAX / sizeof ( *edge );


      if ( old_max >= big_max )
      {
        error = AF_Err_Out_Of_Memory;
        goto Exit;
      }

      new_max += ( new_max >> 2 ) + 4;
      if ( new_max < old_max || new_max > big_max )
        new_max = big_max;

      if ( FT_RENEW_ARRAY( axis->edges, old_max, new_max ) )
        goto Exit;

      axis->max_edges = new_max;
    }

    edges = axis->edges;
    edge  = edges + axis->num_edges;

    while ( edge > edges && edge[-1].fpos > fpos )
    {
      edge[0] = edge[-1];
      edge--;
    }

    axis->num_edges++;

    FT_ZERO( edge );
    edge->fpos = (FT_Short)fpos;

  Exit:
    *aedge = edge;
    return error;
  }


#ifdef AF_DEBUG

#include <stdio.h>

  static const char*
  af_dir_str( AF_Direction  dir )
  {
    const char*  result;


    switch ( dir )
    {
    case AF_DIR_UP:
      result = "up";
      break;
    case AF_DIR_DOWN:
      result = "down";
      break;
    case AF_DIR_LEFT:
      result = "left";
      break;
    case AF_DIR_RIGHT:
      result = "right";
      break;
    default:
      result = "none";
    }

    return result;
  }


#define AF_INDEX_NUM( ptr, base )  ( (ptr) ? ( (ptr) - (base) ) : -1 )


  void
  af_glyph_hints_dump_points( AF_GlyphHints  hints )
  {
    AF_Point  points = hints->points;
    AF_Point  limit  = points + hints->num_points;
    AF_Point  point;


    printf( "Table of points:\n" );
    printf(   "  [ index |  xorg |  yorg |  xscale |  yscale "
              "|  xfit  |  yfit  |  flags ]\n" );

    for ( point = points; point < limit; point++ )
    {
      printf( "  [ %5d | %5d | %5d | %-5.2f | %-5.2f "
              "| %-5.2f | %-5.2f | %c%c%c%c%c%c ]\n",
              point - points,
              point->fx,
              point->fy,
              point->ox/64.0,
              point->oy/64.0,
              point->x/64.0,
              point->y/64.0,
              ( point->flags & AF_FLAG_WEAK_INTERPOLATION ) ? 'w' : ' ',
              ( point->flags & AF_FLAG_INFLECTION )         ? 'i' : ' ',
              ( point->flags & AF_FLAG_EXTREMA_X )          ? '<' : ' ',
              ( point->flags & AF_FLAG_EXTREMA_Y )          ? 'v' : ' ',
              ( point->flags & AF_FLAG_ROUND_X )            ? '(' : ' ',
              ( point->flags & AF_FLAG_ROUND_Y )            ? 'u' : ' ');
    }
    printf( "\n" );
  }


  /* A function to dump the array of linked segments. */
  void
  af_glyph_hints_dump_segments( AF_GlyphHints  hints )
  {
    AF_Point  points = hints->points;
    FT_Int    dimension;


    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AF_AxisHints  axis     = &hints->axis[dimension];
      AF_Segment    segments = axis->segments;
      AF_Segment    limit    = segments + axis->num_segments;
      AF_Segment    seg;


      printf ( "Table of %s segments:\n",
               dimension == AF_DIMENSION_HORZ ? "vertical" : "horizontal" );
      printf ( "  [ index |  pos |  dir  | link | serif |"
               " numl | first | start ]\n" );

      for ( seg = segments; seg < limit; seg++ )
      {
        printf ( "  [ %5d | %4d | %5s | %4d | %5d | %4d | %5d | %5d ]\n",
                 seg - segments,
                 (int)seg->pos,
                 af_dir_str( seg->dir ),
                 AF_INDEX_NUM( seg->link, segments ),
                 AF_INDEX_NUM( seg->serif, segments ),
                 (int)seg->num_linked,
                 seg->first - points,
                 seg->last - points );
      }
      printf( "\n" );
    }
  }


  void
  af_glyph_hints_dump_edges( AF_GlyphHints  hints )
  {
    FT_Int  dimension;


    for ( dimension = 1; dimension >= 0; dimension-- )
    {
      AF_AxisHints  axis  = &hints->axis[dimension];
      AF_Edge       edges = axis->edges;
      AF_Edge       limit = edges + axis->num_edges;
      AF_Edge       edge;


      /*
       *  note: AF_DIMENSION_HORZ corresponds to _vertical_ edges
       *        since they have constant a X coordinate.
       */
      printf ( "Table of %s edges:\n",
               dimension == AF_DIMENSION_HORZ ? "vertical" : "horizontal" );
      printf ( "  [ index |  pos |  dir  | link |"
               " serif | blue | opos  |  pos  ]\n" );

      for ( edge = edges; edge < limit; edge++ )
      {
        printf ( "  [ %5d | %4d | %5s | %4d |"
                 " %5d |   %c  | %5.2f | %5.2f ]\n",
                 edge - edges,
                 (int)edge->fpos,
                 af_dir_str( edge->dir ),
                 AF_INDEX_NUM( edge->link, edges ),
                 AF_INDEX_NUM( edge->serif, edges ),
                 edge->blue_edge ? 'y' : 'n',
                 edge->opos / 64.0,
                 edge->pos / 64.0 );
      }
      printf( "\n" );
    }
  }

#endif /* AF_DEBUG */



  /* compute the direction value of a given vector */
  FT_LOCAL_DEF( AF_Direction )
  af_direction_compute( FT_Pos  dx,
                        FT_Pos  dy )
  {
#if 1
    AF_Direction  dir = AF_DIR_NONE;


    /* atan(1/12) == 4.7 degrees */

    if ( dx < 0 )
    {
      if ( dy < 0 )
      {
        if ( -dx * 12 < -dy )
          dir = AF_DIR_DOWN;

        else if ( -dy * 12 < -dx )
          dir = AF_DIR_LEFT;
      }
      else /* dy >= 0 */
      {
        if ( -dx * 12 < dy )
          dir = AF_DIR_UP;

        else if ( dy * 12 < -dx )
          dir = AF_DIR_LEFT;
      }
    }
    else /* dx >= 0 */
    {
      if ( dy < 0 )
      {
        if ( dx * 12 < -dy )
          dir = AF_DIR_DOWN;

        else if ( -dy * 12 < dx )
          dir = AF_DIR_RIGHT;
      }
      else  /* dy >= 0 */
      {
        if ( dx * 12 < dy )
          dir = AF_DIR_UP;

        else if ( dy * 12 < dx )
          dir = AF_DIR_RIGHT;
      }
    }

    return dir;

#else /* 0 */

    AF_Direction  dir;
    FT_Pos        ax = FT_ABS( dx );
    FT_Pos        ay = FT_ABS( dy );


    dir = AF_DIR_NONE;

    /* atan(1/12) == 4.7 degrees */

    /* test for vertical direction */
    if ( ax * 12 < ay )
    {
      dir = dy > 0 ? AF_DIR_UP : AF_DIR_DOWN;
    }
    /* test for horizontal direction */
    else if ( ay * 12 < ax )
    {
      dir = dx > 0 ? AF_DIR_RIGHT : AF_DIR_LEFT;
    }

    return dir;

#endif /* 0 */

  }


  /* compute all inflex points in a given glyph */
  static void
  af_glyph_hints_compute_inflections( AF_GlyphHints  hints )
  {
    AF_Point*  contour       = hints->contours;
    AF_Point*  contour_limit = contour + hints->num_contours;


    /* do each contour separately */
    for ( ; contour < contour_limit; contour++ )
    {
      AF_Point  point = contour[0];
      AF_Point  first = point;
      AF_Point  start = point;
      AF_Point  end   = point;
      AF_Point  before;
      AF_Point  after;
      AF_Angle  angle_in, angle_seg, angle_out;
      AF_Angle  diff_in, diff_out;
      FT_Int    finished = 0;


      /* compute first segment in contour */
      first = point;

      start = end = first;
      do
      {
        end = end->next;
        if ( end == first )
          goto Skip;

      } while ( end->fx == first->fx && end->fy == first->fy );

      angle_seg = af_angle_atan( end->fx - start->fx,
                                 end->fy - start->fy );

      /* extend the segment start whenever possible */
      before = start;
      do
      {
        do
        {
          start  = before;
          before = before->prev;
          if ( before == first )
            goto Skip;

        } while ( before->fx == start->fx && before->fy == start->fy );

        angle_in = af_angle_atan( start->fx - before->fx,
                                  start->fy - before->fy );

      } while ( angle_in == angle_seg );

      first = start;

      AF_ANGLE_DIFF( diff_in, angle_in, angle_seg );

      /* now, process all segments in the contour */
      do
      {
        /* first, extend current segment's end whenever possible */
        after = end;
        do
        {
          do
          {
            end   = after;
            after = after->next;
            if ( after == first )
              finished = 1;

          } while ( end->fx == after->fx && end->fy == after->fy );

          angle_out = af_angle_atan( after->fx - end->fx,
                                     after->fy - end->fy );

        } while ( angle_out == angle_seg );

        AF_ANGLE_DIFF( diff_out, angle_seg, angle_out );

        if ( ( diff_in ^ diff_out ) < 0 )
        {
          /* diff_in and diff_out have different signs, we have */
          /* inflection points here...                          */
          do
          {
            start->flags |= AF_FLAG_INFLECTION;
            start = start->next;

          } while ( start != end );

          start->flags |= AF_FLAG_INFLECTION;
        }

        start     = end;
        end       = after;
        angle_seg = angle_out;
        diff_in   = diff_out;

      } while ( !finished );

    Skip:
      ;
    }
  }


  FT_LOCAL_DEF( void )
  af_glyph_hints_init( AF_GlyphHints  hints,
                       FT_Memory      memory )
  {
    FT_ZERO( hints );
    hints->memory = memory;
  }


  FT_LOCAL_DEF( void )
  af_glyph_hints_done( AF_GlyphHints  hints )
  {
    if ( hints && hints->memory )
    {
      FT_Memory  memory = hints->memory;
      int        dim;


      /*
       *  note that we don't need to free the segment and edge
       *  buffers, since they are really within the hints->points array
       */
      for ( dim = 0; dim < AF_DIMENSION_MAX; dim++ )
      {
        AF_AxisHints  axis = &hints->axis[dim];


        axis->num_segments = 0;
        axis->max_segments = 0;
        FT_FREE( axis->segments );

        axis->num_edges    = 0;
        axis->max_edges    = 0;
        FT_FREE( axis->edges );
      }

      FT_FREE( hints->contours );
      hints->max_contours = 0;
      hints->num_contours = 0;

      FT_FREE( hints->points );
      hints->num_points = 0;
      hints->max_points = 0;

      hints->memory = NULL;
    }
  }


  FT_LOCAL_DEF( void )
  af_glyph_hints_rescale( AF_GlyphHints     hints,
                          AF_ScriptMetrics  metrics )
  {
    hints->metrics      = metrics;
    hints->scaler_flags = metrics->scaler.flags;
  }


  FT_LOCAL_DEF( FT_Error )
  af_glyph_hints_reload( AF_GlyphHints  hints,
                         FT_Outline*    outline )
  {
    FT_Error   error   = AF_Err_Ok;
    AF_Point   points;
    FT_UInt    old_max, new_max;
    FT_Fixed   x_scale = hints->x_scale;
    FT_Fixed   y_scale = hints->y_scale;
    FT_Pos     x_delta = hints->x_delta;
    FT_Pos     y_delta = hints->y_delta;
    FT_Memory  memory  = hints->memory;


    hints->num_points   = 0;
    hints->num_contours = 0;

    hints->axis[0].num_segments = 0;
    hints->axis[0].num_edges    = 0;
    hints->axis[1].num_segments = 0;
    hints->axis[1].num_edges    = 0;

    /* first of all, reallocate the contours array when necessary */
    new_max = (FT_UInt)outline->n_contours;
    old_max = hints->max_contours;
    if ( new_max > old_max )
    {
      new_max = ( new_max + 3 ) & ~3;

      if ( FT_RENEW_ARRAY( hints->contours, old_max, new_max ) )
        goto Exit;

      hints->max_contours = new_max;
    }

    /*
     *  then reallocate the points arrays if necessary --
     *  note that we reserve two additional point positions, used to
     *  hint metrics appropriately
     */
    new_max = (FT_UInt)( outline->n_points + 2 );
    old_max = hints->max_points;
    if ( new_max > old_max )
    {
      new_max = ( new_max + 2 + 7 ) & ~7;

      if ( FT_RENEW_ARRAY( hints->points, old_max, new_max ) )
        goto Exit;

      hints->max_points = new_max;
    }

    hints->num_points   = outline->n_points;
    hints->num_contours = outline->n_contours;

    /* We can't rely on the value of `FT_Outline.flags' to know the fill   */
    /* direction used for a glyph, given that some fonts are broken (e.g., */
    /* the Arphic ones).  We thus recompute it each time we need to.       */
    /*                                                                     */
    hints->axis[AF_DIMENSION_HORZ].major_dir = AF_DIR_UP;
    hints->axis[AF_DIMENSION_VERT].major_dir = AF_DIR_LEFT;

    if ( FT_Outline_Get_Orientation( outline ) == FT_ORIENTATION_POSTSCRIPT )
    {
      hints->axis[AF_DIMENSION_HORZ].major_dir = AF_DIR_DOWN;
      hints->axis[AF_DIMENSION_VERT].major_dir = AF_DIR_RIGHT;
    }

    hints->x_scale = x_scale;
    hints->y_scale = y_scale;
    hints->x_delta = x_delta;
    hints->y_delta = y_delta;

    points = hints->points;
    if ( hints->num_points == 0 )
      goto Exit;

    {
      AF_Point  point;
      AF_Point  point_limit = points + hints->num_points;


      /* compute coordinates & Bezier flags */
      {
        FT_Vector*  vec = outline->points;
        char*       tag = outline->tags;


        for ( point = points; point < point_limit; point++, vec++, tag++ )
        {
          point->fx = (FT_Short)vec->x;
          point->fy = (FT_Short)vec->y;
          point->ox = point->x = FT_MulFix( vec->x, x_scale ) + x_delta;
          point->oy = point->y = FT_MulFix( vec->y, y_scale ) + y_delta;

          switch ( FT_CURVE_TAG( *tag ) )
          {
          case FT_CURVE_TAG_CONIC:
            point->flags = AF_FLAG_CONIC;
            break;
          case FT_CURVE_TAG_CUBIC:
            point->flags = AF_FLAG_CUBIC;
            break;
          default:
            point->flags = 0;
          }
        }
      }

      /* compute `next' and `prev' */
      {
        FT_Int    contour_index;
        AF_Point  prev;
        AF_Point  first;
        AF_Point  end;


        contour_index = 0;

        first = points;
        end   = points + outline->contours[0];
        prev  = end;

        for ( point = points; point < point_limit; point++ )
        {
          point->prev = prev;
          if ( point < end )
          {
            point->next = point + 1;
            prev        = point;
          }
          else
          {
            point->next = first;
            contour_index++;
            if ( point + 1 < point_limit )
            {
              end   = points + outline->contours[contour_index];
              first = point + 1;
              prev  = end;
            }
          }
        }
      }

      /* set-up the contours array */
      {
        AF_Point*  contour       = hints->contours;
        AF_Point*  contour_limit = contour + hints->num_contours;
        short*     end           = outline->contours;
        short      idx           = 0;


        for ( ; contour < contour_limit; contour++, end++ )
        {
          contour[0] = points + idx;
          idx        = (short)( end[0] + 1 );
        }
      }

      /* compute directions of in & out vectors */
      {
        for ( point = points; point < point_limit; point++ )
        {
          AF_Point  prev;
          AF_Point  next;
          FT_Pos    in_x, in_y, out_x, out_y;


          prev   = point->prev;
          in_x   = point->fx - prev->fx;
          in_y   = point->fy - prev->fy;

          point->in_dir = (FT_Char)af_direction_compute( in_x, in_y );

          next   = point->next;
          out_x  = next->fx - point->fx;
          out_y  = next->fy - point->fy;

          point->out_dir = (FT_Char)af_direction_compute( out_x, out_y );

          if ( point->flags & ( AF_FLAG_CONIC | AF_FLAG_CUBIC ) )
          {
          Is_Weak_Point:
            point->flags |= AF_FLAG_WEAK_INTERPOLATION;
          }
          else if ( point->out_dir == point->in_dir )
          {
            AF_Angle  angle_in, angle_out, delta;


            if ( point->out_dir != AF_DIR_NONE )
              goto Is_Weak_Point;

            angle_in  = af_angle_atan( in_x, in_y );
            angle_out = af_angle_atan( out_x, out_y );

            AF_ANGLE_DIFF( delta, angle_in, angle_out );

            if ( delta < 2 && delta > -2 )
              goto Is_Weak_Point;
          }
          else if ( point->in_dir == -point->out_dir )
            goto Is_Weak_Point;
        }
      }
    }

    /* compute inflection points */
    af_glyph_hints_compute_inflections( hints );

  Exit:
    return error;
  }


  FT_LOCAL_DEF( void )
  af_glyph_hints_save( AF_GlyphHints  hints,
                       FT_Outline*    outline )
  {
    AF_Point    point = hints->points;
    AF_Point    limit = point + hints->num_points;
    FT_Vector*  vec   = outline->points;
    char*       tag   = outline->tags;


    for ( ; point < limit; point++, vec++, tag++ )
    {
      vec->x = point->x;
      vec->y = point->y;

      if ( point->flags & AF_FLAG_CONIC )
        tag[0] = FT_CURVE_TAG_CONIC;
      else if ( point->flags & AF_FLAG_CUBIC )
        tag[0] = FT_CURVE_TAG_CUBIC;
      else
        tag[0] = FT_CURVE_TAG_ON;
    }
  }


  /****************************************************************
   *
   *                     EDGE POINT GRID-FITTING
   *
   ****************************************************************/


  FT_LOCAL_DEF( void )
  af_glyph_hints_align_edge_points( AF_GlyphHints  hints,
                                    AF_Dimension   dim )
  {
    AF_AxisHints  axis       = & hints->axis[dim];
    AF_Edge       edges      = axis->edges;
    AF_Edge       edge_limit = edges + axis->num_edges;
    AF_Edge       edge;


    for ( edge = edges; edge < edge_limit; edge++ )
    {
      /* move the points of each segment     */
      /* in each edge to the edge's position */
      AF_Segment  seg = edge->first;


      do
      {
        AF_Point  point = seg->first;


        for (;;)
        {
          if ( dim == AF_DIMENSION_HORZ )
          {
            point->x      = edge->pos;
            point->flags |= AF_FLAG_TOUCH_X;
          }
          else
          {
            point->y      = edge->pos;
            point->flags |= AF_FLAG_TOUCH_Y;
          }

          if ( point == seg->last )
            break;

          point = point->next;
        }

        seg = seg->edge_next;

      } while ( seg != edge->first );
    }
  }


  /****************************************************************
   *
   *                    STRONG POINT INTERPOLATION
   *
   ****************************************************************/


  /* hint the strong points -- this is equivalent to the TrueType `IP' */
  /* hinting instruction                                               */

  FT_LOCAL_DEF( void )
  af_glyph_hints_align_strong_points( AF_GlyphHints  hints,
                                      AF_Dimension   dim )
  {
    AF_Point      points      = hints->points;
    AF_Point      point_limit = points + hints->num_points;
    AF_AxisHints  axis        = &hints->axis[dim];
    AF_Edge       edges       = axis->edges;
    AF_Edge       edge_limit  = edges + axis->num_edges;
    AF_Flags      touch_flag;


    if ( dim == AF_DIMENSION_HORZ )
      touch_flag = AF_FLAG_TOUCH_X;
    else
      touch_flag  = AF_FLAG_TOUCH_Y;

    if ( edges < edge_limit )
    {
      AF_Point  point;
      AF_Edge   edge;


      for ( point = points; point < point_limit; point++ )
      {
        FT_Pos  u, ou, fu;  /* point position */
        FT_Pos  delta;


        if ( point->flags & touch_flag )
          continue;

        /* if this point is candidate to weak interpolation, we       */
        /* interpolate it after all strong points have been processed */

        if (  ( point->flags & AF_FLAG_WEAK_INTERPOLATION ) &&
             !( point->flags & AF_FLAG_INFLECTION )         )
          continue;

        if ( dim == AF_DIMENSION_VERT )
        {
          u  = point->fy;
          ou = point->oy;
        }
        else
        {
          u  = point->fx;
          ou = point->ox;
        }

        fu = u;

        /* is the point before the first edge? */
        edge  = edges;
        delta = edge->fpos - u;
        if ( delta >= 0 )
        {
          u = edge->pos - ( edge->opos - ou );
          goto Store_Point;
        }

        /* is the point after the last edge? */
        edge  = edge_limit - 1;
        delta = u - edge->fpos;
        if ( delta >= 0 )
        {
          u = edge->pos + ( ou - edge->opos );
          goto Store_Point;
        }

        {
          FT_UInt  min, max, mid;
          FT_Pos   fpos;


          /* find enclosing edges */
          min = 0;
          max = edge_limit - edges;

          while ( min < max )
          {
            mid  = ( max + min ) >> 1;
            edge = edges + mid;
            fpos = edge->fpos;

            if ( u < fpos )
              max = mid;
            else if ( u > fpos )
              min = mid + 1;
            else
            {
              /* we are on the edge */
              u = edge->pos;
              goto Store_Point;
            }
          }

          {
            AF_Edge  before = edges + min - 1;
            AF_Edge  after  = edges + min + 0;


            /* assert( before && after && before != after ) */
            if ( before->scale == 0 )
              before->scale = FT_DivFix( after->pos - before->pos,
                                         after->fpos - before->fpos );

            u = before->pos + FT_MulFix( fu - before->fpos,
                                         before->scale );
          }
        }

      Store_Point:
        /* save the point position */
        if ( dim == AF_DIMENSION_HORZ )
          point->x = u;
        else
          point->y = u;

        point->flags |= touch_flag;
      }
    }
  }


  /****************************************************************
   *
   *                    WEAK POINT INTERPOLATION
   *
   ****************************************************************/


  static void
  af_iup_shift( AF_Point  p1,
                AF_Point  p2,
                AF_Point  ref )
  {
    AF_Point  p;
    FT_Pos    delta = ref->u - ref->v;


    for ( p = p1; p < ref; p++ )
      p->u = p->v + delta;

    for ( p = ref + 1; p <= p2; p++ )
      p->u = p->v + delta;
  }


  static void
  af_iup_interp( AF_Point  p1,
                 AF_Point  p2,
                 AF_Point  ref1,
                 AF_Point  ref2 )
  {
    AF_Point  p;
    FT_Pos    u;
    FT_Pos    v1 = ref1->v;
    FT_Pos    v2 = ref2->v;
    FT_Pos    d1 = ref1->u - v1;
    FT_Pos    d2 = ref2->u - v2;


    if ( p1 > p2 )
      return;

    if ( v1 == v2 )
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v1 )
          u += d1;
        else
          u += d2;

        p->u = u;
      }
      return;
    }

    if ( v1 < v2 )
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v1 )
          u += d1;
        else if ( u >= v2 )
          u += d2;
        else
          u = ref1->u + FT_MulDiv( u - v1, ref2->u - ref1->u, v2 - v1 );

        p->u = u;
      }
    }
    else
    {
      for ( p = p1; p <= p2; p++ )
      {
        u = p->v;

        if ( u <= v2 )
          u += d2;
        else if ( u >= v1 )
          u += d1;
        else
          u = ref1->u + FT_MulDiv( u - v1, ref2->u - ref1->u, v2 - v1 );

        p->u = u;
      }
    }
  }


  FT_LOCAL_DEF( void )
  af_glyph_hints_align_weak_points( AF_GlyphHints  hints,
                                    AF_Dimension   dim )
  {
    AF_Point   points        = hints->points;
    AF_Point   point_limit   = points + hints->num_points;
    AF_Point*  contour       = hints->contours;
    AF_Point*  contour_limit = contour + hints->num_contours;
    AF_Flags   touch_flag;
    AF_Point   point;
    AF_Point   end_point;
    AF_Point   first_point;


    /* PASS 1: Move segment points to edge positions */

    if ( dim == AF_DIMENSION_HORZ )
    {
      touch_flag = AF_FLAG_TOUCH_X;

      for ( point = points; point < point_limit; point++ )
      {
        point->u = point->x;
        point->v = point->ox;
      }
    }
    else
    {
      touch_flag = AF_FLAG_TOUCH_Y;

      for ( point = points; point < point_limit; point++ )
      {
        point->u = point->y;
        point->v = point->oy;
      }
    }

    point = points;

    for ( ; contour < contour_limit; contour++ )
    {
      point       = *contour;
      end_point   = point->prev;
      first_point = point;

      while ( point <= end_point && !( point->flags & touch_flag ) )
        point++;

      if ( point <= end_point )
      {
        AF_Point  first_touched = point;
        AF_Point  cur_touched   = point;


        point++;
        while ( point <= end_point )
        {
          if ( point->flags & touch_flag )
          {
            /* we found two successive touched points; we interpolate */
            /* all contour points between them                        */
            af_iup_interp( cur_touched + 1, point - 1,
                           cur_touched, point );
            cur_touched = point;
          }
          point++;
        }

        if ( cur_touched == first_touched )
        {
          /* this is a special case: only one point was touched in the */
          /* contour; we thus simply shift the whole contour           */
          af_iup_shift( first_point, end_point, cur_touched );
        }
        else
        {
          /* now interpolate after the last touched point to the end */
          /* of the contour                                          */
          af_iup_interp( cur_touched + 1, end_point,
                         cur_touched, first_touched );

          /* if the first contour point isn't touched, interpolate */
          /* from the contour start to the first touched point     */
          if ( first_touched > points )
            af_iup_interp( first_point, first_touched - 1,
                           cur_touched, first_touched );
        }
      }
    }

    /* now save the interpolated values back to x/y */
    if ( dim == AF_DIMENSION_HORZ )
    {
      for ( point = points; point < point_limit; point++ )
        point->x = point->u;
    }
    else
    {
      for ( point = points; point < point_limit; point++ )
        point->y = point->u;
    }
  }


#ifdef AF_USE_WARPER

  FT_LOCAL_DEF( void )
  af_glyph_hints_scale_dim( AF_GlyphHints  hints,
                            AF_Dimension   dim,
                            FT_Fixed       scale,
                            FT_Pos         delta )
  {
    AF_Point  points       = hints->points;
    AF_Point  points_limit = points + hints->num_points;
    AF_Point  point;
    

    if ( dim == AF_DIMENSION_HORZ )
    {
      for ( point = points; point < points_limit; point++ )
        point->x = FT_MulFix( point->fx, scale ) + delta;
    }
    else
    {
      for ( point = points; point < points_limit; point++ )
        point->y = FT_MulFix( point->fy, scale ) + delta;
    }
  }

#endif /* AF_USE_WARPER */

/* END */
