/*
  $Id: gap.c,v 1.3 2005/11/08 23:21:40 pjcreath Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
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
 * Gap analysis support code for paranoia
 *
 ***/

#include "config.h"
#include "p_block.h"
#include <cdio/paranoia.h>
#include "gap.h"
#include <string.h>

/**** Gap analysis code ***************************************************/

/* ===========================================================================
 * i_paranoia_overlap_r (internal)
 *
 * This function seeks backward through two vectors (starting at the given
 * offsets) to determine how many consecutive samples agree.  It returns
 * the number of matching samples, which may be 0.
 *
 * Unlike its sibling, i_paranoia_overlap_f, this function doesn't need to
 * be given the size of the vectors (all vectors stop at offset 0).
 *
 * This function is used by i_analyze_rift_r() below to find where a
 * leading rift ends.
 */
long int
i_paranoia_overlap_r(int16_t *buffA,int16_t *buffB,
			  long offsetA, long offsetB)
{
  long beginA=offsetA;
  long beginB=offsetB;

  /* Start at the given offsets and work our way backwards until we hit
   * the beginning of one of the vectors.
   */
  for( ; beginA>=0 && beginB>=0; beginA--,beginB-- )
    if (buffA[beginA] != buffB[beginB]) break;

  /* These values will either point to the first mismatching sample, or
   * -1 if we hit the beginning of a vector.  So increment to point to the
   * last matching sample.
   *
   * ??? Why?  This would appear to return one less sample than actually
   * matched.  E.g., no matching samples returns -1!  Is this a bug?
   */
  beginA++;
  beginB++;
  
  return(offsetA-beginA);
}


/* ===========================================================================
 * i_paranoia_overlap_f (internal)
 *
 * This function seeks forward through two vectors (starting at the given
 * offsets) to determine how many consecutive samples agree.  It returns
 * the number of matching samples, which may be 0.
 *
 * Unlike its sibling, i_paranoia_overlap_r, this function needs to given
 * the size of the vectors.
 *
 * This function is used by i_analyze_rift_f() below to find where a
 * trailing rift ends.
 */
long int 
i_paranoia_overlap_f(int16_t *buffA,int16_t *buffB,
		     long offsetA, long offsetB,
		     long sizeA,long sizeB)
{
  long endA=offsetA;
  long endB=offsetB;
  
  /* Start at the given offsets and work our way forward until we hit
   * the end of one of the vectors.
   */
  for(;endA<sizeA && endB<sizeB;endA++,endB++)
    if(buffA[endA]!=buffB[endB])break;
  
  /* ??? Note that we don't do any post-loop tweaking of endA.  Why the
   * asymmetry with i_paranoia_overlap_r?
   */

  return(endA-offsetA);
}


/* ===========================================================================
 * i_stutter_or_gap (internal)
 *
 * This function compares (gap) samples of two vectors at the given offsets.
 * It returns 0 if all the samples are identical, or nonzero if they differ.
 *
 * This is used by i_analyze_rift_[rf] below to determine whether a rift
 * contains samples dropped by the other vector (that should be inserted),
 * or whether the rift contains a stutter (that should be dropped).  See
 * i_analyze_rift_[rf] for more details.
 */
int 
i_stutter_or_gap(int16_t *A, int16_t *B,long offA, long offB, long int gap)
{
  long a1=offA;
  long b1=offB;
  
  /* If the rift was so big that there aren't enough samples in the other
   * vector to compare against the full gap, then just compare what we
   * have available.  E.g.:
   *
   *            (5678)|(newly matching run ...)
   *    (... 12345678)| (345678) |(newly matching run ...)
   *
   * In this case, a1 would be -2, since we'd want to compare 6 samples
   * against a vector that had only 4.  So we start 2 samples later, and
   * compare the 4 available samples.
   *
   * Again, this approach to identifying stutters is simply a heuristic,
   * so this may not produce correct results in all cases.
   */
  if(a1<0){
    /* Note that a1 is negative, so we're increasing b1 and decreasing (gap).
     */
    b1-=a1;
    gap+=a1;
    a1=0;
  }

  /* Note that we don't have an equivalent adjustment for leading rifts.
   * Thus, it's possible for the following memcmp() to run off the end
   * of A.  See the bug note in i_analyze_rift_r().
   */
  
  /* Multiply gap by 2 because samples are 2 bytes long and memcmp compares
   * at the byte level.
   */
  return(memcmp(A+a1,B+b1,gap*2));
}

/* riftv is the first value into the rift -> or <- */


/* ===========================================================================
 * i_analyze_rift_f (internal)
 *
 * This function examines a trailing rift to see how far forward the rift goes
 * and to determine what kind of rift it is.  This function is called by
 * i_stage2_each() when a trailing rift is detected.  (aoffset,boffset) are
 * the offsets into (A,B) of the first mismatching sample.
 *
 * This function returns:
 *  matchA  > 0 if there are (matchA) samples missing from A
 *  matchA  < 0 if there are (-matchA) duplicate samples (stuttering) in A
 *  matchB  > 0 if there are (matchB) samples missing from B
 *  matchB  < 0 if there are (-matchB) duplicate samples in B
 *  matchC != 0 if there are (matchC) samples of garbage, after which
 *              both A and B are in sync again
 */
void 
i_analyze_rift_f(int16_t *A,int16_t *B,
		 long sizeA, long sizeB,
		 long aoffset, long boffset, 
		 long *matchA,long *matchB,long *matchC)
{
  
  long apast=sizeA-aoffset;
  long bpast=sizeB-boffset;
  long i;
  
  *matchA=0, *matchB=0, *matchC=0;
  
  /* Look forward to see where we regain agreement between vectors
   * A and B (of at least MIN_WORDS_RIFT samples).  We look for one of
   * the following possible matches:
   * 
   *                         edge
   *                          v
   * (1)  (... A matching run)|(aoffset matches ...)
   *      (... B matching run)| (rift) |(boffset+i matches ...)
   *
   * (2)  (... A matching run)| (rift) |(aoffset+i matches ...)
   *      (... B matching run)|(boffset matches ...)
   *
   * (3)  (... A matching run)| (rift) |(aoffset+i matches ...)
   *      (... B matching run)| (rift) |(boffset+i matches ...)
   *
   * Anything that doesn't match one of these three is too corrupt to
   * for us to recover from.  E.g.:
   *
   *      (... A matching run)| (rift) |(eventual match ...)
   *      (... B matching run)| (big rift) |(eventual match ...)
   *
   * We won't find the eventual match, since we wouldn't be sure how
   * to fix the rift.
   */
  
  for(i=0;;i++){
    /* Search for whatever case we hit first, so as to end up with the
     * smallest rift.
     *
     * ??? Why do we start at 0?  It should never match.
     */

    /* Don't search for (1) past the end of B */
    if (i<bpast)

      /* See if we match case (1) above, which either means that A dropped
       * samples at the rift, or that B stuttered.
       */
      if(i_paranoia_overlap_f(A,B,aoffset,boffset+i,sizeA,sizeB)>=MIN_WORDS_RIFT){
	*matchA=i;
	break;
      }
    
    /* Don't search for (2) or (3) past the end of A */
    if (i<apast) {

      /* See if we match case (2) above, which either means that B dropped
       * samples at the rift, or that A stuttered.
       */
      if(i_paranoia_overlap_f(A,B,aoffset+i,boffset,sizeA,sizeB)>=MIN_WORDS_RIFT){
	*matchB=i;
	break;
      }

      /* Don't search for (3) past the end of B */
      if (i<bpast)

        /* See if we match case (3) above, which means that a fixed-length
         * rift of samples is getting read unreliably.
         */
	if(i_paranoia_overlap_f(A,B,aoffset+i,boffset+i,sizeA,sizeB)>=MIN_WORDS_RIFT){
	  *matchC=i;
	  break;
	}
    }else

      /* Stop searching when we've reached the end of both vectors.
       * In theory we could stop when there aren't MIN_WORDS_RIFT samples
       * left in both vectors, but this case should happen fairly rarely.
       */
      if(i>=bpast)break;
    
    /* Try the search again with a larger tentative rift. */
  }
  
  if(*matchA==0 && *matchB==0 && *matchC==0)return;
  
  if(*matchC)return;

  /* For case (1) or (2), we need to determine whether the rift contains
   * samples dropped by the other vector (that should be inserted), or
   * whether the rift contains a stutter (that should be dropped).  To
   * distinguish, we check the contents of the rift against the good samples
   * just before the rift.  If the contents match, then the rift contains
   * a stutter.
   *
   * A stutter in the second vector:
   *     (...good samples... 1234)|(567 ...newly matched run...)
   *     (...good samples... 1234)| (1234) | (567 ...newly matched run)
   *
   * Samples missing from the first vector:
   *     (...good samples... 1234)|(901 ...newly matched run...)
   *     (...good samples... 1234)| (5678) |(901 ...newly matched run...)
   *
   * Of course, there's no theoretical guarantee that a non-stutter
   * truly represents missing samples, but given that we're dealing with
   * verified fragments in stage 2, we can have some confidence that this
   * is the case.
   */
  if(*matchA){
    /* For case (1), we need to determine whether A dropped samples at the
     * rift or whether B stuttered.
     *
     * If the rift doesn't match the good samples in A (and hence in B),
     * it's not a stutter, and the rift should be inserted into A.
     */
    if(i_stutter_or_gap(A,B,aoffset-*matchA,boffset,*matchA))
      return;

    /* It is a stutter, so we need to signal that we need to remove
     * (matchA) bytes from B.
     */
    *matchB = -*matchA;
    *matchA=0;
    return;

  }else{
    /* Case (2) is the inverse of case (1) above. */
    if(i_stutter_or_gap(B,A,boffset-*matchB,aoffset,*matchB))
      return;

    *matchA = -*matchB;
    *matchB=0;
    return;
  }
}


/* riftv must be first even val of rift moving back */

/* ===========================================================================
 * i_analyze_rift_r (internal)
 *
 * This function examines a leading rift to see how far back the rift goes
 * and to determine what kind of rift it is.  This function is called by
 * i_stage2_each() when a leading rift is detected.  (aoffset,boffset) are
 * the offsets into (A,B) of the first mismatching sample.
 *
 * This function returns:
 *  matchA  > 0 if there are (matchA) samples missing from A
 *  matchA  < 0 if there are (-matchA) duplicate samples (stuttering) in A
 *  matchB  > 0 if there are (matchB) samples missing from B
 *  matchB  < 0 if there are (-matchB) duplicate samples in B
 *  matchC != 0 if there are (matchC) samples of garbage, after which
 *              both A and B are in sync again
 */
void 
i_analyze_rift_r(int16_t *A,int16_t *B,
		 long sizeA, long sizeB,
		 long aoffset, long boffset, 
		 long *matchA,long *matchB,long *matchC)
{
  
  long apast=aoffset+1;
  long bpast=boffset+1;
  long i;
  
  *matchA=0, *matchB=0, *matchC=0;

  /* Look backward to see where we regain agreement between vectors
   * A and B (of at least MIN_WORDS_RIFT samples).  We look for one of
   * the following possible matches:
   * 
   *                                    edge
   *                                      v
   * (1)             (... aoffset matches)|(A matching run ...)
   *      (... boffset-i matches)| (rift) |(B matching run ...)
   *
   * (2)  (... aoffset-i matches)| (rift) |(A matching run ...)
   *                (... boffset matches)|(B matching run ...)
   *
   * (3)  (... aoffset-i matches)| (rift) |(A matching run ...)
   *      (... boffset-i matches)| (rift) |(B matching run ...)
   *
   * Anything that doesn't match one of these three is too corrupt to
   * for us to recover from.  E.g.:
   *
   *         (... eventual match)| (rift) |(A matching run ...)
   *    (... eventual match) | (big rift) |(B matching run ...)
   *
   * We won't find the eventual match, since we wouldn't be sure how
   * to fix the rift.
   */
  
  for(i=0;;i++){
    /* Search for whatever case we hit first, so as to end up with the
     * smallest rift.
     *
     * ??? Why do we start at 0?  It should never match.
     */

    /* Don't search for (1) past the beginning of B */
    if (i<bpast)

      /* See if we match case (1) above, which either means that A dropped
       * samples at the rift, or that B stuttered.
       */
      if(i_paranoia_overlap_r(A,B,aoffset,boffset-i)>=MIN_WORDS_RIFT){
	*matchA=i;
	break;
      }

    /* Don't search for (2) or (3) past the beginning of A */
    if (i<apast) {

      /* See if we match case (2) above, which either means that B dropped
       * samples at the rift, or that A stuttered.
       */
      if(i_paranoia_overlap_r(A,B,aoffset-i,boffset)>=MIN_WORDS_RIFT){
	*matchB=i;
	break;
      }

      /* Don't search for (3) past the beginning of B */
      if (i<bpast)

	/* See if we match case (3) above, which means that a fixed-length
	 * rift of samples is getting read unreliably.
	 */
	if(i_paranoia_overlap_r(A,B,aoffset-i,boffset-i)>=MIN_WORDS_RIFT){
	  *matchC=i;
	  break;
	}
    }else

      /* Stop searching when we've reached the end of both vectors.
       * In theory we could stop when there aren't MIN_WORDS_RIFT samples
       * left in both vectors, but this case should happen fairly rarely.
       */
      if(i>=bpast)break;

    /* Try the search again with a larger tentative rift. */
  }

  if(*matchA==0 && *matchB==0 && *matchC==0)return;

  if(*matchC)return;

  /* For case (1) or (2), we need to determine whether the rift contains
   * samples dropped by the other vector (that should be inserted), or
   * whether the rift contains a stutter (that should be dropped).  To
   * distinguish, we check the contents of the rift against the good samples
   * just after the rift.  If the contents match, then the rift contains
   * a stutter.
   *
   * A stutter in the second vector:
   *              (...newly matched run... 234)|(5678 ...good samples...)
   *     (...newly matched run... 234)| (5678) |(5678 ...good samples...)
   *
   * Samples missing from the first vector:
   *              (...newly matched run... 890)|(5678 ...good samples...)
   *     (...newly matched run... 890)| (1234) |(5678 ...good samples...)
   *
   * Of course, there's no theoretical guarantee that a non-stutter
   * truly represents missing samples, but given that we're dealing with
   * verified fragments in stage 2, we can have some confidence that this
   * is the case.
   */

  if(*matchA){
    /* For case (1), we need to determine whether A dropped samples at the
     * rift or whether B stuttered.
     *
     * If the rift doesn't match the good samples in A (and hence in B),
     * it's not a stutter, and the rift should be inserted into A.
     *
     * ???BUG??? It's possible for aoffset+1+*matchA to be > sizeA, in
     * which case the comparison in i_stutter_or_gap() will extend beyond
     * the bounds of A.  Thankfully, this isn't writing data and thus
     * trampling memory, but it's still a memory access error that should
     * be fixed.
     *
     * This bug is not fixed yet.
     */
    if(i_stutter_or_gap(A,B,aoffset+1,boffset-*matchA+1,*matchA))
      return;

    /* It is a stutter, so we need to signal that we need to remove
     * (matchA) bytes from B.
     */
    *matchB = -*matchA;
    *matchA=0;
    return;

  }else{
    /* Case (2) is the inverse of case (1) above. */
    if(i_stutter_or_gap(B,A,boffset+1,aoffset-*matchB+1,*matchB))
      return;

    *matchA = -*matchB;
    *matchB=0;
    return;
  }
}


/* ===========================================================================
 * analyze_rift_silence_f (internal)
 *
 * This function examines the fragment and root from the rift onward to
 * see if they have a rift's worth of silence (or if they end with silence).
 * It sets (*matchA) to -1 if A's rift is silence, (*matchB) to -1 if B's
 * rift is silence, and sets them to 0 otherwise.
 *
 * Note that, unlike every other function in cdparanoia, this function
 * considers any repeated value to be silence (which, in effect, it is).
 * All other functions only consider repeated zeroes to be silence.
 *
 * ??? Is this function name just a misnomer, as it's really looking for
 * repeated garbage?
 *
 * This function is called by i_stage2_each() if it runs into a trailing rift
 * that i_analyze_rift_f couldn't diagnose.  This checks for another variant:
 * where one vector has silence and the other doesn't.  We then assume
 * that the silence (and anything following it) is garbage.
 *
 * Note that while this function checks both A and B for silence, the caller
 * assumes that only one or the other has silence.
 */
void
analyze_rift_silence_f(int16_t *A,int16_t *B,long sizeA,long sizeB,
		       long aoffset, long boffset,
		       long *matchA, long *matchB)
{
  *matchA=-1;
  *matchB=-1;

  /* Search for MIN_WORDS_RIFT samples, or to the end of the vector,
   * whichever comes first.
   */
  sizeA=min(sizeA,aoffset+MIN_WORDS_RIFT);
  sizeB=min(sizeB,boffset+MIN_WORDS_RIFT);

  aoffset++;
  boffset++;

  /* Check whether A has only "silence" within the search range.  Note
   * that "silence" here is a single, repeated value (zero or not).
   */
  while(aoffset<sizeA){
    if(A[aoffset]!=A[aoffset-1]){
      *matchA=0;
      break;
    }
    aoffset++;
  }

  /* Check whether B has only "silence" within the search range.  Note
   * that "silence" here is a single, repeated value (zero or not).
   *
   * Also note that while the caller assumes that only matchA or matchB
   * is set, we check both vectors here.
   */
  while(boffset<sizeB){
    if(B[boffset]!=B[boffset-1]){
      *matchB=0;
      break;
    }
    boffset++;
  }
}
