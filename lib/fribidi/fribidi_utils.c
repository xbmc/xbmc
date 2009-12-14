/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 1999,2000 Dov Grobgeld
 * 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this library, in a file named COPYING; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA  
 * 
 * For licensing issues, contact <dov@imagic.weizmann.ac.il>.
 */

/*======================================================================
 *  This file contains various utility functions that are commonly
 *  needed by programs that implement BiDi functionality. The more
 *  code that may be put here, the easier it is for the application
 *  writers.
 *----------------------------------------------------------------------*/

#include <stdlib.h>
#include "fribidi.h"
#include "fribidi_mem.h"

/*======================================================================
 *  The find_visual_ranges() function is used to convert between a
 *  continous span in either logical or visual space to a one, two, 
 *  three, ..., sixty-three discontinous spans in the other space.
 *  The function outputs the number of ranges needed to display the
 *  mapped range as well as the resolved ranges.
 *
 *  The variable is_v2l_map indicates whether the position map is
 *  is in the direction of visual-to-logical. This information is
 *  needed in order to look up the correct character from the
 *  embedding_level_list which is assumed to be in logical order.
 *
 *  This function is typically used to resolve a logical range to visual
 *  ranges e.g. to display the selection.
 *
 *  TBD: it does not support vis2log_map parameter yet, also it should
 *  merge the continous intervals found to one.
 *
 *  Example:
 *     The selection is between logical characters 10 to 45. Calculate
 *     the corresponding visual selection(s):
 *
 *     FriBidiStrIndex sel_span[2] = {10,45};
 *
 *     fribidi_map_range(sel_span,
 *                       FRIBIDI_TRUE,
 *                       length,
 *                       vis2log_map,
 *                       embedding_levels,
 *                       // output
 *                       &num_vis_ranges, *vis_ranges);
 **----------------------------------------------------------------------*/
FRIBIDI_API void
fribidi_map_range (FriBidiStrIndex in_span[2],	/* Start and end span */
		   FriBidiStrIndex len, fribidi_boolean is_v2l_map,	/* Needed for embedding_level */
		   FriBidiStrIndex *position_map,
		   FriBidiLevel *embedding_level_list,
		   /* output */
		   int *num_mapped_spans, FriBidiStrIndex mapped_spans[63][2])
{
  FriBidiStrIndex ch_idx;
  fribidi_boolean in_range = FRIBIDI_FALSE;
  FriBidiStrIndex start_idx = in_span[0];
  FriBidiStrIndex end_idx = in_span[1];

  if (start_idx == -1)
    start_idx = 0;

  if (end_idx == -1)
    end_idx = len;

  *num_mapped_spans = 0;

  /* This is a loop in the source space of the map... */
  for (ch_idx = 0; ch_idx <= len; ch_idx++)
    {
      FriBidiStrIndex mapped_pos;

      if (ch_idx < len)
	mapped_pos = position_map[ch_idx];
      else
	mapped_pos = -1;	/* Will cause log_pos < start_idx to trigger below */

      if (!in_range && mapped_pos >= start_idx && mapped_pos < end_idx)
	{
	  in_range = FRIBIDI_TRUE;
	  (*num_mapped_spans)++;
	  mapped_spans[(*num_mapped_spans) - 1][0] = ch_idx;
	}
      else if (in_range && (mapped_pos < start_idx || mapped_pos >= end_idx))
	{
	  mapped_spans[(*num_mapped_spans) - 1][1] = ch_idx;
	  in_range = FRIBIDI_FALSE;
	}
    }
}

/*======================================================================
 *  fribidi_find_string_changes() finds the bounding box of the section
 *  of characters that need redrawing. It returns the start and the
 *  length of the section in the new string that needs redrawing.
 *----------------------------------------------------------------------*/
FRIBIDI_API void
fribidi_find_string_changes (	/* input */
			      FriBidiChar *old_str,
			      FriBidiStrIndex old_len,
			      FriBidiChar *new_str, FriBidiStrIndex new_len,
			      /* output */
			      FriBidiStrIndex *change_start,
			      FriBidiStrIndex *change_len)
{
  FriBidiStrIndex i, num_bol, num_eol;

  /* Search forwards */
  i = 0;
  while (i < old_len && i < new_len && old_str[i] == new_str[i])
    i++;
  num_bol = i;

  /* Search backwards */
  i = 0;
  while (i < old_len
	 && i < new_len
	 && old_str[old_len - 1 - i] == new_str[new_len - 1 - i])
    i++;
  num_eol = i;

  /* Assign output */
  *change_start = num_bol;
  *change_len = new_len - num_eol - num_bol;
}

/*======================================================================
 *  fribidi_xpos_resolve() does the complicated translation of
 *  an x-coordinate, e.g. as received through a mouse press event,
 *  to the logical and the visual position the xcoordinate is closest
 *  to. It will also resolve the direction of the cursor according
 *  to the embedding level of the closest character.
 *
 *  It does this through the following logics:
 *  Here are the different possibilities:
 *
 *        Pointer              =>          Log Pos         Vis pos
 *  
 *     Before first vis char             log_pos(vis=0)L       0
 *     After last vis char               log_pos(vis=n-1)R     n
 *     Within 1/2 width of vis char i    log_pos(vis=i)L       i
 *     Within last 1/2 width of vchar i  log_pos(vis=i)R       i+1
 *     Border between vis chars i,i+1       resolve!           i+1
 *
 *  Input:
 *     x_pos        The pixel position to be resolved measured in pixels.
 *     x_offset     The x_offset is the pixel position of the left side
 *                  of the leftmost visual character. 
 *     len          The length of the embedding level, the vis2log and
 *                  the char width arrays.
 *     base_dir     The resolved base direction of the line.
 *     vis2log      The vis2log mapping.
 *                  x_position and the character widths. The position
 *                  (x_pos-x_offset) is number of pixels from the left
 *                  of logical character 0.
 *     char_widths  Width in pixels of each character. Note that the
 *                  widths should be provided in logical order.
 *
 *  Output:
 *     res_log_pos  Resolved logical position.
 *     res_vis_pos  Resolved visual position
 *     res_cursor_x_pos   The resolved pixel position to the left or
 *                  the right of the character position x_pos.
 *     res_cursor_dir_is_rtl   Whether the resolved dir of the character
 *                  at position x_pos is rtl.
 *     res_attach_before  Whether the x_pos is cutting the bounding
 *                  box in such a way that the visual cursor should be
 *                  be positioned before the following logical character.
 *                  Note that in the bidi context, the positions "after
 *                  a logical character" and "before the following logical
 *                  character" is not necessarily the same. If x_pos is
 *                  beyond the end of the line, res_attach_before is true.
 *
 *----------------------------------------------------------------------*/
FRIBIDI_API void
fribidi_xpos_resolve (int x_pos,
		      int x_offset,
		      FriBidiStrIndex len,
		      FriBidiLevel *embedding_level_list,
		      FriBidiCharType base_dir,
		      FriBidiStrIndex *vis2log, int *char_widths,
		      /* output */
		      FriBidiStrIndex *res_log_pos,
		      FriBidiStrIndex *res_vis_pos,
		      int *res_cursor_x_pos,
		      fribidi_boolean *res_cursor_dir_is_rtl,
		      fribidi_boolean *res_attach_before)
{
  int char_width_sum = 0;
  FriBidiStrIndex char_idx;

  char_width_sum = 0;
  *res_vis_pos = 0;

  /* Check if we are to the left of the line bounding box */
  if (x_pos < x_offset)
    {
      *res_cursor_dir_is_rtl = (base_dir == FRIBIDI_TYPE_RTL);
      if (*res_cursor_dir_is_rtl)
	*res_log_pos = len;
      else
	*res_log_pos = 0;

      *res_cursor_x_pos = x_offset;
      *res_vis_pos = 0;
      *res_attach_before = 1;
    }
  else
    {
      /* Find the cursor pos by a linear search on the row */
      for (char_idx = 0; char_idx < len; char_idx++)
	{
	  FriBidiStrIndex log_pos = vis2log[char_idx];
	  int char_width = char_widths[log_pos];

	  if (x_offset + char_width_sum + char_width > x_pos)
	    {
	      /* Found position */
	      *res_cursor_dir_is_rtl =
		fribidi_is_char_rtl (embedding_level_list, base_dir, log_pos);
	      /* Are we in the left hand side of the clicked character? */
	      if (x_pos - (x_offset + char_width_sum + char_width / 2) < 0)
		{
		  /* RTL? */
		  if (*res_cursor_dir_is_rtl)
		    {
		      log_pos++;
		      *res_attach_before = FRIBIDI_FALSE;
		    }
		  /* LTR */
		  else
		    *res_attach_before = FRIBIDI_TRUE;
		  *res_cursor_x_pos = x_offset + char_width_sum;
		}
	      /* We are in the right hand side. */
	      else
		{
		  /* LTR? */
		  if (!*res_cursor_dir_is_rtl)
		    {
		      log_pos++;
		      *res_attach_before = FRIBIDI_FALSE;
		    }
		  /* RTL */
		  else
		    *res_attach_before = FRIBIDI_TRUE;

		  *res_cursor_x_pos = x_offset + char_width_sum + char_width;
		  (*res_vis_pos)++;
		}
	      *res_log_pos = log_pos;
	      break;
	    }
	  char_width_sum += char_width;
	  (*res_vis_pos)++;
	}

      /* If we still haven't found the position we are to the left of the
         character bounding box */
      if (char_idx == len)
	{
	  *res_cursor_dir_is_rtl = (base_dir == FRIBIDI_TYPE_RTL);

	  if (*res_cursor_dir_is_rtl)
	    *res_log_pos = 0;
	  else
	    *res_log_pos = len;
	  *res_cursor_x_pos = char_width_sum + x_offset;
	  *res_vis_pos = len;
	  *res_attach_before = FRIBIDI_TRUE;
	}
    }

  /*  printf("x l,v = %d %d,%d\n", *res_cursor_x_pos, *res_log_pos, *res_vis_pos); */

}

/*======================================================================
 *  fribidi_is_char_rtl() answers the question whether a character
 *  was resolved in the rtl direction. This simply involves asking
 *  if the embedding level for the character is odd.
 *----------------------------------------------------------------------*/
FRIBIDI_API fribidi_boolean
fribidi_is_char_rtl (FriBidiLevel *embedding_level_list,
		     FriBidiCharType base_dir, FriBidiStrIndex idx)
{
  if (!embedding_level_list || idx < 0)
    return FRIBIDI_IS_RTL (base_dir);
  /* Otherwise check if the embedding level is odd */
  else
    return embedding_level_list[idx] % 2;
}

/*======================================================================
 *  fribidi_runs_log2vis takes a list of logical runs and returns a
 *  a list of visual runs. A run is defined as a sequence that has
 *  the same attributes.
 *----------------------------------------------------------------------*/
FRIBIDI_API void
fribidi_runs_log2vis (		/* input */
		       FriBidiList *logical_runs,	/* List of FriBidiRunType */
		       FriBidiStrIndex len, FriBidiStrIndex *log2vis, FriBidiCharType base_dir,	/* TBD: remove it, not needed */
		       /* output */
		       FriBidiList **visual_runs)
{
  void **visual_attribs = (void **) malloc (sizeof (void *) * len);
  void *current_attrib;
  FriBidiStrIndex pos, i;
  FriBidiList *list, *last;
  FriBidiStrIndex current_idx;


  /* 1. Open up the runlength encoded list and at the same time apply
     the log2vis map. The result is a visual array of attributes.
   */
  pos = 0;
  list = logical_runs;
  while (list)
    {
      FriBidiRunType *run = (FriBidiRunType *) (list->data);
      FriBidiStrIndex length = run->length;
      void *attrib = run->attribute;

      for (i = pos; i < pos + length; i++)
	visual_attribs[log2vis[i]] = attrib;
      list = list->next;
    }

  /* 2. Run length encode the resulting attributes. */
  *visual_runs = last = NULL;
  current_attrib = visual_attribs[0];
  current_idx = 0;
  for (i = 0; i <= len; i++)
    {
      if (i == len || current_attrib != visual_attribs[i])
	{
	  FriBidiRunType *run =
	    (FriBidiRunType *) malloc (sizeof (FriBidiRunType));
	  run->length = i - current_idx;
	  run->attribute = current_attrib;

	  /* Keeping track of the last node is crucial for efficiency
	     for long lists... */
	  if (last == NULL)
	    last = *visual_runs = fribidi_list_append (NULL, run);
	  else
	    {
	      fribidi_list_append (last, run);
	      last = last->next;
	    }
	  if (i == len)
	    break;

	  current_attrib = visual_attribs[i];
	  current_idx = i;
	}
    }
  free (visual_attribs);
}
