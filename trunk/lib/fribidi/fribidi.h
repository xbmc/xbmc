/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 1999,2000 Dov Grobgeld, and
 * Copyright (C) 2001,2002 Behdad Esfahbod. 
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
 * For licensing issues, contact <dov@imagic.weizmann.ac.il> and 
 * <fwpg@sharif.edu>. 
 */

#ifndef FRIBIDI_H
#define FRIBIDI_H

#ifndef NULL
#define NULL 0
#endif

#include "fribidi_config.h"
#include "fribidi_unicode.h"
#include "fribidi_types.h"
#ifndef FRIBIDI_NO_CHARSETS
#include "fribidi_char_sets.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

  FRIBIDI_API fribidi_boolean fribidi_log2vis (	/* input */
						FriBidiChar *str,
						FriBidiStrIndex len,
						FriBidiCharType *pbase_dirs,
						/* output */
						FriBidiChar *visual_str,
						FriBidiStrIndex
						*position_L_to_V_list,
						FriBidiStrIndex
						*position_V_to_L_list,
						FriBidiLevel
						*embedding_level_list);

  FRIBIDI_API fribidi_boolean fribidi_log2vis_get_embedding_levels (	/* input */
								     FriBidiChar
								     *str,
								     FriBidiStrIndex
								     len,
								     FriBidiCharType
								     *pbase_dir,
								     /* output */
								     FriBidiLevel
								     *embedding_level_list);

/*======================================================================
 *  fribidi_remove_bidi_marks() removes bidirectional marks, and returns
 *  the new length, also updates each of other inputs if not NULL.
 *----------------------------------------------------------------------*/
  FRIBIDI_API FriBidiStrIndex fribidi_remove_bidi_marks (FriBidiChar *str,
							 FriBidiStrIndex
							 length,
							 FriBidiStrIndex
							 *position_to_this_list,
							 FriBidiStrIndex
							 *position_from_this_list,
							 FriBidiLevel
							 *embedding_level_list);

/*======================================================================
 *  fribidi_get_type() returns bidi type of a character.
 *----------------------------------------------------------------------*/
  FRIBIDI_API FriBidiCharType fribidi_get_type (FriBidiChar uch);

/*======================================================================
 *  fribidi_get_types() returns bidi type of a string.
 *----------------------------------------------------------------------*/
  FRIBIDI_API void fribidi_get_types (	/* input */
				       FriBidiChar *str,
				       FriBidiStrIndex len,
				       /* output */
				       FriBidiCharType *type);

/*======================================================================
 *  fribidi_get_mirror_char() returns the mirrored character, if input
 *  character has a mirror, or the input itself.
 *  if mirrored_ch is NULL, just returns if character has a mirror or not.
 *----------------------------------------------------------------------*/
  FRIBIDI_API fribidi_boolean fribidi_get_mirror_char (	/* Input */
							FriBidiChar ch,
							/* Output */
							FriBidiChar
							*mirrored_ch);

/*======================================================================
 *  fribidi_mirroring_status() returns whether mirroring is on or off,
 *  default is on.
 *----------------------------------------------------------------------*/
  FRIBIDI_API fribidi_boolean fribidi_mirroring_status (void);

/*======================================================================
 *  fribidi_set_mirroring() sets mirroring on or off.
 *----------------------------------------------------------------------*/
  FRIBIDI_API void fribidi_set_mirroring (fribidi_boolean mirror);

/*======================================================================
 *  fribidi_reorder_nsm_status() returns whether reordering of NSM
 *  sequences is on or off, default is off.
 *----------------------------------------------------------------------*/
  FRIBIDI_API fribidi_boolean fribidi_reorder_nsm_status (void);

/*======================================================================
 *  fribidi_set_reorder_nsm() sets reordering of NSM characters on or off.
 *----------------------------------------------------------------------*/
  FRIBIDI_API void fribidi_set_reorder_nsm (fribidi_boolean);

/*======================================================================
 *  fribidi_set_debug() turn on or off debugging, default is off, return
 *  false is fribidi is not compiled with debug enabled.
 *----------------------------------------------------------------------*/
  FRIBIDI_API fribidi_boolean fribidi_set_debug (fribidi_boolean debug);

/* fribidi_utils.c */

/*======================================================================
 *  fribidi_find_string_changes() finds the bounding box of the section
 *  of characters that need redrawing. It returns the start and the
 *  length of the section in the new string that needs redrawing.
 *----------------------------------------------------------------------*/
  FRIBIDI_API void fribidi_find_string_changes (	/* input */
						 FriBidiChar *old_str,
						 FriBidiStrIndex old_len,
						 FriBidiChar *new_str,
						 FriBidiStrIndex new_len,
						 /* output */
						 FriBidiStrIndex
						 *change_start,
						 FriBidiStrIndex *change_len);


/*======================================================================
 *  The find_visual_ranges() function is used to convert between a
 *  continous span in either logical or visual space to a one, two or
 *  three discontinous spans in the other space. The function outputs
 *  the number of ranges needed to display the mapped range as
 *  well as the resolved ranges.
 *
 *  The variable is_v2l_map indicates whether the position map is
 *  is in the direction of visual-to-logical. This information is
 *  needed in order to look up the correct character from the
 *  embedding_level_list which is assumed to be in logical order.
 *
 *  This function is typically used to resolve a logical range to visual
 *  ranges e.g. to display the selection.
 *
 *  Example:
 *     The selection is between logical characters 10 to 45. Calculate
 *     the corresponding visual selection(s):
 *
 *     FriBidiStrIndex sel_span[2] = {10,45};
 *
 *     fribidi_map_range(sel_span,
 *                       TRUE,
 *                       length,
 *                       vis2log_map,
 *                       embedding_levels,
 *                       // output
 *                       &num_vis_ranges, *vis_ranges);
 **----------------------------------------------------------------------*/
  FRIBIDI_API void fribidi_map_range (FriBidiStrIndex span[2],
				      FriBidiStrIndex len,
				      fribidi_boolean is_v2l_map,
				      FriBidiStrIndex *position_map,
				      FriBidiLevel *embedding_level_list,
				      /* output */
				      int *num_mapped_spans,
				      FriBidiStrIndex spans[3][2]);

/*======================================================================
 *  fribidi_is_char_rtl() answers the question whether a character
 *  was resolved in the rtl direction. This simply involves asking
 *  if the embedding level for the character is odd.
 *----------------------------------------------------------------------*/
  FRIBIDI_API fribidi_boolean fribidi_is_char_rtl (FriBidiLevel
						   *embedding_level_list,
						   FriBidiCharType base_dir,
						   FriBidiStrIndex idx);

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
  FRIBIDI_API void fribidi_xpos_resolve (int x_pos,
					 int x_offset,
					 FriBidiStrIndex len,
					 FriBidiLevel *embedding_level_list,
					 FriBidiCharType base_dir,
					 FriBidiStrIndex *vis2log,
					 int *char_widths,
					 /* output */
					 FriBidiStrIndex *res_log_pos,
					 FriBidiStrIndex *res_vis_pos,
					 int *res_cursor_x_pos,
					 fribidi_boolean
					 *res_cursor_dir_is_rtl,
					 fribidi_boolean *res_attach_before);

/*======================================================================
 *  fribidi_runs_log2vis takes a list of logical runs and returns a
 *  a list of visual runs. A run is defined as a sequence that has
 *  the same attributes.
 *----------------------------------------------------------------------*/
  FRIBIDI_API void fribidi_runs_log2vis (	/* input */
					  FriBidiList *logical_runs,	/* List of FriBidiRunType */

					  FriBidiStrIndex len,
					  FriBidiStrIndex *log2vis,
					  FriBidiCharType base_dir,
					  /* output */
					  FriBidiList **visual_runs);


#ifdef	__cplusplus
}
#endif

#endif				/* FRIBIDI_H */
