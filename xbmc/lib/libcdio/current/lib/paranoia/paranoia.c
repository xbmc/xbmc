/*
  $Id: paranoia.c,v 1.26 2006/03/18 18:37:56 rocky Exp $

  Copyright (C) 2004, 2005, 2006 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/***
 * Toplevel file for the paranoia abstraction over the cdda lib 
 *
 ***/

/* immediate todo:: */
/* Allow disabling of root fixups? */ 
/* Dupe bytes are creeping into cases that require greater overlap
   than a single fragment can provide.  We need to check against a
   larger area* (+/-32 sectors of root?) to better eliminate
   dupes. Of course this leads to other problems... Is it actually a
   practically solvable problem? */
/* Bimodal overlap distributions break us. */
/* scratch detection/tolerance not implemented yet */

/***************************************************************

  Da new shtick: verification now a two-step assymetric process.
  
  A single 'verified/reconstructed' data segment cache, and then the
  multiple fragment cache 

  verify a newly read block against previous blocks; do it only this
  once. We maintain a list of 'verified sections' from these matches.

  We then glom these verified areas into a new data buffer.
  Defragmentation fixups are allowed here alone.

  We also now track where read boundaries actually happened; do not
  verify across matching boundaries.

  **************************************************************/

/***************************************************************

  Silence.  "It's BAAAAAAaaack."

  audio is now treated as great continents of values floating on a
  mantle of molten silence.  Silence is not handled by basic
  verification at all; we simply anchor sections of nonzero audio to a
  position and fill in everything else as silence.  We also note the
  audio that interfaces with silence; an edge must be 'wet'.

  **************************************************************/

/* ===========================================================================
 * Let's translate the above vivid metaphor into something a mere mortal
 * can understand:
 *
 * Non-silent audio is "solid."  Silent audio is "wet" and fluid.  The reason
 * to treat silence as fluid is that if there's a long enough span of
 * silence, we can't reliably detect jitter or dropped samples within that
 * span (since all silence looks alike).  Non-silent audio, on the other
 * hand, is distinctive and can be reliably reassembled.
 *
 * So we treat long spans of silence specially.  We only consider an edge
 * of a non-silent region ("continent" or "island") to be "wet" if it borders
 * a long span of silence.  Short spans of silence are merely damp and can
 * be reliably placed within a continent.
 *
 * We position ("anchor") the non-silent regions somewhat arbitrarily (since
 * they may be jittered and we have no way to verify their exact position),
 * and fill the intervening space with silence.
 *
 * See i_silence_match() for the gory details.
 * ===========================================================================
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <unistd.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <math.h>
#include <cdio/cdda.h>
#include "../cdda_interface/smallft.h"
#include "p_block.h"
#include <cdio/paranoia.h>
#include "overlap.h"
#include "gap.h"
#include "isort.h"

const char *paranoia_cb_mode2str[] = {
  "read",
  "verify",
  "fixup edge",
  "fixup atom",
  "scratch",
  "repair",
  "skip",
  "drift",
  "backoff",
  "overlap",
  "fixup dropped",
  "fixup duplicated",
  "read error"
};

/** The below variables are trickery to force the above enum symbol
    values to be recorded in debug symbol tables. They are used to
    allow one to refer to the enumeration value names in the typedefs
    above in a debugger and debugger expressions
*/

paranoia_mode_t    debug_paranoia_mode;
paranoia_cb_mode_t debug_paranoia_cb_mode;

static inline long 
re(root_block *root)
{
  if (!root)return(-1);
  if (!root->vector)return(-1);
  return(ce(root->vector));
}

static inline long 
rb(root_block *root)
{
  if (!root)return(-1);
  if (!root->vector)return(-1);
  return(cb(root->vector));
}

static inline 
long rs(root_block *root)
{
  if (!root)return(-1);
  if (!root->vector)return(-1);
  return(cs(root->vector));
}

static inline int16_t *
rv(root_block *root){
  if (!root)return(NULL);
  if (!root->vector)return(NULL);
  return(cv(root->vector));
}

#define rc(r) (r->vector)

/** 
    Flags indicating the status of a read samples.

    Imagine the below enumeration values are #defines to be used in a
    bitmask rather than distinct values of an enum.

    The variable part of the declaration is trickery to force the enum
    symbol values to be recorded in debug symbol tables. They are used
    to allow one refer to the enumeration value names in a debugger
    and in debugger expressions.
*/
enum  {
  FLAGS_EDGE    =0x1, /**< first/last N words of frame */
  FLAGS_UNREAD  =0x2, /**< unread, hence missing and unmatchable */
  FLAGS_VERIFIED=0x4  /**< block read and verified */
} paranoia_read_flags;


/**** matching and analysis code *****************************************/

/* ===========================================================================
 * i_paranoia_overlap() (internal)
 *
 * This function is called when buffA[offsetA] == buffB[offsetB].  This
 * function searches backward and forward to see how many consecutive
 * samples also match.
 *
 * This function is called by do_const_sync() when we're not doing any
 * verification.  Its more complicated sibling is i_paranoia_overlap2.
 *
 * This function returns the number of consecutive matching samples.
 * If (ret_begin) or (ret_end) are not NULL, it fills them with the
 * offsets of the first and last matching samples in A.
 */
static inline long 
i_paranoia_overlap(int16_t *buffA,int16_t *buffB,
		   long offsetA, long offsetB,
		   long sizeA,long sizeB,
		   long *ret_begin, long *ret_end)
{
  long beginA=offsetA,endA=offsetA;
  long beginB=offsetB,endB=offsetB;

  /* Scan backward to extend the matching run in that direction. */
  for(; beginA>=0 && beginB>=0; beginA--,beginB--)
    if (buffA[beginA] != buffB[beginB]) break;
  beginA++;
  beginB++;

  /* Scan forward to extend the matching run in that direction. */
  for(; endA<sizeA && endB<sizeB; endA++,endB++)
    if (buffA[endA] != buffB[endB]) break;

  /* Return the result of our search. */
  if (ret_begin) *ret_begin = beginA;
  if (ret_end) *ret_end = endA;
  return (endA-beginA);
}


/* ===========================================================================
 * i_paranoia_overlap2() (internal)
 *
 * This function is called when buffA[offsetA] == buffB[offsetB].  This
 * function searches backward and forward to see how many consecutive
 * samples also match.
 *
 * This function is called by do_const_sync() when we're verifying the
 * data coming off the CD.  Its less complicated sibling is
 * i_paranoia_overlap, which is a good place to look to see the simplest
 * outline of how this function works.
 *
 * This function returns the number of consecutive matching samples.
 * If (ret_begin) or (ret_end) are not NULL, it fills them with the
 * offsets of the first and last matching samples in A.
 */
static inline long 
i_paranoia_overlap2(int16_t *buffA,int16_t *buffB,
		    unsigned char *flagsA, unsigned char *flagsB,
		    long offsetA, long offsetB,
		    long sizeA,long sizeB,
		    long *ret_begin, long *ret_end)
{
  long beginA=offsetA, endA=offsetA;
  long beginB=offsetB, endB=offsetB;
  
  /* Scan backward to extend the matching run in that direction. */
  for (; beginA>=0 && beginB>=0; beginA--,beginB--) {
    if (buffA[beginA] != buffB[beginB]) break;

    /* don't allow matching across matching sector boundaries */
    /* Stop if both samples were at the edges of a low-level read.
     * ???: What implications does this have?
     * ???: Why do we include the first sample for which this is true?
     */
    if ((flagsA[beginA]&flagsB[beginB]&FLAGS_EDGE)) {
      beginA--;
      beginB--;
      break;
    }

    /* don't allow matching through known missing data */
    if ((flagsA[beginA]&FLAGS_UNREAD) || (flagsB[beginB]&FLAGS_UNREAD))
      break;
  }
  beginA++;
  beginB++;
  
  /* Scan forward to extend the matching run in that direction. */
  for (; endA<sizeA && endB<sizeB; endA++,endB++) {
    if (buffA[endA] != buffB[endB]) break;

    /* don't allow matching across matching sector boundaries */
    /* Stop if both samples were at the edges of a low-level read.
     * ???: What implications does this have?
     * ???: Why do we not stop if endA == beginA?
     */
    if ((flagsA[endA]&flagsB[endB]&FLAGS_EDGE) && endA!=beginA){
      break;
    }

    /* don't allow matching through known missing data */
    if ((flagsA[endA]&FLAGS_UNREAD) || (flagsB[endB]&FLAGS_UNREAD))
      break;
  }

  /* Return the result of our search. */
  if (ret_begin) *ret_begin = beginA;
  if (ret_end) *ret_end = endA;
  return (endA-beginA);
}


/* ===========================================================================
 * do_const_sync() (internal)
 *
 * This function is called when samples A[posA] == B[posB].  It tries to
 * build a matching run from that point, looking forward and backward to
 * see how many consecutive samples match.  Since the starting samples
 * might only be coincidentally identical, we only consider the run to
 * be a true match if it's longer than MIN_WORDS_SEARCH.
 *
 * This function returns the length of the run if a matching run was found,
 * or 0 otherwise.  If a matching run was found, (begin) and (end) are set
 * to the absolute positions of the beginning and ending samples of the
 * run in A, and (offset) is set to the jitter between the c_blocks.
 * (I.e., offset indicates the distance between what A considers sample N
 * on the CD and what B considers sample N.)
 */
static inline long int 
do_const_sync(c_block_t *A,
	      sort_info_t *B, unsigned char *flagB,
	      long posA, long posB,
	      long *begin, long *end, long *offset)
{
  unsigned char *flagA=A->flags;
  long ret=0;

  /* If we're doing any verification whatsoever, we have flags in stage
   * 1, and will take them into account.  Otherwise (e.g. in stage 2),
   * we just do the simple equality test for samples on both sides of
   * the initial match.
   */
  if (flagB==NULL)
    ret=i_paranoia_overlap(cv(A), iv(B), posA, posB,
			   cs(A), is(B), begin, end);
  else
    if ((flagB[posB]&FLAGS_UNREAD)==0)
      ret=i_paranoia_overlap2(cv(A), iv(B), flagA, flagB, 
			      posA, posB, cs(A), is(B),
			      begin, end);
	
  /* Small matching runs could just be coincidental.  We only consider this
   * a real match if it's long enough.
   */
  if (ret > MIN_WORDS_SEARCH) {
    *offset=+(posA+cb(A))-(posB+ib(B));

    /* Note that try_sort_sync()'s swaps A & B when it calls this function,
     * so while we adjust begin & end to be relative to A here, that means
     * it's relative to B in try_sort_sync().
     */
    *begin+=cb(A);
    *end+=cb(A);
    return(ret);
  }
  
  return(0);
}


/* ===========================================================================
 * try_sort_sync() (internal)
 *
 * Starting from the sample in B with the absolute position (post), look
 * for a matching run in A.  This search will look in A for a first
 * matching sample within (p->dynoverlap) samples around (post).  If it
 * finds one, it will then determine how many consecutive samples match
 * both A and B from that point, looking backwards and forwards.  If
 * this search produces a matching run longer than MIN_WORDS_SEARCH, we
 * consider it a match.
 *
 * When used by stage 1, the "post" is planted with respect to the old
 * c_block being compare to the new c_block.  In stage 2, the "post" is
 * planted with respect to the verified root.
 *
 * This function returns 1 if a match is found and 0 if not.  When a match
 * is found, (begin) and (end) are set to the boundaries of the run, and
 * (offset) is set to the difference in position of the run in A and B.
 * (begin) and (end) are the absolute positions of the samples in
 * B.  (offset) transforms A to B's frame of reference.  I.e., an offset of
 * 2 would mean that A's absolute 3 is equivalent to B's 5.
 */

/* post is w.r.t. B.  in stage one, we post from old.  In stage 2 we
   post from root. Begin, end, offset count from B's frame of
   reference */

static inline long int 
try_sort_sync(cdrom_paranoia_t *p,
	      sort_info_t *A, unsigned char *Aflags,
	      c_block_t *B,
	      long int post,
	      long int *begin,
	      long int *end,
	      long *offset,
	      void (*callback)(long int, paranoia_cb_mode_t))
{
  
  long int dynoverlap=p->dynoverlap;
  sort_link_t *ptr=NULL;
  unsigned char *Bflags=B->flags;

  /* block flag matches FLAGS_UNREAD (and hence unmatchable) */
  if (Bflags==NULL || (Bflags[post-cb(B)]&FLAGS_UNREAD)==0){
    /* always try absolute offset zero first! */
    {
      long zeropos=post-ib(A);
      if (zeropos>=0 && zeropos<is(A)) {

	/* Before we bother with the search for a matching samples,
	 * we check the simple case.  If there's no jitter at all
	 * (i.e. the absolute positions of A's and B's samples are
	 * consistent), A's sample at (post) should be identical
	 * to B's sample at the same position.
	 */
	if ( cv(B)[post-cb(B)] == iv(A)[zeropos] ) {

	  /* The first sample matched, now try to grow the matching run
	   * in both directions.  We only consider it a match if more
	   * than MIN_WORDS_SEARCH consecutive samples match.
	   */
	  if (do_const_sync(B, A, Aflags,
			    post-cb(B), zeropos,
			    begin, end, offset) ) {

	    /* ???BUG??? Jitter cannot be accurately detected when there are
	     * large regions of silence.  Silence all looks alike, so if
	     * there is actually jitter but lots of silence, jitter (offset)
	     * will be incorrectly identified as 0.  When the incorrect zero
	     * jitter is passed to offset_add_value, it eventually reduces
	     * dynoverlap so much that it's impossible for stage 2 to merge
	     * jittered fragments into the root (it doesn't search far enough).
	     *
	     * A potential solution (tested, but not committed) is to check
	     * for silence in do_const_sync and simply not call
	     * offset_add_value if the match is all silence.
	     *
	     * This bug is not fixed yet.
	     */
	    /* ???: To be studied. */
	    offset_add_value(p,&(p->stage1),*offset,callback);
	    
	    return(1);
	  }
	}
      }
    }
  } else
    return(0);

  /* If the samples with the same absolute position didn't match, it's
   * either a bad sample, or the two c_blocks are jittered with respect
   * to each other.  Now we search through A for samples that do have
   * the same value as B's post.  The search looks from first to last
   * occurrence witin (dynoverlap) samples of (post).
   */
  ptr=sort_getmatch(A,post-ib(A),dynoverlap,cv(B)[post-cb(B)]);
  
  while (ptr){

    /* We've found a matching sample, so try to grow the matching run in
     * both directions.  If we find a long enough run (longer than
     * MIN_WORDS_SEARCH), we've found a match.
     */
    if (do_const_sync(B,A,Aflags,
		     post-cb(B),ipos(A,ptr),
		     begin,end,offset)){

      /* ???BUG??? Jitter cannot be accurately detected when there are
       * large regions of silence.  Silence all looks alike, so if
       * there is actually jitter but lots of silence, jitter (offset)
       * will be incorrectly identified as 0.  When the incorrect zero
       * jitter is passed to offset_add_value, it eventually reduces
       * dynoverlap so much that it's impossible for stage 2 to merge
       * jittered fragments into the root (it doesn't search far enough).
       *
       * A potential solution (tested, but not committed) is to check
       * for silence in do_const_sync and simply not call
       * offset_add_value if the match is all silence.
       *
       * This bug is not fixed yet.
       */
      /* ???: To be studied. */
      offset_add_value(p,&(p->stage1),*offset,callback);
      return(1);
    }

    /* The matching sample was just a fluke -- there weren't enough adjacent
     * samples that matched to consider a matching run.  So now we check
     * for the next occurrence of that value in A.
     */
    ptr=sort_nextmatch(A,ptr);
  }

  /* We didn't find any matches. */
  *begin=-1;
  *end=-1;
  *offset=-1;
  return(0);
}


/* ===========================================================================
 * STAGE 1 MATCHING
 *
 * ???: Insert high-level explanation here.
 * ===========================================================================
 */

/* Top level of the first stage matcher */

/* We match each analysis point of new to the preexisting blocks
recursively.  We can also optionally maintain a list of fragments of
the preexisting block that didn't match anything, and match them back
afterward. */

#define OVERLAP_ADJ (MIN_WORDS_OVERLAP/2-1)


/* ===========================================================================
 * stage1_matched() (internal)
 *
 * This function is called whenever stage 1 verification finds two identical
 * runs of samples from different reads.  The runs must be more than
 * MIN_WORDS_SEARCH samples long.  They may be jittered (i.e. their absolute
 * positions on the CD may not match due to inaccurate seeking) with respect
 * to each other, but they have been verified to have no dropped samples
 * within them.
 *
 * This function provides feedback via the callback mechanism and marks the
 * runs as verified.  The details of the marking are somehwat subtle and
 * are described near the relevant code.
 *
 * Subsequent portions of the stage 1 code will build a verified fragment
 * from this run.  The verified fragment will eventually be merged
 * into the verified root (and its absolute position determined) in
 * stage 2.
 */
static inline void 
stage1_matched(c_block_t *old, c_block_t *new,
	       long matchbegin,long matchend,
	       long matchoffset,
	       void (*callback)(long int, paranoia_cb_mode_t))
{
  long i;
  long oldadjbegin=matchbegin-cb(old);
  long oldadjend=matchend-cb(old);
  long newadjbegin=matchbegin-matchoffset-cb(new);
  long newadjend=matchend-matchoffset-cb(new);


  /* Provide feedback via the callback about the samples we've just
   * verified.
   *
   * ???: How can matchbegin ever be < cb(old)?
   *
   * ???: Why do edge samples get logged only when there's jitter
   * between the matched runs (matchoffset != 0)?
   */
  if ( matchbegin-matchoffset<=cb(new)
       || matchbegin<=cb(old)
       || (new->flags[newadjbegin]&FLAGS_EDGE) 
       || (old->flags[oldadjbegin]&FLAGS_EDGE) ) {
    if ( matchoffset && callback )
	(*callback)(matchbegin,PARANOIA_CB_FIXUP_EDGE);
  } else
    if (callback)
      (*callback)(matchbegin,PARANOIA_CB_FIXUP_ATOM);
  
  if ( matchend-matchoffset>=ce(new) ||
       (new->flags[newadjend]&FLAGS_EDGE) ||
       matchend>=ce(old) ||
       (old->flags[oldadjend]&FLAGS_EDGE) ) {
    if ( matchoffset && callback )
      (*callback)(matchend,PARANOIA_CB_FIXUP_EDGE);
  } else
    if (callback) 
      (*callback)(matchend, PARANOIA_CB_FIXUP_ATOM);


#if TRACE_PARANOIA & 1
  fprintf(stderr, "-   Matched [%ld-%ld] against [%ld-%ld]\n",
	  newadjbegin+cb(new), newadjend+cb(new),
	  oldadjbegin+cb(old), oldadjend+cb(old));
#endif

  /* Mark verified samples as "verified," but trim the verified region
   * by OVERLAP_ADJ samples on each side.  There are several significant
   * implications of this trimming:
   *
   * 1) Why we trim at all:  We have to trim to distinguish between two
   * adjacent verified runs and one long verified run.  We encounter this
   * situation when samples have been dropped:
   *
   *   matched portion of read 1 ....)(.... matched portion of read 1
   *       read 2 adjacent run  .....)(..... read 2 adjacent run
   *                                 ||
   *                      dropped samples in read 2
   *
   * So at this point, the fact that we have two adjacent runs means
   * that we have not yet verified that the two runs really are adjacent.
   * (In fact, just the opposite:  there are two runs because they were
   * matched by separate runs, indicating that some samples didn't match
   * across the length of read 2.)
   *
   * If we verify that they are actually adjacent (e.g. if the two runs
   * are simply a result of matching runs from different reads, not from
   * dropped samples), we will indeed mark them as one long merged run.
   *
   * 2) Why we trim by this amount: We want to ensure that when we
   * verify the relationship between these two runs, we do so with
   * an overlapping fragment at least OVERLAP samples long.  Following
   * from the above example:
   *
   *                (..... matched portion of read 3 .....)
   *       read 2 adjacent run  .....)(..... read 2 adjacent run
   *
   * Assuming there were no dropped samples between the adjacent runs,
   * the matching portion of read 3 will need to be at least OVERLAP
   * samples long to mark the two runs as one long verified run.
   * If there were dropped samples, read 3 wouldn't match across the
   * two runs, proving our caution worthwhile.
   *
   * 3) Why we partially discard the work we've done:  We don't.
   * When subsequently creating verified fragments from this run,
   * we compensate for this trimming.  Thus the verified fragment will
   * contain the full length of verified samples.  Only the c_blocks
   * will reflect this trimming.
   *
   * ???: The comment below indicates that the sort cache is updated in
   * some way, but this does not appear to be the case.
   */

  /* Mark the verification flags.  Don't mark the first or
     last OVERLAP/2 elements so that overlapping fragments
     have to overlap by OVERLAP to actually merge. We also
     remove elements from the sort such that later sorts do
     not have to sift through already matched data */

  newadjbegin+=OVERLAP_ADJ;
  newadjend-=OVERLAP_ADJ;
  for(i=newadjbegin;i<newadjend;i++)
    new->flags[i]|=FLAGS_VERIFIED; /* mark verified */

  oldadjbegin+=OVERLAP_ADJ;
  oldadjend-=OVERLAP_ADJ;
  for(i=oldadjbegin;i<oldadjend;i++)
    old->flags[i]|=FLAGS_VERIFIED; /* mark verified */
}


/* ===========================================================================
 * i_iterate_stage1 (internal)
 *
 * This function is called by i_stage1() to compare newly read samples with
 * previously read samples, searching for contiguous runs of identical
 * samples.  Matching runs indicate that at least two reads of the CD
 * returned identical data, with no dropped samples in that run.
 * The runs may be jittered (i.e. their absolute positions on the CD may
 * not be accurate due to inaccurate seeking) at this point.  Their
 * positions will be determined in stage 2.
 *
 * This function compares the new c_block (which has been indexed in
 * p->sortcache) to a previous c_block.  It is called for each previous
 * c_block.  It searches for runs of identical samples longer than
 * MIN_WORDS_SEARCH.  Samples in matched runs are marked as verified.
 *
 * Subsequent stage 1 code builds verified fragments from the runs of
 * verified samples.  These fragments are merged into the verified root
 * in stage 2.
 *
 * This function returns the number of distinct runs verified in the new
 * c_block when compared against this old c_block.
 */
static long int 
i_iterate_stage1(cdrom_paranoia_t *p, c_block_t *old, c_block_t *new,
		 void(*callback)(long int, paranoia_cb_mode_t)) 
{
  long matchbegin = -1;
  long matchend   = -1;
  long matchoffset;

  /* ???: Why do we limit our search only to the samples with overlapping
   * absolute positions?  It could be because it eliminates some further
   * bounds checking.
   *
   * Why do we "no longer try to spread the ... search" as mentioned below?
   */
  /* we no longer try to spread the stage one search area by dynoverlap */
  long searchend   = min(ce(old), ce(new));
  long searchbegin = max(cb(old), cb(new));
  long searchsize  = searchend-searchbegin;
  sort_info_t *i = p->sortcache;
  long ret = 0;
  long int j;

  long tried = 0;
  long matched = 0;

  if (searchsize<=0)
    return(0);

  /* match return values are in terms of the new vector, not old */

  /* ???: Why 23?  */

  for (j=searchbegin; j<searchend; j+=23) {

    /* Skip past any samples verified in previous comparisons to
     * other old c_blocks.  Also, obviously, don't bother verifying
     * unread/unmatchable samples.
     */
    if ((new->flags[j-cb(new)] & (FLAGS_VERIFIED|FLAGS_UNREAD)) == 0) {
      tried++;

      /* Starting from the sample in the old c_block with the absolute
       * position j, look for a matching run in the new c_block.  This
       * search will look a certain distance around j, and if successful
       * will extend the matching run as far backward and forward as
       * it can.
       *
       * The search will only return 1 if it finds a matching run long
       * enough to be deemed significant.
       */
      if (try_sort_sync(p, i, new->flags, old, j,
			&matchbegin, &matchend, &matchoffset,
			callback) == 1) {

	matched+=matchend-matchbegin;

	/* purely cosmetic: if we're matching zeros, don't use the
           callback because they will appear to be all skewed */
	{
	  long j = matchbegin-cb(old);
	  long end = matchend-cb(old);
	  for (; j<end; j++) if (cv(old)[j]!=0) break;

	  /* Mark the matched samples in both c_blocks as verified.
	   * In reality, not all the samples are marked.  See
	   * stage1_matched() for details.
	   */
	  if (j<end) {
	    stage1_matched(old,new,matchbegin,matchend,matchoffset,callback);
	  } else {
	    stage1_matched(old,new,matchbegin,matchend,matchoffset,NULL);
	  }
	}
	ret++;

	/* Skip past this verified run to look for more matches. */
	if (matchend-1 > j)
	  j = matchend-1;
      }
    }
  } /* end for */

#ifdef NOISY 
  fprintf(stderr,"iterate_stage1: search area=%ld[%ld-%ld] tried=%ld matched=%ld spans=%ld\n",
	  searchsize,searchbegin,searchend,tried,matched,ret);
#endif

  return(ret);
}


/* ===========================================================================
 * i_stage1() (internal)
 *
 * Compare newly read samples against previously read samples, searching
 * for contiguous runs of identical samples.  Matching runs indicate that
 * at least two reads of the CD returned identical data, with no dropped
 * samples in that run.  The runs may be jittered (i.e. their absolute
 * positions on the CD may not be accurate due to inaccurate seeking) at
 * this point.  Their positions will be determined in stage 2.
 *
 * This function compares a new c_block against all other c_blocks in memory,
 * searching for sufficiently long runs of identical samples.  Since each
 * c_block represents a separate call to read_c_block, this ensures that
 * multiple reads have returned identical data.  (Additionally, read_c_block
 * varies the reads so that multiple reads are unlikely to produce identical
 * errors, so any matches between reads are considered verified.  See
 * i_read_c_block for more details.)
 *
 * Each time we find such a  run (longer than MIN_WORDS_SEARCH), we mark
 * the samples as "verified" in both c_blocks.  Runs of verified samples in
 * the new c_block are promoted into verified fragments, which will later
 * be merged into the verified root in stage 2.
 *
 * In reality, not all the verified samples are marked as "verified."
 * See stage1_matched() for an explanation.
 *
 * This function returns the number of verified fragments created by the
 * stage 1 matching.
 */
static long int
i_stage1(cdrom_paranoia_t *p, c_block_t *p_new, 
	 void (*callback)(long int, paranoia_cb_mode_t))
{
  long size=cs(p_new);
  c_block_t *ptr=c_last(p);
  int ret=0;
  long int begin=0;
  long int end;

#if TRACE_PARANOIA & 1
  long int block_count = 0;
  fprintf(stderr,
	  "Verifying block %ld:[%ld-%ld] against previously read blocks...\n",
	  p->cache->active,
	  cb(p_new), ce(p_new));
#endif
  
  /* We're going to be comparing the new c_block against the other
   * c_blocks in memory.  Initialize the "sort cache" index to allow
   * for fast searching through the new c_block.  (The index will
   * actually be built the first time we search.)
   */
  if (ptr) 
    sort_setup( p->sortcache, cv(p_new), &cb(p_new), cs(p_new), cb(p_new), 
		ce(p_new) );

  /* Iterate from oldest to newest c_block, comparing the new c_block
   * to each, looking for a sufficiently long run of identical samples
   * (longer than MIN_WORDS_SEARCH), which will be marked as "verified"
   * in both c_blocks.
   *
   * Since the new c_block is already in the list (at the head), don't
   * compare it against itself.
   */
  while ( ptr && ptr != p_new ) {
#if TRACE_PARANOIA & 1
    block_count++;
    fprintf(stderr,
	    "- Verifying against block %ld:[%ld-%ld] dynoverlap=%ld\n",
	    block_count, cb(ptr), ce(ptr), p->dynoverlap);
#endif

    if (callback)
      (*callback)(cb(p_new), PARANOIA_CB_VERIFY);
    i_iterate_stage1(p,ptr,p_new,callback);

    ptr=c_prev(ptr);
  }

  /* parse the verified areas of p_new into v_fragments */

  /* Find each run of contiguous verified samples in the new c_block
   * and create a verified fragment from each run.
   */
  begin=0;
  while (begin<size) {
    for ( ; begin < size; begin++)
      if (p_new->flags[begin]&FLAGS_VERIFIED) break;
    for (end=begin; end < size; end++)
      if ((p_new->flags[end]&FLAGS_VERIFIED)==0) break;
    if (begin>=size) break;
    
    ret++;

    /* We create a new verified fragment from the contiguous run
     * of verified samples.
     *
     * We expand the "verified" range by OVERLAP_ADJ on each side
     * to compensate for trimming done to the verified range by
     * stage1_matched().  The samples were actually verified, and
     * hence belong in the verified fragment.  See stage1_matched()
     * for an explanation of the trimming.
     */
    new_v_fragment(p,p_new,cb(p_new)+max(0,begin-OVERLAP_ADJ),
		   cb(p_new)+min(size,end+OVERLAP_ADJ),
		   (end+OVERLAP_ADJ>=size && p_new->lastsector));

    begin=end;
  }

  /* Return the number of distinct verified fragments we found with
   * stage 1 matching.
   */
  return(ret);
}


/* ===========================================================================
 * STAGE 2 MATCHING
 *
 * ???: Insert high-level explanation here.
 * ===========================================================================
 */

typedef struct sync_result {
  long offset;
  long begin;
  long end;
} sync_result_t;

/* Reconcile v_fragments to root buffer.  Free if matched, fragment/fixup root
   if necessary.

   Do *not* match using zero posts
*/

/* ===========================================================================
 * i_iterate_stage2 (internal)
 *
 * This function searches for a sufficiently long run of identical samples
 * between the passed verified fragment and the verified root.  The search
 * is similar to that performed by i_iterate_stage1.  Of course, what we do
 * as a result of a match is different.
 *
 * Our search is slightly different in that we refuse to match silence to
 * silence.  All silence looks alike, and it would result in too many false
 * positives here, so we handle silence separately.
 *
 * Also, because we're trying to determine whether this fragment as a whole
 * overlaps with the root at all, we narrow our search (since it should match
 * immediately or not at all).  This is in contrast to stage 1, where we
 * search the entire vector looking for all possible matches.
 *
 * This function returns 0 if no match was found (including failure to find
 * one due to silence), or 1 if we found a match.
 *
 * When a match is found, the sync_result_t is set to the boundaries of
 * matching run (begin/end, in terms of the root) and how far out of sync
 * the fragment is from the canonical root (offset).  Note that this offset
 * is opposite in sign from the notion of offset used by try_sort_sync()
 * and stage 1 generally.
 */
static long int 
i_iterate_stage2(cdrom_paranoia_t *p,
		 v_fragment_t *v,
		 sync_result_t *r,
		 void(*callback)(long int, paranoia_cb_mode_t))
{
  root_block *root=&(p->root);
  long matchbegin=-1,matchend=-1,offset;
  long fbv,fev;
  
#if TRACE_PARANOIA & 2
  fprintf(stderr, "- Comparing fragment [%ld-%ld] to root [%ld-%ld]...",
	  fb(v), fe(v), rb(root), re(root));
#endif

#ifdef NOISY
      fprintf(stderr,"Stage 2 search: fbv=%ld fev=%ld\n",fb(v),fe(v));
#endif

  /* Quickly check whether there could possibly be any overlap between
   * the verified fragment and the root.  Our search will allow up to
   * (p->dynoverlap) jitter between the two, so we expand the fragment
   * search area by p->dynoverlap on both sides and see if that expanded
   * area overlaps with the root.
   *
   * We could just as easily expand root's boundaries by p->dynoverlap
   * instead and achieve the same result.
   */
  if (min(fe(v) + p->dynoverlap,re(root)) -
    max(fb(v) - p->dynoverlap,rb(root)) <= 0) 
    return(0);

  if (callback)
    (*callback)(fb(v), PARANOIA_CB_VERIFY);

  /* We're going to try to match the fragment to the root while allowing
   * for p->dynoverlap jitter, so we'll actually be looking at samples
   * in the fragment whose position claims to be up to p->dynoverlap
   * outside the boundaries of the root.  But, of course, don't extend
   * past the edges of the fragment.
   */
  fbv = max(fb(v), rb(root)-p->dynoverlap);

  /* Skip past leading zeroes in the fragment, and bail if there's nothing
   * but silence.  We handle silence later separately.
   */
  while (fbv<fe(v) && fv(v)[fbv-fb(v)]==0)
    fbv++;
  if (fbv == fe(v))
    return(0);

  /* This is basically the same idea as the initial calculation for fbv
   * above.  Look at samples up to p->dynoverlap outside the boundaries
   * of the root, but don't extend past the edges of the fragment.
   *
   * However, we also limit the search to no more than 256 samples.
   * Unlike stage 1, we're not trying to find all possible matches within
   * two runs -- rather, we're trying to see if the fragment as a whole
   * overlaps with the root.  If we can't find a match within 256 samples,
   * there's probably no match to be found (because this fragment doesn't
   * overlap with the root).
   *
   * ??? Is this why?  Why 256?
   */
  fev = min(min(fbv+256, re(root)+p->dynoverlap), fe(v));
  
  {
    /* Because we'll allow for up to (p->dynoverlap) jitter between the
     * fragment and the root, we expand the search area (fbv to fev) by
     * p->dynoverlap on both sides.  But, because we're iterating through
     * root, we need to constrain the search area not to extend beyond
     * the root's boundaries.
     */
    long searchend=min(fev+p->dynoverlap,re(root));
    long searchbegin=max(fbv-p->dynoverlap,rb(root));
    sort_info_t *i=p->sortcache;
    long j;

    /* Initialize the "sort cache" index to allow for fast searching
     * through the verified fragment between (fbv,fev).  (The index will
     * actually be built the first time we search.)
     */
    sort_setup(i, fv(v), &fb(v), fs(v), fbv, fev);

    /* ??? Why 23? */
    for(j=searchbegin; j<searchend; j+=23){

      /* Skip past silence in the root.  If there are just a few silent
       * samples, the effect is minimal.  The real reason we need this is
       * for large regions of silence.  All silence looks alike, so you
       * could false-positive "match" two runs of silence that are either
       * unrelated or ought to be jittered, and try_sort_sync can't
       * accurately determine jitter (offset) from silence.
       *
       * Therefore, we want to post on a non-zero sample.  If there's
       * nothing but silence left in the root, bail.  We don't want
       * to match it here.
       */
      while (j<searchend && rv(root)[j-rb(root)]==0)j++;
      if (j==searchend) break;

      /* Starting from the (non-zero) sample in the root with the absolute
       * position j, look for a matching run in the verified fragment.  This
       * search will look a certain distance around j, and if successful
       * will extend the matching run as far backward and forward as
       * it can.
       *
       * The search will only return 1 if it finds a matching run long
       * enough to be deemed significant.  Note that the search is limited
       * by the boundaries given to sort_setup() above.
       *
       * Note also that flags aren't used in stage 2 (since neither verified
       * fragments nor the root have them).
       */
      if (try_sort_sync(p, i, NULL, rc(root), j,
			&matchbegin,&matchend,&offset,callback)){

	/* If we found a matching run, we return the results of our match.
	 *
	 * Note that we flip the sign of (offset) because try_sort_sync()
	 * returns it in terms of the fragment (i.e. what we add
	 * to the fragment's position to yield the corresponding position
	 * in the root), but here we consider the root to be canonical,
	 * and so our returned "offset" reflects how the fragment is offset
	 * from the root.
	 *
	 * E.g.: If the fragment's sample 10 corresponds to root's 12,
	 * try_sort_sync() would return 2.  But since root is canonical,
	 * we say that the fragment is off by -2.
	 */
	r->begin=matchbegin;
	r->end=matchend;
	r->offset=-offset;
	if (offset)if (callback)(*callback)(r->begin,PARANOIA_CB_FIXUP_EDGE);
	return(1);
      }
    }
  }
  
  return(0);
}


/* ===========================================================================
 * i_silence_test() (internal)
 *
 * If the entire root is silent, or there's enough trailing silence
 * to be significant (MIN_SILENCE_BOUNDARY samples), mark the beginning
 * of the silence and "light" the silence flag.  This flag will remain lit
 * until i_silence_match() appends some non-silent samples to the root.
 *
 * We do this because if there's a long enough span of silence, we can't
 * reliably detect jitter or dropped samples within that span.  See
 * i_silence_match() for details on how we recover from this situation.
 */
static void 
i_silence_test(root_block *root)
{
  int16_t *vec=rv(root);
  long end=re(root)-rb(root)-1;
  long j;

  /* Look backward from the end of the root to find the first non-silent
   * sample.
   */
  for(j=end-1;j>=0;j--)
    if (vec[j]!=0) break;

  /* If the entire root is silent, or there's enough trailing silence
   * to be significant, mark the beginning of the silence and "light"
   * the silence flag.
   */
  if (j<0 || end-j>MIN_SILENCE_BOUNDARY) {
    /* ???BUG???:
     *
     * The original code appears to have a bug, as it points to the
     * last non-zero sample, and silence matching appears to treat
     * silencebegin as the first silent sample.  As a result, in certain
     * situations, the last non-zero sample can get clobbered.
     *
     * This bug has been tentatively fixed, since it allows more regression
     * tests to pass.  The original code was:
     *   if (j<0)j=0;
     */
    j++;

    root->silenceflag=1;
    root->silencebegin=rb(root)+j;

    /* ???: To be studied. */
    if (root->silencebegin<root->returnedlimit)
      root->silencebegin=root->returnedlimit;
  }
}


/* ===========================================================================
 * i_silence_match() (internal)
 *
 * This function is merges verified fragments into the verified root in cases
 * where there is a problematic amount of silence (MIN_SILENCE_BOUNDARY
 * samples) at the end of the root.
 *
 * We need a special approach because if there's a long enough span of
 * silence, we can't reliably detect jitter or dropped samples within that
 * span (since all silence looks alike).
 *
 * Only fragments that begin with MIN_SILENCE_BOUNDARY samples are eligible
 * to be merged in this case.  Fragments that are too far beyond the edge
 * of the root to possibly overlap are also disregarded.
 *
 * Our first approach is to assume that such fragments have no jitter (since
 * we can't establish otherwise) and merge them.  However, if it's clear
 * that there must be jitter (i.e. because non-silent samples overlap when
 * we assume no jitter), we assume the fragment has the minimum possible
 * jitter and then merge it.
 *
 * This function extends silence fairly aggressively, so it must be called
 * with fragments in ascending order (beginning position) in case there are
 * small non-silent regions within the silence.
 */
static long int 
i_silence_match(root_block *root, v_fragment_t *v, 
		void(*callback)(long int, paranoia_cb_mode_t))
{

  cdrom_paranoia_t *p=v->p;
  int16_t *vec=fv(v);
  long end=fs(v),begin;
  long j;

#if TRACE_PARANOIA & 2
  fprintf(stderr, "- Silence matching fragment [%ld-%ld] to root [%ld-%ld]"
	  " silencebegin=%ld\n",
	  fb(v), fe(v), rb(root), re(root), root->silencebegin);
#endif

  /* See how much leading silence this fragment has.  If there are fewer than
   * MIN_SILENCE_BOUNDARY leading silent samples, we don't do this special
   * silence matching.
   *
   * This fragment could actually belong here, but we can't be sure unless
   * it has enough silence on its leading edge.  This fragment will likely
   * stick around until we do successfully extend the root, at which point
   * it will be merged using the usual method.
   */
  if (end<MIN_SILENCE_BOUNDARY) return(0);
  for(j=0;j<end;j++)
    if (vec[j]!=0) break;
  if (j<MIN_SILENCE_BOUNDARY) return(0);

  /* Convert the offset of the first non-silent sample to an absolute
   * position.  For the time being, we will assume that this position
   * is accurate, with no jitter.
   */
  j+=fb(v);

#if TRACE_PARANOIA & 2
  fprintf(stderr, "- Fragment begins with silence [%ld-%ld]\n", fb(v), j);
#endif

  /* If this fragment is ahead of the root, see if that could just be due
   * to jitter (if it's within p->dynoverlap samples of the end of root).
   */
  if (fb(v)>=re(root) && fb(v)-p->dynoverlap<re(root)){

    /* This fragment is within jitter range of the root, so we extend the
     * root's silence so that it overlaps with this fragment.  At this point
     * we know that the fragment has at least MIN_SILENCE_BOUNDARY silent
     * samples at the beginning, so we overlap by that amount.
     */
    long addto   = fb(v) + MIN_SILENCE_BOUNDARY - re(root);
    int16_t *vec = calloc(addto, sizeof(int16_t));
    c_append(rc(root), vec, addto);
    free(vec);

#if TRACE_PARANOIA & 2
    fprintf(stderr, "* Adding silence [%ld-%ld] to root\n",
	    re(root)-addto, re(root));
#endif
  }

  /* Calculate the overlap of the root's trailing silence and the fragment's
   * leading silence.  (begin,end) are the boundaries of that overlap.
   */
  begin = max(fb(v),root->silencebegin);
  end = min(j,re(root));

  /* If there is an overlap, we assume that both the root and the fragment
   * are jitter-free (since there's no way for us to tell otherwise).
   */
  if (begin<end){

    /* If the fragment will extend the root, then we append it to the root.
     * Otherwise, no merging is necessary, as the fragment should already
     * be contained within the root.
     */
    if (fe(v)>re(root)){
      long int voff = begin-fb(v);

      /* Truncate the overlapping silence from the end of the root.
       */
      c_remove(rc(root),begin-rb(root),-1);

      /* Append the fragment to the root, starting from the point of overlap.
       */
      c_append(rc(root),vec+voff,fs(v)-voff);

#if TRACE_PARANOIA & 2
      fprintf(stderr, "* Adding [%ld-%ld] to root (no jitter)\n",
	      begin, re(root));
#endif
    }

    /* Record the fact that we merged this fragment assuming zero jitter.
     */
    offset_add_value(p,&p->stage2,0,callback);

  } else {

    /* We weren't able to merge the fragment assuming zero jitter.
     *
     * Check whether the fragment's leading silence ends before the root's
     * trailing silence begins.  If it does, we assume that the root is
     * jittered forward.
     */
    if (j<begin){

      /* We're going to append the non-silent samples of the fragment
       * to the root where its silence begins.
       *
       * ??? This seems to be a very strange approach.  At this point
       * the root has a lot of trailing silence, and the fragment has
       * the lot of leading silence.  This merge will drop the silence
       * and just splice the non-silence together.
       *
       * In theory, rift analysis will either confirm or fix this result.
       * What circumstances motivated this approach?
       */

      /* Compute the amount of silence at the beginning of the fragment.
       */
      long voff = j - fb(v);

      /* If attaching the non-silent tail of the fragment to the end
       * of the non-silent portion of the root will extend the root,
       * then we'll append the samples to the root.  Otherwise, no
       * merging is necessary, as the fragment should already be contained
       * within the root.
       */
      if (begin+fs(v)-voff>re(root)) {

	/* Truncate the trailing silence from the root.
	 */
	c_remove(rc(root),root->silencebegin-rb(root),-1);

	/* Append the non-silent tail of the fragment to the root.
	 */
	c_append(rc(root),vec+voff,fs(v)-voff);

#if TRACE_PARANOIA & 2
	fprintf(stderr, "* Adding [%ld-%ld] to root (jitter=%ld)\n",
		root->silencebegin, re(root), end-begin);
#endif
      }

      /* Record the fact that we merged this fragment assuming (end-begin)
       * jitter.
       */
      offset_add_value(p,&p->stage2,end-begin,callback);

    } else

      /* We only get here if the fragment is past the end of the root,
       * which means it must be farther than (dynoverlap) away, due to our
       * root extension above.
       */

      /* We weren't able to merge this fragment into the root after all.
       */
      return(0);
  }


  /* We only get here if we merged the fragment into the root.  Update
   * the root's silence flag.
   *
   * Note that this is the only place silenceflag is reset.  In other words,
   * once i_silence_test() lights the silence flag, it can only be reset
   * by i_silence_match().
   */
  root->silenceflag = 0;

  /* Now see if the new, extended root ends in silence.
   */
  i_silence_test(root);


  /* Since we merged the fragment, we can free it now.  But first we propagate
   * its lastsector flag.
   */
  if (v->lastsector) root->lastsector=1;
  free_v_fragment(v);
  return(1);
}


/* ===========================================================================
 * i_stage2_each (internal)
 *
 * This function (which is entirely too long) attempts to merge the passed
 * verified fragment into the verified root.
 *
 * First this function looks for a run of identical samples between
 * the root and the fragment.  If it finds a long enough run, it then
 * checks for "rifts" (see below) and fixes the root and/or fragment as
 * necessary.  Finally, if the fragment will extend the tail of the root,
 * we merge the fragment and extend the root.
 *
 * Most of the ugliness in this function has to do with handling "rifts",
 * which are points of disagreement between the root and the verified
 * fragment.  This can happen when a drive consistently drops a few samples
 * or stutters and repeats a few samples.  It has to be consistent enough
 * to result in a verified fragment (i.e. it happens twice), but inconsistent
 * enough (e.g. due to the jiggled reads) not to happen every time.
 *
 * This function returns 1 if the fragment was successfully merged into the
 * root, and 0 if not.
 */
static long int 
i_stage2_each(root_block *root, v_fragment_t *v,
	      void(*callback)(long int, paranoia_cb_mode_t))
{

  cdrom_paranoia_t *p=v->p;

  /* ??? Why do we round down to an even dynoverlap? */
  long dynoverlap=p->dynoverlap/2*2;
  
  /* If this fragment has already been merged & freed, abort. */
  if (!v || !v->one) return(0);

  /* If there's no verified root yet, abort. */
  if (!rv(root)){
    return(0);
  } else {
    sync_result_t r;

    /* Search for a sufficiently long run of identical samples between
     * the verified fragment and the verified root.  There's a little
     * bit of subtlety in the search when silence is involved.
     */
    if (i_iterate_stage2(p,v,&r,callback)){

      /* Convert the results of the search to be relative to the root. */
      long int begin=r.begin-rb(root);
      long int end=r.end-rb(root);

      /* Convert offset into a value that will transform a relative
       * position in the root to the corresponding relative position in
       * the fragment.  I.e., if offset = -2, then the sample at relative
       * position 2 in the root is at relative position 0 in the fragment.
       *
       * While a bit opaque, this does reduce the number of calculations
       * below.
       */
      long int offset=r.begin+r.offset-fb(v)-begin;
      long int temp;
      c_block_t *l=NULL;

      /* we have a match! We don't rematch off rift, we chase the
	 match all the way to both extremes doing rift analysis. */

#if TRACE_PARANOIA & 2
      fprintf(stderr, "matched [%ld-%ld], offset=%ld\n",
	      r.begin, r.end, r.offset);
      int traced = 0;
#endif
#ifdef NOISY
      fprintf(stderr,"Stage 2 match\n");
#endif

      /* Now that we've found a sufficiently long run of identical samples
       * between the fragment and the root, we need to check for rifts.
       *
       * A "rift", as mentioned above, is a disagreement between the
       * fragment and the root.  When there's a rift, the matching run
       * found by i_iterate_stage2() will obviously stop where the root
       * and the fragment disagree.
       *
       * So we detect rifts by checking whether the matching run extends
       * to the ends of the fragment and root.  If the run does extend to
       * the ends of the fragment and root, then all overlapping samples
       * agreed, and there's no rift.  If, however, the matching run
       * stops with samples left over in both the root and the fragment,
       * that means the root and fragment disagreed at that point.
       * Leftover samples at the beginning of the match indicate a
       * leading rift, and leftover samples at the end of the match indicate
       * a trailing rift.
       *
       * Once we detect a rift, we attempt to fix it, depending on the
       * nature of the disagreement.  See i_analyze_rift_[rf] for details
       * on how we determine what kind of rift it is.  See below for
       * how we attempt to fix the rifts.
       */

      /* First, check for a leading rift, fix it if possible, and then
       * extend the match forward until either we hit the limit of the
       * overlapping samples, or until we encounter another leading rift.
       * Keep doing this until we hit the beginning of the overlap.
       *
       * Note that while we do fix up leading rifts, we don't extend
       * the root backward (earlier samples) -- only forward (later
       * samples).
       */

      /* If the beginning of the match didn't reach the beginning of
       * either the fragment or the root, we have a leading rift to be
       * examined.
       *
       * Remember that (begin) is the offset into the root, and (begin+offset)
       * is the equivalent offset into the fragment.  If neither one is at
       * zero, then they both have samples before the match, and hence a
       * rift.
       */
      while ((begin+offset>0 && begin>0)){
	long matchA=0,matchB=0,matchC=0;

	/* (begin) is the offset into the root of the first matching sample,
	 * (beginL) is the offset into the fragment of the first matching
	 * sample.  These samples are at the edge of the rift.
	 */
	long beginL=begin+offset;

#if TRACE_PARANOIA & 2
	if ((traced & 1) == 0) {
	  fprintf(stderr, "- Analyzing leading rift...\n");
	  traced |= 1;
	}
#endif

	/* The first time we encounter a leading rift, allocate a
	 * scratch copy of the verified fragment which we'll use if
	 * we need to fix up the fragment before merging it into
	 * the root.
	 */
	if (l==NULL){
	  int16_t *buff=malloc(fs(v)*sizeof(int16_t));
	  l=c_alloc(buff,fb(v),fs(v));
	  memcpy(buff,fv(v),fs(v)*sizeof(int16_t));
	}

	/* Starting at the first mismatching sample, see how far back the
	 * rift goes, and determine what kind of rift it is.  Note that
	 * we're searching through the fixed up copy of the fragment.
	 *
	 * matchA  > 0 if there are samples missing from the root
	 * matchA  < 0 if there are duplicate samples (stuttering) in the root
	 * matchB  > 0 if there are samples missing from the fragment
	 * matchB  < 0 if there are duplicate samples in the fragment
	 * matchC != 0 if there's a section of garbage, after which
	 *             the fragment and root agree and are in sync
	 */
	i_analyze_rift_r(rv(root),cv(l),
			 rs(root),cs(l),
			 begin-1,beginL-1,
			 &matchA,&matchB,&matchC);
	
#ifdef NOISY
	fprintf(stderr,"matching rootR: matchA:%ld matchB:%ld matchC:%ld\n",
		matchA,matchB,matchC);
#endif		
	
	/* ??? The root.returnedlimit checks below are presently a mystery. */

	if (matchA){
	  /* There's a problem with the root */

	  if (matchA>0){
	    /* There were (matchA) samples dropped from the root.  We'll add
	     * them back from the fixed up fragment.
	     */
	    if (callback)
	      (*callback)(begin+rb(root)-1,PARANOIA_CB_FIXUP_DROPPED);
	    if (rb(root)+begin<p->root.returnedlimit)
	      break;
	    else{

	      /* At the edge of the rift in the root, insert the missing
	       * samples from the fixed up fragment.  They're the (matchA)
	       * samples immediately preceding the edge of the rift in the
	       * fragment.
	       */
	      c_insert(rc(root),begin,cv(l)+beginL-matchA,
		       matchA);

	      /* We just inserted (matchA) samples into the root, so update
	       * our begin/end offsets accordingly.  Also adjust the
	       * (offset) to compensate (since we use it to find samples in
	       * the fragment, and the fragment hasn't changed).
	       */
	      offset-=matchA;
	      begin+=matchA;
	      end+=matchA;
	    }

	  } else {
	    /* There were (-matchA) duplicate samples (stuttering) in the
	     * root.  We'll drop them.
	     */
	    if (callback)
	      (*callback)(begin+rb(root)-1,PARANOIA_CB_FIXUP_DUPED);
	    if (rb(root)+begin+matchA<p->root.returnedlimit) 
	      break;
	    else{

	      /* Remove the (-matchA) samples immediately preceding the
	       * edge of the rift in the root.
	       */
	      c_remove(rc(root),begin+matchA,-matchA);

	      /* We just removed (-matchA) samples from the root, so update
	       * our begin/end offsets accordingly.  Also adjust the offset
	       * to compensate.  Remember that matchA < 0, so we're actually
	       * subtracting from begin/end.
	       */
	      offset-=matchA;
	      begin+=matchA;
	      end+=matchA;
	    }
	  }
	} else if (matchB){
	  /* There's a problem with the fragment */

	  if (matchB>0){
	    /* There were (matchB) samples dropped from the fragment.  We'll
	     * add them back from the root.
	     */
	    if (callback)
	      (*callback)(begin+rb(root)-1,PARANOIA_CB_FIXUP_DROPPED);

	    /* At the edge of the rift in the fragment, insert the missing
	     * samples from the root.  They're the (matchB) samples
	     * immediately preceding the edge of the rift in the root.
	     * Note that we're fixing up the scratch copy of the fragment.
	     */
	    c_insert(l,beginL,rv(root)+begin-matchB,
			 matchB);

	    /* We just inserted (matchB) samples into the fixed up fragment,
	     * so update (offset), since we use it to find samples in the
	     * fragment based on the root's unchanged offsets.
	     */
	    offset+=matchB;

	  } else {
	    /* There were (-matchB) duplicate samples (stuttering) in the
	     * fixed up fragment.  We'll drop them.
	     */
	    if (callback)
	      (*callback)(begin+rb(root)-1,PARANOIA_CB_FIXUP_DUPED);

	    /* Remove the (-matchB) samples immediately preceding the edge
	     * of the rift in the fixed up fragment.
	     */
	    c_remove(l,beginL+matchB,-matchB);

	    /* We just removed (-matchB) samples from the fixed up fragment,
	     * so update (offset), since we use it to find samples in the
	     * fragment based on the root's unchanged offsets.
	     */
	    offset+=matchB;
	  }

	} else if (matchC){

	  /* There are (matchC) samples that simply disagree between the
	   * fragment and the root.  On the other side of the mismatch, the
	   * fragment and root agree again.  We can't classify the mismatch
	   * as either a stutter or dropped samples, and we have no way of
	   * telling whether the fragment or the root is right.
	   *
	   * The original comment indicated that we set "disagree" flags
	   * in the root, but it seems to be historical.
	   */

	  if (rb(root)+begin-matchC<p->root.returnedlimit)
	    break;

	  /* Overwrite the mismatching (matchC) samples in root with the
	   * samples from the fixed up fragment.
	   *
	   * ??? Do we think the fragment is more likely correct, is this
	   * just arbitrary, or is there some other reason for overwriting
	   * the root?
	   */
	  c_overwrite(rc(root),begin-matchC,
			cv(l)+beginL-matchC,matchC);
	  
	} else {

	  /* We may have had a mismatch because we ran into leading silence.
	   *
	   * ??? To be studied: why would this cause a mismatch?  Neither
	   * i_analyze_rift_r nor i_iterate_stage2() nor i_paranoia_overlap()
	   * appear to take silence into consideration in this regard.
	   * It could be due to our skipping of silence when searching for
	   * a match.
	   *
	   * Since we don't extend the root in that direction, we don't
	   * do anything, just move on to trailing rifts.
	   */

	  /* If the rift was too complex to fix (see i_analyze_rift_r),
	   * we just stop and leave the leading edge where it is.
	   */
	    
	  /*RRR(*callback)(post,PARANOIA_CB_XXX);*/
	  break;
	}

	/* Recalculate the offset of the edge of the rift in the fixed
	 * up fragment, in case it changed.
	 *
	 * ??? Why is this done here rather than in the (matchB) case above,
	 * which should be the only time beginL will change.
	 */
	beginL=begin+offset;

	/* Now that we've fixed up the root or fragment as necessary, see
	 * how far we can extend the matching run.  This function is
	 * overkill, as it tries to extend the matching run in both
	 * directions (and rematches what we already matched), but it works.
	 */
	i_paranoia_overlap(rv(root),cv(l),
			   begin,beginL,
			   rs(root),cs(l),
			   &begin,&end);	

      } /* end while (leading rift) */


      /* Second, check for a trailing rift, fix it if possible, and then
       * extend the match forward until either we hit the limit of the
       * overlapping samples, or until we encounter another trailing rift.
       * Keep doing this until we hit the end of the overlap.
       */

      /* If the end of the match didn't reach the end of either the fragment
       * or the root, we have a trailing rift to be examined.
       *
       * Remember that (end) is the offset into the root, and (end+offset)
       * is the equivalent offset into the fragment.  If neither one is
       * at the end of the vector, then they both have samples after the
       * match, and hence a rift.
       *
       * (temp) is the size of the (potentially fixed-up) fragment.  If
       * there was a leading rift, (l) is the fixed up fragment, and
       * (offset) is now relative to it.
       */
      temp=l ? cs(l) : fs(v);
      while (end+offset<temp && end<rs(root)){
	long matchA=0,matchB=0,matchC=0;

	/* (begin) is the offset into the root of the first matching sample,
	 * (beginL) is the offset into the fragment of the first matching
	 * sample.  We know these samples match and will use these offsets
	 * later when we try to extend the matching run.
	 */
	long beginL=begin+offset;

	/* (end) is the offset into the root of the first mismatching sample
	 * after the matching run, (endL) is the offset into the fragment of
	 * the equivalent sample.  These samples are at the edge of the rift.
	 */
	long endL=end+offset;
	
#if TRACE_PARANOIA & 2
	if ((traced & 2) == 0) {
	  fprintf(stderr, "- Analyzing trailing rift...\n");
	  traced |= 2;
	}
#endif

	/* The first time we encounter a rift, allocate a scratch copy of
	 * the verified fragment which we'll use if we need to fix up the
	 * fragment before merging it into the root.
	 *
	 * Note that if there was a leading rift, we'll already have
	 * this (potentially fixed-up) scratch copy allocated.
	 */
	if (l==NULL){
	  int16_t *buff=malloc(fs(v)*sizeof(int16_t));
	  l=c_alloc(buff,fb(v),fs(v));
	  memcpy(buff,fv(v),fs(v)*sizeof(int16_t));
	}

	/* Starting at the first mismatching sample, see how far forward the
	 * rift goes, and determine what kind of rift it is.  Note that we're
	 * searching through the fixed up copy of the fragment.
	 *
	 * matchA  > 0 if there are samples missing from the root
	 * matchA  < 0 if there are duplicate samples (stuttering) in the root
	 * matchB  > 0 if there are samples missing from the fragment
	 * matchB  < 0 if there are duplicate samples in the fragment
	 * matchC != 0 if there's a section of garbage, after which
	 *             the fragment and root agree and are in sync
	 */
	i_analyze_rift_f(rv(root),cv(l),
			 rs(root),cs(l),
			 end,endL,
			 &matchA,&matchB,&matchC);
	
#ifdef NOISY	
	fprintf(stderr,"matching rootF: matchA:%ld matchB:%ld matchC:%ld\n",
		matchA,matchB,matchC);
#endif

	/* ??? The root.returnedlimit checks below are presently a mystery. */
	
	if (matchA){
	  /* There's a problem with the root */

	  if (matchA>0){
	    /* There were (matchA) samples dropped from the root.  We'll add
	     * them back from the fixed up fragment.
	     */
	    if (callback)(*callback)(end+rb(root),PARANOIA_CB_FIXUP_DROPPED);
	    if (end+rb(root)<p->root.returnedlimit)
	      break;

	    /* At the edge of the rift in the root, insert the missing
	     * samples from the fixed up fragment.  They're the (matchA)
	     * samples immediately preceding the edge of the rift in the
	     * fragment.
	     */
	    c_insert(rc(root),end,cv(l)+endL,matchA);

	    /* Although we just inserted samples into the root, we did so
	     * after (begin) and (end), so we needn't update those offsets.
	     */

	  } else {
	    /* There were (-matchA) duplicate samples (stuttering) in the
	     * root.  We'll drop them.
	     */
	    if (callback)(*callback)(end+rb(root),PARANOIA_CB_FIXUP_DUPED);
	    if (end+rb(root)<p->root.returnedlimit)
	      break;

	    /* Remove the (-matchA) samples immediately following the edge
	     * of the rift in the root.
	     */
	    c_remove(rc(root),end,-matchA);

	    /* Although we just removed samples from the root, we did so
	     * after (begin) and (end), so we needn't update those offsets.
	     */

	  }
	} else if (matchB){
	  /* There's a problem with the fragment */

	  if (matchB>0){
	    /* There were (matchB) samples dropped from the fragment.  We'll
	     * add them back from the root.
	     */
	    if (callback)(*callback)(end+rb(root),PARANOIA_CB_FIXUP_DROPPED);

	    /* At the edge of the rift in the fragment, insert the missing
	     * samples from the root.  They're the (matchB) samples
	     * immediately following the dge of the rift in the root.
	     * Note that we're fixing up the scratch copy of the fragment.
	     */
	    c_insert(l,endL,rv(root)+end,matchB);

	    /* Although we just inserted samples into the fragment, we did so
	     * after (begin) and (end), so (offset) hasn't changed either.
	     */

	  } else {
	    /* There were (-matchB) duplicate samples (stuttering) in the
	     * fixed up fragment.  We'll drop them.
	     */
	    if (callback)(*callback)(end+rb(root),PARANOIA_CB_FIXUP_DUPED);

	    /* Remove the (-matchB) samples immediately following the edge
	     * of the rift in the fixed up fragment.
	     */
	    c_remove(l,endL,-matchB);

	    /* Although we just removed samples from the fragment, we did so
	     * after (begin) and (end), so (offset) hasn't changed either.
	     */
	  }
	} else if (matchC){

          /* There are (matchC) samples that simply disagree between the
           * fragment and the root.  On the other side of the mismatch, the
           * fragment and root agree again.  We can't classify the mismatch
           * as either a stutter or dropped samples, and we have no way of
           * telling whether the fragment or the root is right.
           *
           * The original comment indicated that we set "disagree" flags
           * in the root, but it seems to be historical.
           */

	  if (end+rb(root)<p->root.returnedlimit)
	    break;

          /* Overwrite the mismatching (matchC) samples in root with the
           * samples from the fixed up fragment.
           *
           * ??? Do we think the fragment is more likely correct, is this
           * just arbitrary, or is there some other reason for overwriting
           * the root?
           */
	  c_overwrite(rc(root),end,cv(l)+endL,matchC);

	} else {

	  /* We may have had a mismatch because we ran into trailing silence.
	   *
	   * ??? To be studied: why would this cause a mismatch?  Neither
	   * i_analyze_rift_f nor i_iterate_stage2() nor i_paranoia_overlap()
	   * appear to take silence into consideration in this regard.
           * It could be due to our skipping of silence when searching for
           * a match.
	   */

	  /* At this point we have a trailing rift.  We check whether
	   * one of the vectors (fragment or root) has trailing silence.
	   */
	  analyze_rift_silence_f(rv(root),cv(l),
				 rs(root),cs(l),
				 end,endL,
				 &matchA,&matchB);
	  if (matchA){

	    /* The contents of the root's trailing rift are silence.  The
	     * fragment's are not (otherwise there wouldn't be a rift).
	     * We therefore assume that the root has garbage from this
	     * point forward and truncate it.
	     *
	     * This will have the effect of eliminating the trailing
	     * rift, causing the fragment's samples to be appended to
	     * the root.
	     *
	     * ??? Does this have any negative side effects?  Why is this
	     * a good idea?
	     */
	    /* ??? TODO: returnedlimit */
	    /* Can only do this if we haven't already returned data */
	    if (end+rb(root)>=p->root.returnedlimit){
	      c_remove(rc(root),end,-1);
	    }

	  } else if (matchB){

	    /* The contents of the fragment's trailing rift are silence.
	     * The root's are not (otherwise there wouldn't be a rift).
	     * We therefore assume that the fragment has garbage from this
	     * point forward.
	     *
	     * We needn't actually truncate the fragment, because the root
	     * has already been fixed up from this fragment as much as
	     * possible, and the truncated fragment wouldn't extend the
	     * root.  Therefore, we can consider this (truncated) fragment
	     * to be already merged into the root.  So we dispose of it and
	     * return a success.
	     */
	    if (l)i_cblock_destructor(l);
	    free_v_fragment(v);
	    return(1);

	  } else {

	    /* If the rift was too complex to fix (see i_analyze_rift_f),
	     * we just stop and leave the trailing edge where it is.
	     */
	    
	    /*RRR(*callback)(post,PARANOIA_CB_XXX);*/
	  }
	  break;
	}

        /* Now that we've fixed up the root or fragment as necessary, see
         * how far we can extend the matching run.  This function is
         * overkill, as it tries to extend the matching run in both
         * directions (and rematches what we already matched), but it works.
         */
	i_paranoia_overlap(rv(root),cv(l),
			   begin,beginL,
			   rs(root),cs(l),
			   NULL,&end);

	/* ???BUG??? (temp) never gets updated within the loop, even if the
	 * fragment gets fixed up.  In contrast, rs(root) is inherently
	 * updated when the verified root gets fixed up.
	 *
	 * This bug is not fixed yet.
	 */

      } /* end while (trailing rift) */


      /* Third and finally, if the overlapping verified fragment extends
       * our range forward (later samples), we append ("glom") the new
       * samples to the end of the root.
       *
       * Note that while we did fix up leading rifts, we don't extend
       * the root backward (earlier samples) -- only forward (later
       * samples).
       *
       * This is generally fine, since the verified root is supposed to
       * slide from earlier samples to later samples across multiple calls
       * to paranoia_read().
       *
       * ??? But, is this actually right?  Because of this, we don't
       * extend the root to hold the earliest read sample, if we happened
       * to initialize the root with a later sample due to jitter.
       * There are probably some ugly side effects from extending the root
       * backward, in the general case, but it may not be so dire if we're
       * near sample 0.  To be investigated.
       */
      {
	long sizeA=rs(root);
	long sizeB;
	long vecbegin;
	int16_t *vector;

	/* If there were any rifts, we'll use the fixed up fragment (l),
	 * otherwise, we use the original fragment (v).
	 */
	if (l){
	  sizeB=cs(l);
	  vector=cv(l);
	  vecbegin=cb(l);
	} else {
	  sizeB=fs(v);
	  vector=fv(v);
	  vecbegin=fb(v);
	}

	/* Convert the fragment-relative offset (sizeB) into an offset
	 * relative to the root (A), and see if the offset is past the
	 * end of the root (> sizeA).  If it is, this fragment will extend
	 * our root.
	 *
	 * ??? Why do we check for v->lastsector separately?
	 */
	if (sizeB-offset>sizeA || v->lastsector){	  
	  if (v->lastsector){
	    root->lastsector=1;
	  }

	  /* ??? Why would end be < sizeA? Why do we truncate root? */
	  if (end<sizeA)c_remove(rc(root),end,-1);

	  /* Extend the root with the samples from the end of the
	   * (potentially fixed up) fragment.
	   *
	   * ??? When would this condition not be true?
	   */
	  if (sizeB-offset-end)c_append(rc(root),vector+end+offset,
					 sizeB-offset-end);
	  
#if TRACE_PARANOIA & 2
	  fprintf(stderr, "* Adding [%ld-%ld] to root\n",
		  rb(root)+end, re(root));
#endif

	  /* Any time we update the root we need to check whether it ends
	   * with a large span of silence.
	   */
	  i_silence_test(root);

	  /* Add the offset into our stage 2 statistics.
	   *
	   * Note that we convert our peculiar offset (which is in terms of
	   * the relative positions of samples within each vector) back into
	   * the actual offset between what A considers sample N and what B
	   * considers sample N.
	   *
	   * We do this at the end of rift handling because any original
	   * offset returned by i_iterate_stage2() might have been due to
	   * dropped or duplicated samples.  Once we've fixed up the root
	   * and the fragment, we have an offset which more reliably
	   * indicates jitter.
	   */
	  offset_add_value(p,&p->stage2,offset+vecbegin-rb(root),callback);
	}
      }
      if (l)i_cblock_destructor(l);
      free_v_fragment(v);
      return(1);
      
    } else { /* !i_iterate_stage2(...) */
#if TRACE_PARANOIA & 2
      fprintf(stderr, "no match");
#endif

      /* We were unable to merge this fragment into the root.
       *
       * Check whether the fragment should have overlapped with the root,
       * even taking possible jitter into account.  (I.e., If the fragment
       * ends so far before the end of the root that even (dynoverlap)
       * samples of jitter couldn't push it beyond the end of the root,
       * it should have overlapped.)
       *
       * It is, however, possible that we failed to match using the normal
       * tests because we're dealing with silence, which we handle
       * separately.
       *
       * If the fragment should have overlapped, and we're not dealing
       * with the special silence case, we don't know what to make of
       * this fragment, and we just discard it.
       */
      if (fe(v)+dynoverlap<re(root) && !root->silenceflag){
	/* It *should* have matched.  No good; free it. */
	free_v_fragment(v);
#if TRACE_PARANOIA & 2
	fprintf(stderr, ", discarding fragment.");
#endif
      }

#if TRACE_PARANOIA & 2
      fprintf(stderr, "\n");
#endif

      /* otherwise, we likely want this for an upcoming match */
      /* we don't free the sort info (if it was collected) */
      return(0);
      
    }
  } /* endif rv(root) */
}

static int 
i_init_root(root_block *root, v_fragment_t *v,long int begin,
		       void(*callback)(long int, paranoia_cb_mode_t))
{
  if (fb(v)<=begin && fe(v)>begin){
    
    root->lastsector=v->lastsector;
    root->returnedlimit=begin;

    if (rv(root)){
      i_cblock_destructor(rc(root));
      rc(root)=NULL;
    }

    {
      int16_t *buff=malloc(fs(v)*sizeof(int16_t));
      memcpy(buff,fv(v),fs(v)*sizeof(int16_t));
      root->vector=c_alloc(buff,fb(v),fs(v));
    }    

    /* Check whether the new root has a long span of trailing silence.
     */
    i_silence_test(root);

#if TRACE_PARANOIA & 2
    fprintf(stderr,
	    "* Assigning fragment [%ld-%ld] to root, silencebegin=%ld\n",
	    rb(root), re(root), root->silencebegin);
#endif

    return(1);
  } else
    return(0);
}

static int 
vsort(const void *a,const void *b)
{
  return((*(v_fragment_t **)a)->begin-(*(v_fragment_t **)b)->begin);
}


/* ===========================================================================
 * i_stage2 (internal)
 *
 * This function attempts to extend the verified root by merging verified
 * fragments into it.  It keeps extending the tail end of the root until
 * it runs out of matching fragments.  See i_stage2_each (and
 * i_iterate_stage2) for details of fragment matching and merging.
 *
 * This function is called by paranoia_read_limited when the verified root
 * doesn't contain sufficient data to satisfy the request for samples.
 * If this function fails to extend the verified root far enough (having
 * exhausted the currently available verified fragments), the caller
 * will then read the device again to try and establish more verified
 * fragments.
 *
 * We first try to merge all the fragments in ascending order using the
 * standard method (i_stage2_each()), and then we try to merge the
 * remaining fragments using silence matching (i_silence_match())
 * if the root has a long span of trailing silence.  See the initial
 * comments on silence and  i_silence_match() for an explanation of this
 * distinction.
 *
 * This function returns the number of verified fragments successfully
 * merged into the verified root.
 */
static int 
i_stage2(cdrom_paranoia_t *p, long int beginword, long int endword,
	 void (*callback)(long int, paranoia_cb_mode_t))
{

  int flag=1,ret=0;
  root_block *root=&(p->root);

#ifdef NOISY
  fprintf(stderr,"Fragments:%ld\n",p->fragments->active);
  fflush(stderr);
#endif

  /* even when the 'silence flag' is lit, we try to do non-silence
     matching in the event that there are still audio vectors with
     content to be sunk before the silence */

  /* This flag is not the silence flag.  Rather, it indicates whether
   * we succeeded in adding a verified fragment to the verified root.
   * In short, we keep adding fragments until we no longer find a
   * match.
   */
  while (flag) {

    /* Convert the linked list of verified fragments into an array,
     * to be sorted in order of beginning sample position
     */
    v_fragment_t *first=v_first(p);
    long active=p->fragments->active,count=0;
    v_fragment_t **list = calloc(active, sizeof(v_fragment_t *));

    while (first){
      v_fragment_t *next=v_next(first);
      list[count++]=first;
      first=next;
    }

    /* Reset the flag so that if we don't match any fragments, we
     * stop looping.  Then, proceed only if there are any fragments
     * to match.
     */
    flag=0;
    if (count){

      /* Sort the array of verified fragments in order of beginning
       * sample position.
       */
      qsort(list,active,sizeof(v_fragment_t *),&vsort);

      /* We don't check for the silence flag yet, because even if the
       * verified root ends in silence (and thus the silence flag is set),
       * there may be a non-silent region at the beginning of the verified
       * root, into which we can merge the verified fragments.
       */

      /* Iterate through the verified fragments, starting at the fragment
       * with the lowest beginning sample position.
       */
      for(count=0;count<active;count++){
	first=list[count];

	/* Make sure this fragment hasn't already been merged (and
	 * thus freed). */
	if (first->one){

	  /* If we don't have a verified root yet, just promote the first
	   * fragment (with lowest beginning sample) to be the verified
	   * root.
	   *
	   * ??? It seems that this could be fairly arbitrary if jitter
	   * is an issue.  If we've verified two fragments allegedly
	   * beginning at "0" (which are actually slightly offset due to
	   * jitter), the root might not begin at the earliest read
	   * sample.  Additionally, because subsequent fragments are
	   * only merged at the tail end of the root, this situation
	   * won't be fixed by merging the earlier samples.
	   *
	   * Practically, this ends up not being critical since most
	   * drives insert some extra silent samples at the beginning
	   * of the stream.  Missing a few of them doesn't cause any
	   * real lost data.  But it is non-deterministic.
	   */
	  if (rv(root)==NULL){
	    if (i_init_root(&(p->root),first,beginword,callback)){
	      free_v_fragment(first);

	      /* Consider this a merged fragment, so set the flag
	       * to keep looping.
	       */
	      flag=1;
	      ret++;
	    }
	  } else {

	    /* Try to merge this fragment with the verified root,
	     * extending the tail of the root.
	     */
	    if (i_stage2_each(root,first,callback)){

	      /* If we successfully merged the fragment, set the flag
	       * to keep looping.
	       */
	      ret++;
	      flag=1;
	    }
	  }
	}
      }

      /* If the verified root ends in a long span of silence, iterate
       * through the remaining unmerged fragments to see if they can be
       * merged using our special silence matching.
       */
      if (!flag && p->root.silenceflag){
	for(count=0;count<active;count++){
	  first=list[count];

	  /* Make sure this fragment hasn't already been merged (and
	   * thus freed). */
	  if (first->one){
	    if (rv(root)!=NULL){

	      /* Try to merge the fragment into the root.  This will only
	       * succeed if the fragment overlaps and begins with sufficient
	       * silence to be a presumed match.
	       *
	       * Note that the fragments must be passed to i_silence_match()
	       * in ascending order, as they are here.
	       */
	      if (i_silence_match(root,first,callback)){

		/* If we successfully merged the fragment, set the flag
		 * to keep looping.
		 */
		ret++;
		flag=1;
	      }
	    }
	  }
	} /* end for */
      }
    } /* end if(count) */
    free(list);

    /* If we were able to extend the verified root at all during this pass
     * through the loop, loop again to see if we can merge any remaining
     * fragments with the extended root.
     */

#if TRACE_PARANOIA & 2
    if (flag)
      fprintf(stderr,
	      "- Root updated, comparing remaining fragments again.\n");
#endif

  } /* end while */

  /* Return the number of fragments we successfully merged into the
   * verified root.
   */
  return(ret);
}


static void 
i_end_case(cdrom_paranoia_t *p,long endword, 
	   void(*callback)(long int, paranoia_cb_mode_t))
{

  root_block *root=&p->root;

  /* have an 'end' flag; if we've just read in the last sector in a
     session, set the flag.  If we verify to the end of a fragment
     which has the end flag set, we're done (set a done flag).  Pad
     zeroes to the end of the read */
  
  if (root->lastsector==0)return;
  if (endword<re(root))return;
  
  {
    long addto=endword-re(root);
    char *temp=calloc(addto,sizeof(char)*2);

    c_append(rc(root),(void *)temp,addto);
    free(temp);

    /* trash da cache */
    paranoia_resetcache(p);

  }
}

/* We want to add a sector. Look through the caches for something that
   spans.  Also look at the flags on the c_block... if this is an
   obliterated sector, get a bit of a chunk past the obliteration. */

/* Not terribly smart right now, actually.  We can probably find
   *some* match with a cache block somewhere.  Take it and continue it
   through the skip */

static void 
verify_skip_case(cdrom_paranoia_t *p,
		 void(*callback)(long int, paranoia_cb_mode_t))
{

  root_block *root=&(p->root);
  c_block_t *graft=NULL;
  int vflag=0;
  int gend=0;
  long post;
  
#ifdef NOISY
	fprintf(stderr,"\nskipping\n");
#endif

  if (rv(root)==NULL){
    post=0;
  } else {
    post=re(root);
  }
  if (post==-1)post=0;

  if (callback)(*callback)(post,PARANOIA_CB_SKIP);
  
#if TRACE_PARANOIA
  fprintf(stderr, "Skipping [%ld-", post);
#endif

  /* We want to add a sector.  Look for a c_block that spans,
     preferrably a verified area */

  {
    c_block_t *c=c_first(p);
    while (c){
      long cbegin=cb(c);
      long cend=ce(c);
      if (cbegin<=post && cend>post){
	long vend=post;

	if (c->flags[post-cbegin]&FLAGS_VERIFIED){
	  /* verified area! */
	  while (vend<cend && (c->flags[vend-cbegin]&FLAGS_VERIFIED))vend++;
	  if (!vflag || vend>vflag){
	    graft=c;
	    gend=vend;
	  }
	  vflag=1;
	} else {
	  /* not a verified area */
	  if (!vflag){
	    while (vend<cend && (c->flags[vend-cbegin]&FLAGS_VERIFIED)==0)vend++;
	    if (graft==NULL || gend>vend){
	      /* smallest unverified area */
	      graft=c;
	      gend=vend;
	    }
	  }
	}
      }
      c=c_next(c);
    }

    if (graft){
      long cbegin=cb(graft);
      long cend=ce(graft);

      while (gend<cend && (graft->flags[gend-cbegin]&FLAGS_VERIFIED))gend++;
      gend=min(gend+OVERLAP_ADJ,cend);

      if (rv(root)==NULL){
	int16_t *buff=malloc(cs(graft));
	memcpy(buff,cv(graft),cs(graft));
	rc(root)=c_alloc(buff,cb(graft),cs(graft));
      } else {
	c_append(rc(root),cv(graft)+post-cbegin,
		 gend-post);
      }

#if TRACE_PARANOIA
      fprintf(stderr, "%d], filled with %s data from block [%ld-%ld]\n",
	      gend, (graft->flags[post-cbegin]&FLAGS_VERIFIED)
	      ? "verified" : "unverified", cbegin, cend);
#endif

      root->returnedlimit=re(root);
      return;
    }
  }

  /* No?  Fine.  Great.  Write in some zeroes :-P */
  {
    void *temp=calloc(CDIO_CD_FRAMESIZE_RAW,sizeof(int16_t));

    if (rv(root)==NULL){
      rc(root)=c_alloc(temp,post,CDIO_CD_FRAMESIZE_RAW);
    } else {
      c_append(rc(root),temp,CDIO_CD_FRAMESIZE_RAW);
      free(temp);
    }

#if TRACE_PARANOIA
    fprintf(stderr, "%ld], filled with zero\n", re(root));
#endif

    root->returnedlimit=re(root);
  }
}    

/**** toplevel ****************************************/

void 
paranoia_free(cdrom_paranoia_t *p)
{
  paranoia_resetall(p);
  sort_free(p->sortcache);
  free_list(p->cache, 1);
  free_list(p->fragments, 1);
  free(p);
}

/*! 
  Set the kind of repair you want to on for reading. 
  The modes are listed above
  
  @param p       paranoia type
  @mode  mode    paranoia mode flags built from values in 
  paranoia_mode_t, e.g. 
  PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP
*/
void 
paranoia_modeset(cdrom_paranoia_t *p, int mode_flags)
{
  p->enable=mode_flags;
}

/*!
  reposition reading offset. 
  
  @param p       paranoia type
  @param seek    byte offset to seek to
  @param whence  like corresponding parameter in libc's lseek, e.g. 
  SEEK_SET or SEEK_END.
*/
lsn_t
paranoia_seek(cdrom_paranoia_t *p, off_t seek, int whence)
{
  long sector;
  long ret;
  switch(whence){
  case SEEK_SET:
    sector=seek;
    break;
  case SEEK_END:
    sector=cdda_disc_lastsector(p->d)+seek;
    break;
  default:
    sector=p->cursor+seek;
    break;
  }
  
  if (cdda_sector_gettrack(p->d,sector)==-1)return(-1);

  i_cblock_destructor(p->root.vector);
  p->root.vector=NULL;
  p->root.lastsector=0;
  p->root.returnedlimit=0;

  ret=p->cursor;
  p->cursor=sector;

  i_paranoia_firstlast(p);
  
  /* Evil hack to fix pregap patch for NEC drives! To be rooted out in a10 */
  p->current_firstsector=sector;

  return(ret);
}



/* ===========================================================================
 * read_c_block() (internal)
 *
 * This funtion reads many (p->readahead) sectors, encompassing at least
 * the requested words.
 *
 * It returns a c_block which encapsulates these sectors' data and sector
 * number.  The sectors come come from multiple low-level read requests.
 *
 * This function reads many sectors in order to exhaust any caching on the
 * drive itself, as caching would simply return the same incorrect data
 * over and over.  Paranoia depends on truly re-reading portions of the
 * disc to make sure the reads are accurate and correct any inaccuracies.
 *
 * Which precise sectors are read varies ("jiggles") between calls to
 * read_c_block, to prevent consistent errors across multiple reads
 * from being misinterpreted as correct data.
 *
 * The size of each low-level read is determined by the underlying driver
 * (p->d->nsectors), which allows the driver to specify how many sectors
 * can be read in a single request.  Historically, the Linux kernel could
 * only read 8 sectors at a time, with likely dropped samples between each
 * read request.  Other operating systems may have different limitations.
 *
 * This function is called by paranoia_read_limited(), which breaks the
 * c_block of read data into runs of samples that are likely to be
 * contiguous, verifies them and stores them in verified fragments, and
 * eventually merges the fragments into the verified root.
 *
 * This function returns the last c_block read or NULL on error.
 */

static c_block_t *
i_read_c_block(cdrom_paranoia_t *p,long beginword,long endword,
	       void(*callback)(long, paranoia_cb_mode_t))
{

/* why do it this way?  We need to read lots of sectors to kludge
   around stupid read ahead buffers on cheap drives, as well as avoid
   expensive back-seeking. We also want to 'jiggle' the start address
   to try to break borderline drives more noticeably (and make broken
   drives with unaddressable sectors behave more often). */
      
  long readat,firstread;
  long totaltoread=p->readahead;
  long sectatonce=p->d->nsectors;
  long driftcomp=(float)p->dyndrift/CD_FRAMEWORDS+.5;
  c_block_t *new=NULL;
  root_block *root=&p->root;
  int16_t *buffer=NULL;
  unsigned char *flags=NULL;
  long sofar;
  long dynoverlap=(p->dynoverlap+CD_FRAMEWORDS-1)/CD_FRAMEWORDS; 
  long anyflag=0;


  /* Calculate the first sector to read.  This calculation takes
   * into account the need to jitter the starting point of the read
   * to reveal consistent errors as well as the low reliability of
   * the edge words of a read.
   *
   * ???: Document more clearly how dynoverlap and MIN_SECTOR_BACKUP
   * are calculated and used.
   */

  /* What is the first sector to read?  want some pre-buffer if
     we're not at the extreme beginning of the disc */
  
  if (p->enable&(PARANOIA_MODE_VERIFY|PARANOIA_MODE_OVERLAP)){
    
    /* we want to jitter the read alignment boundary */
    long target;
    if (rv(root)==NULL || rb(root)>beginword)
      target=p->cursor-dynoverlap; 
    else
      target=re(root)/(CD_FRAMEWORDS)-dynoverlap;
	
    if (target+MIN_SECTOR_BACKUP>p->lastread && target<=p->lastread)
      target=p->lastread-MIN_SECTOR_BACKUP;

    /* we want to jitter the read alignment boundary, as some
       drives, beginning from a specific point, will tend to
       lose bytes between sectors in the same place.  Also, as
       our vectors are being made up of multiple reads, we want
       the overlap boundaries to move.... */
    
    readat=(target&(~((long)JIGGLE_MODULO-1)))+p->jitter;
    if (readat>target)readat-=JIGGLE_MODULO;
    p->jitter++;
    if (p->jitter>=JIGGLE_MODULO)
      p->jitter=0;
     
  } else {
    readat=p->cursor; 
  }
  
  readat+=driftcomp;

  /* Create a new, empty c_block and add it to the head of the
   * list of c_blocks in memory.  It will be empty until the end of
   * this subroutine.
   */
  if (p->enable&(PARANOIA_MODE_OVERLAP|PARANOIA_MODE_VERIFY)) {
    flags=calloc(totaltoread*CD_FRAMEWORDS, 1);
    new=new_c_block(p);
    recover_cache(p);
  } else {
    /* in the case of root it's just the buffer */
    paranoia_resetall(p);	
    new=new_c_block(p);
  }

  buffer=calloc(totaltoread*CDIO_CD_FRAMESIZE_RAW, 1);
  sofar=0;
  firstread=-1;

#if TRACE_PARANOIA
  fprintf(stderr, "Reading [%ld-%ld] from media\n",
	  readat*CD_FRAMEWORDS, (readat+totaltoread)*CD_FRAMEWORDS);
#endif

  /* Issue each of the low-level reads until we've read enough sectors
   * to exhaust the drive's cache.
   *
   * p->readahead   = total number of sectors to read
   * p->d->nsectors = number of sectors to read per request
   *
   * The driver determines this latter number, which is the maximum
   * number of sectors the kernel can reliably read per request.  In
   * old Linux kernels, there was a hard limit of 8 sectors per read.
   * While this limit has since been removed, certain motherboards
   * can't handle DMA requests larger than 64K.  And other operating
   * systems may have similar limitations.  So the method of splitting
   * up reads is still useful.
   */

  /* actual read loop */

  while (sofar<totaltoread){
    long secread=sectatonce;  /* number of sectors to read this request */
    long adjread=readat;      /* first sector to read for this request */
    long thisread;            /* how many sectors were read this request */

    /* don't under/overflow the audio session */
    if (adjread<p->current_firstsector){
      secread-=p->current_firstsector-adjread;
      adjread=p->current_firstsector;
    }
    if (adjread+secread-1>p->current_lastsector)
      secread=p->current_lastsector-adjread+1;
    
    if (sofar+secread>totaltoread)secread=totaltoread-sofar;
    
    if (secread>0){
      
      if (firstread<0) firstread = adjread;

      /* Issue the low-level read to the driver.
       */

      thisread = cdda_read(p->d, buffer+sofar*CD_FRAMEWORDS, adjread, secread);

#if TRACE_PARANOIA & 1
      fprintf(stderr, "- Read [%ld-%ld] (0x%04X...0x%04X)%s",
	      adjread*CD_FRAMEWORDS, (adjread+thisread)*CD_FRAMEWORDS,
	      buffer[sofar*CD_FRAMEWORDS] & 0xFFFF,
	      buffer[(sofar+thisread)*CD_FRAMEWORDS - 1] & 0xFFFF,
	      thisread < secread ? "" : "\n");
#endif

      /* If the low-level read returned too few sectors, pad the result
       * with null data and mark it as invalid (FLAGS_UNREAD).  We pad
       * because we're going to be appending further reads to the current
       * c_block.
       *
       * ???: Why not re-read?  It might be to keep you from getting
       * hung up on a bad sector.  Or it might be to avoid interrupting
       * the streaming as much as possible.
       */
      if ( thisread < secread) {

	if (thisread<0) thisread=0;

#if TRACE_PARANOIA & 1
	fprintf(stderr, " -- couldn't read [%ld-%ld]\n",
		(adjread+thisread)*CD_FRAMEWORDS,
		(adjread+secread)*CD_FRAMEWORDS);
#endif

	/* Uhhh... right.  Make something up. But don't make us seek
           backward! */

	if (callback)
	  (*callback)((adjread+thisread)*CD_FRAMEWORDS, PARANOIA_CB_READERR);  
	memset(buffer+(sofar+thisread)*CD_FRAMEWORDS,0,
	       CDIO_CD_FRAMESIZE_RAW*(secread-thisread));
	if (flags)
          memset(flags+(sofar+thisread)*CD_FRAMEWORDS, FLAGS_UNREAD,
	         CD_FRAMEWORDS*(secread-thisread));
      }
      if (thisread!=0)anyflag=1;


      /* Because samples are likely to be dropped between read requests,
       * mark the samples near the the boundaries of the read requests
       * as suspicious (FLAGS_EDGE).  This means that any span of samples
       * against which these adjacent read requests are compared must
       * overlap beyond the edges and into the more trustworthy data.
       * Such overlapping spans are accordingly at least MIN_WORDS_OVERLAP
       * words long (and naturally longer if any samples were dropped
       * between the read requests).
       *
       *          (EEEEE...overlapping span...EEEEE)
       * (read 1 ...........EEEEE)   (EEEEE...... read 2 ......EEEEE) ...
       *         dropped samples --^
       */
      if (flags && sofar!=0){
	/* Don't verify across overlaps that are too close to one
           another */
	int i=0;
	for(i=-MIN_WORDS_OVERLAP/2;i<MIN_WORDS_OVERLAP/2;i++)
	  flags[sofar*CD_FRAMEWORDS+i]|=FLAGS_EDGE;
      }


      /* Move the read cursor ahead by the number of sectors we attempted
       * to read.
       *
       * ???: Again, why not move it ahead by the number actually read?
       */
      p->lastread=adjread+secread;
      
      if (adjread+secread-1==p->current_lastsector)
	new->lastsector=-1;
      
      if (callback)(*callback)((adjread+secread-1)*CD_FRAMEWORDS,PARANOIA_CB_READ);
      
      sofar+=secread;
      readat=adjread+secread; 
    } else /* secread <= 0 */
      if (readat<p->current_firstsector)
	readat+=sectatonce; /* due to being before the readable area */
      else
	break; /* due to being past the readable area */


    /* Keep issuing read requests until we've read enough sectors to
     * exhaust the drive's cache.
     */

  } /* end while */


  /* If we managed to read any sectors at all (anyflag), fill in the
   * previously allocated c_block with the read data.  Otherwise, free
   * our buffers, dispose of the c_block, and return NULL.
   */
  if (anyflag) {
    new->vector=buffer;
    new->begin=firstread*CD_FRAMEWORDS-p->dyndrift;
    new->size=sofar*CD_FRAMEWORDS;
    new->flags=flags;

#if TRACE_PARANOIA
    fprintf(stderr, "- Read block %ld:[%ld-%ld] from media\n",
	    p->cache->active, cb(new), ce(new));
#endif
  } else {
    if (new)free_c_block(new);
    free(buffer);
    free(flags);
    new=NULL;
  }
  return(new);
}


/** ==========================================================================
 * cdio_paranoia_read(), cdio_paranoia_read_limited()
 *
 * These functions "read" the next sector of audio data and returns
 * a pointer to a full sector of verified samples (2352 bytes).
 *
 * The returned buffer is *not* to be freed by the caller.  It will
 *   persist only until the next call to paranoia_read() for this p 
*/

int16_t *
cdio_paranoia_read(cdrom_paranoia_t *p, 
		   void(*callback)(long, paranoia_cb_mode_t))
{
  return paranoia_read_limited(p, callback, 20);
}

/* I added max_retry functionality this way in order to avoid
   breaking any old apps using the nerw libs.  cdparanoia 9.8 will
   need the updated libs, but nothing else will require it. */
int16_t *
cdio_paranoia_read_limited(cdrom_paranoia_t *p, 
			   void(*callback)(long int, paranoia_cb_mode_t),
			   int max_retries)
{
  long int beginword  =  p->cursor*(CD_FRAMEWORDS);
  long int endword    =  beginword+CD_FRAMEWORDS;
  long int retry_count=  0;
  long int lastend    = -2;
  root_block *root    = &p->root;

  if (beginword > p->root.returnedlimit)
    p->root.returnedlimit=beginword;
  lastend=re(root);


  /* Since paranoia reads and verifies chunks of data at a time
   * (which it needs to counteract dropped samples and inaccurate
   * seeking), the requested samples may already be in memory,
   * in the verified "root".
   *
   * The root is where paranoia stores samples that have been
   * verified and whose position has been accurately determined.
   */
  
  /* First, is the sector we want already in the root? */
  while (rv(root)==NULL ||
	rb(root)>beginword || 
	(re(root)<endword+(MAX_SECTOR_OVERLAP*CD_FRAMEWORDS) &&
	 p->enable&(PARANOIA_MODE_VERIFY|PARANOIA_MODE_OVERLAP)) ||
	re(root)<endword){
    
    /* Nope; we need to build or extend the root verified range */

#if TRACE_PARANOIA
    fprintf(stderr, "Trying to expand root [%ld-%ld]...\n",
	    rb(root), re(root));
#endif

    /* We may have already read the necessary samples and placed
     * them into verified fragments, but not yet merged them into
     * the verified root.  We'll check that before we actually
     * try to read data from the drive.
     */

    if (p->enable&(PARANOIA_MODE_VERIFY|PARANOIA_MODE_OVERLAP)){

      /* We need to make sure our memory consumption doesn't grow
       * to the size of the whole CD.  But at the same time, we
       * need to hang onto some of the verified data (even perhaps
       * data that's already been returned by paranoia_read()) in
       * order to verify and accurately position future samples.
       *
       * Therefore, we free some of the verified data that we
       * no longer need.
       */
      i_paranoia_trim(p,beginword,endword);
      recover_cache(p);

      if (rb(root)!=-1 && p->root.lastsector)
	i_end_case(p, endword+(MAX_SECTOR_OVERLAP*CD_FRAMEWORDS),
			callback);
      else

	/* Merge as many verified fragments into the verified root
	 * as we need to satisfy the pending request.  We may
	 * not have all the fragments we need, in which case we'll
	 * read data from the CD further below.
	 */
	i_stage2(p, beginword,
		      endword+(MAX_SECTOR_OVERLAP*CD_FRAMEWORDS),
		      callback);
    } else
      i_end_case(p,endword+(MAX_SECTOR_OVERLAP*CD_FRAMEWORDS),
		 callback); /* only trips if we're already done */

#if TRACE_PARANOIA
    fprintf(stderr, "- Root is now [%ld-%ld] silencebegin=%ld\n",
	    rb(root), re(root), root->silencebegin);
#endif

    /* If we were able to fill the verified root with data already
     * in memory, we don't need to read any more data from the drive.
     */
    if (!(rb(root)==-1 || rb(root)>beginword || 
	 re(root)<endword+(MAX_SECTOR_OVERLAP*CD_FRAMEWORDS))) 
      break;
    
    /* Hmm, need more.  Read another block */

    {
      /* Read many sectors, encompassing at least the requested words.
       *
       * The returned c_block encapsulates these sectors' data and
       * sector number.  The sectors come come from multiple low-level
       * read requests, and words which were near the boundaries of
       * those read requests are marked with FLAGS_EDGE.
       */
      c_block_t *new=i_read_c_block(p,beginword,endword,callback);
      
      if (new){
	if (p->enable&(PARANOIA_MODE_OVERLAP|PARANOIA_MODE_VERIFY)){
      
	  /* If we need to verify these samples, send them to
	   * stage 1 verification, which will add verified samples
	   * to the set of verified fragments.  Verified fragments
	   * will be merged into the verified root during stage 2
	   * overlap analysis.
	   */
	  if (p->enable&PARANOIA_MODE_VERIFY)
	    i_stage1(p,new,callback);

	  /* If we're only doing overlapping reads (no stage 1
	   * verification), consider each low-level read in the
	   * c_block to be a verified fragment.  We exclude the
	   * edges from these fragments to enforce the requirement
	   * that we overlap the reads by the minimum amount.
	   * These fragments will be merged into the verified
	   * root during stage 2 overlap analysis.
	   */
	  else{
	    /* just make v_fragments from the boundary information. */
	    long begin=0,end=0;
	    
	    while (begin<cs(new)){
	      /* ???BUG??? This while() should probably read begin<cs(new).
	       *
	       * This bug is not fixed yet.
	       */
	      while (end<cs(new) && (new->flags[begin]&FLAGS_EDGE))begin++;
	      end=begin+1;
	      while (end<cs(new) && (new->flags[end]&FLAGS_EDGE)==0)end++;
	      {
		new_v_fragment(p,new,begin+cb(new),
			       end+cb(new),
			       (new->lastsector && cb(new)+end==ce(new)));
	      }
	      begin=end;
	    }
	  }
	  
	} else {

	  /* If we're not doing any overlapping reads or verification
	   * of data, skip over the stage 1 and stage 2 verification and
	   * promote this c_block directly to the current "verified" root.
	   */

	  if (p->root.vector)i_cblock_destructor(p->root.vector);
	  free_elem(new->e,0);
	  p->root.vector=new;

	  i_end_case(p,endword+(MAX_SECTOR_OVERLAP*CD_FRAMEWORDS),
			  callback);
      
	}
      }
    }

    /* Are we doing lots of retries?  **************************************/

    /* ???: To be studied
     */

    /* Check unaddressable sectors first.  There's no backoff here; 
       jiggle and minimum backseek handle that for us */
    
    if (rb(root)!=-1 && lastend+588<re(root)){ /* If we've not grown
						 half a sector */
      lastend=re(root);
      retry_count=0;
    } else {
      /* increase overlap or bail */
      retry_count++;
      
      /* The better way to do this is to look at how many actual
	 matches we're getting and what kind of gap */

      if (retry_count%5==0){
	if (p->dynoverlap==MAX_SECTOR_OVERLAP*CD_FRAMEWORDS ||
	   retry_count==max_retries){
	  if (!(p->enable&PARANOIA_MODE_NEVERSKIP))
	    verify_skip_case(p,callback);
	  retry_count=0;
	} else {
	  if (p->stage1.offpoints!=-1){ /* hack */
	    p->dynoverlap*=1.5;
	    if (p->dynoverlap>MAX_SECTOR_OVERLAP*CD_FRAMEWORDS)
	      p->dynoverlap=MAX_SECTOR_OVERLAP*CD_FRAMEWORDS;
	    if (callback)
	      (*callback)(p->dynoverlap,PARANOIA_CB_OVERLAP);
	  }
	}
      }
    }

    /* Having read data from the drive and placed it into verified
     * fragments, we now loop back to try to extend the root with
     * the newly loaded data.  Alternatively, if the root already
     * contains the needed data, we'll just fall through.
     */

  } /* end while */
  p->cursor++;

  /* Return a pointer into the verified root.  Thus, the caller
   * must NOT free the returned pointer!
   */
  return(rv(root)+(beginword-rb(root)));
}

/* a temporary hack */
void 
cdio_paranoia_overlapset(cdrom_paranoia_t *p, long int overlap)
{
  p->dynoverlap=overlap*CD_FRAMEWORDS;
  p->stage1.offpoints=-1; 
}
