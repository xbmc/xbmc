/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// MP3 internal implementation details (Huffman encoding)
// Implementation

#include "MP3InternalsHuffman.hh"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

MP3HuffmanEncodingInfo
::MP3HuffmanEncodingInfo(Boolean includeDecodedValues) {
  if (includeDecodedValues) {
    decodedValues = new unsigned[(SBLIMIT*SSLIMIT + 1)*4];
  } else {
    decodedValues = NULL;
  }
}

MP3HuffmanEncodingInfo::~MP3HuffmanEncodingInfo() {
  delete[] decodedValues;
}

// This is crufty old code that needs to be cleaned up #####

static unsigned debugCount = 0; /* for debugging */

#define TRUNC_FAVORa

void updateSideInfoForHuffman(MP3SideInfo& sideInfo, Boolean isMPEG2,
			      unsigned char const* mainDataPtr,
			      unsigned p23L0, unsigned p23L1,
			      unsigned& part23Length0a,
			      unsigned& part23Length0aTruncation,
			      unsigned& part23Length0b,
			      unsigned& part23Length0bTruncation,
			      unsigned& part23Length1a,
			      unsigned& part23Length1aTruncation,
			      unsigned& part23Length1b,
			      unsigned& part23Length1bTruncation) {
  int i, j;
  unsigned sfLength, origTotABsize, adjustment;
  MP3SideInfo::gr_info_s_t* gr;

  /* First, Huffman-decode each part of the segment's main data,
     to see at which bit-boundaries the samples appear:
   */
  MP3HuffmanEncodingInfo hei;

  ++debugCount;
#ifdef DEBUG
  fprintf(stderr, "usifh-start: p23L0: %d, p23L1: %d\n", p23L0, p23L1);
#endif

  /* Process granule 0 */
  {
    gr = &(sideInfo.ch[0].gr[0]);
    origTotABsize = gr->part2_3_length;

    MP3HuffmanDecode(gr, isMPEG2, mainDataPtr, 0, origTotABsize, sfLength, hei);

    /* Begin by computing new sizes for parts a & b (& their truncations) */
#ifdef DEBUG
    fprintf(stderr, "usifh-0: %d, %d:%d, %d:%d, %d:%d, %d:%d, %d:%d\n",
	    hei.numSamples,
	    sfLength/8, sfLength%8,
	    hei.reg1Start/8, hei.reg1Start%8,
	    hei.reg2Start/8, hei.reg2Start%8,
	    hei.bigvalStart/8, hei.bigvalStart%8,
	    origTotABsize/8, origTotABsize%8);
#endif
    if (p23L0 < sfLength) {
      /* We can't use this, so give it all to the next granule: */
      p23L1 += p23L0;
      p23L0 = 0;
    }

    part23Length0a = hei.bigvalStart;
    part23Length0b = origTotABsize - hei.bigvalStart;
    part23Length0aTruncation = part23Length0bTruncation = 0;
    if (origTotABsize > p23L0) {
      /* We need to shorten one or both of fields a & b */
      unsigned truncation = origTotABsize - p23L0;
#ifdef TRUNC_FAIRLY
      part23Length0aTruncation	= (truncation*(part23Length0a-sfLength))
	                          /(origTotABsize-sfLength);
      part23Length0bTruncation = truncation - part23Length0aTruncation;
#endif
#ifdef TRUNC_FAVORa
      part23Length0bTruncation
	= (truncation > part23Length0b) ? part23Length0b : truncation;
      part23Length0aTruncation = truncation - part23Length0bTruncation;
#endif
#ifdef TRUNC_FAVORb
      part23Length0aTruncation	= (truncation > part23Length0a-sfLength)
	? (part23Length0a-sfLength) : truncation;
      part23Length0bTruncation = truncation - part23Length0aTruncation;
#endif
    }
    /* ASSERT:  part23Length0xTruncation <= part23Length0x */
    part23Length0a -= part23Length0aTruncation;
    part23Length0b -= part23Length0bTruncation;
#ifdef DEBUG
    fprintf(stderr, "usifh-0: interim sizes: %d (%d), %d (%d)\n",
	    part23Length0a, part23Length0aTruncation,
	    part23Length0b, part23Length0bTruncation);
#endif

    /* Adjust these new lengths so they end on sample bit boundaries: */
    for (i = 0; i < (int)hei.numSamples; ++i) {
      if (hei.allBitOffsets[i] == part23Length0a) break;
      else if (hei.allBitOffsets[i] > part23Length0a) {--i; break;}
    }
    if (i < 0) { /* should happen only if we couldn't fit sfLength */
      i = 0; adjustment = 0;
    } else {
      adjustment = part23Length0a - hei.allBitOffsets[i];
    }
#ifdef DEBUG
    fprintf(stderr, "%d usifh-0: adjustment 1: %d\n", debugCount, adjustment);
#endif
    part23Length0a -= adjustment;
    part23Length0aTruncation += adjustment;
    /* Assign the bits we just shaved to field b and granule 1: */
    if (part23Length0bTruncation < adjustment) {
      p23L1 += (adjustment - part23Length0bTruncation);
      adjustment = part23Length0bTruncation;
    }
    part23Length0b += adjustment;
    part23Length0bTruncation -= adjustment;
    for (j = i; j < (int)hei.numSamples; ++j) {
      if (hei.allBitOffsets[j]
	  == part23Length0a + part23Length0aTruncation + part23Length0b)
	break;
      else if (hei.allBitOffsets[j]
	  > part23Length0a + part23Length0aTruncation + part23Length0b)
	{--j; break;}
    }
    if (j < 0) { /* should happen only if we couldn't fit sfLength */
      j = 0; adjustment = 0;
    } else {
      adjustment = part23Length0a+part23Length0aTruncation+part23Length0b
                   - hei.allBitOffsets[j];
    }
#ifdef DEBUG
    fprintf(stderr, "%d usifh-0: adjustment 2: %d\n", debugCount, adjustment);
#endif
    if (adjustment > part23Length0b) adjustment = part23Length0b; /*sanity*/
    part23Length0b -= adjustment;
    part23Length0bTruncation += adjustment;
    /* Assign the bits we just shaved to granule 1 */
    p23L1 += adjustment;

    if (part23Length0aTruncation > 0) {
      /* Change the granule's 'big_values' field to reflect the truncation */
      gr->big_values = i;
    }
  }

  /* Process granule 1 (MPEG-1 only) */

  if (isMPEG2) {
    part23Length1a = part23Length1b = 0;
    part23Length1aTruncation = part23Length1bTruncation = 0;
  } else {
    unsigned granule1Offset
      = origTotABsize + sideInfo.ch[1].gr[0].part2_3_length;

    gr = &(sideInfo.ch[0].gr[1]);
    origTotABsize = gr->part2_3_length;

    MP3HuffmanDecode(gr, isMPEG2, mainDataPtr, granule1Offset,
		     origTotABsize, sfLength, hei);

    /* Begin by computing new sizes for parts a & b (& their truncations) */
#ifdef DEBUG
    fprintf(stderr, "usifh-1: %d, %d:%d, %d:%d, %d:%d, %d:%d, %d:%d\n",
	    hei.numSamples,
	    sfLength/8, sfLength%8,
	    hei.reg1Start/8, hei.reg1Start%8,
	    hei.reg2Start/8, hei.reg2Start%8,
	    hei.bigvalStart/8, hei.bigvalStart%8,
	    origTotABsize/8, origTotABsize%8);
#endif
    if (p23L1 < sfLength) {
      /* We can't use this, so give up on this granule: */
      p23L1 = 0;
    }

    part23Length1a = hei.bigvalStart;
    part23Length1b = origTotABsize - hei.bigvalStart;
    part23Length1aTruncation = part23Length1bTruncation = 0;
    if (origTotABsize > p23L1) {
      /* We need to shorten one or both of fields a & b */
      unsigned truncation = origTotABsize - p23L1;
#ifdef TRUNC_FAIRLY
      part23Length1aTruncation	= (truncation*(part23Length1a-sfLength))
	                          /(origTotABsize-sfLength);
      part23Length1bTruncation = truncation - part23Length1aTruncation;
#endif
#ifdef TRUNC_FAVORa
      part23Length1bTruncation
	= (truncation > part23Length1b) ? part23Length1b : truncation;
      part23Length1aTruncation = truncation - part23Length1bTruncation;
#endif
#ifdef TRUNC_FAVORb
      part23Length1aTruncation	= (truncation > part23Length1a-sfLength)
	? (part23Length1a-sfLength) : truncation;
      part23Length1bTruncation = truncation - part23Length1aTruncation;
#endif
    }
    /* ASSERT:  part23Length1xTruncation <= part23Length1x */
    part23Length1a -= part23Length1aTruncation;
    part23Length1b -= part23Length1bTruncation;
#ifdef DEBUG
    fprintf(stderr, "usifh-1: interim sizes: %d (%d), %d (%d)\n",
	    part23Length1a, part23Length1aTruncation,
	    part23Length1b, part23Length1bTruncation);
#endif

    /* Adjust these new lengths so they end on sample bit boundaries: */
    for (i = 0; i < (int)hei.numSamples; ++i) {
      if (hei.allBitOffsets[i] == part23Length1a) break;
      else if (hei.allBitOffsets[i] > part23Length1a) {--i; break;}
    }
    if (i < 0) { /* should happen only if we couldn't fit sfLength */
      i = 0; adjustment = 0;
    } else {
      adjustment = part23Length1a - hei.allBitOffsets[i];
    }
#ifdef DEBUG
    fprintf(stderr, "%d usifh-1: adjustment 0: %d\n", debugCount, adjustment);
#endif
    part23Length1a -= adjustment;
    part23Length1aTruncation += adjustment;
    /* Assign the bits we just shaved to field b: */
    if (part23Length1bTruncation < adjustment) {
      adjustment = part23Length1bTruncation;
    }
    part23Length1b += adjustment;
    part23Length1bTruncation -= adjustment;
    for (j = i; j < (int)hei.numSamples; ++j) {
      if (hei.allBitOffsets[j]
	  == part23Length1a + part23Length1aTruncation + part23Length1b)
	break;
      else if (hei.allBitOffsets[j]
	  > part23Length1a + part23Length1aTruncation + part23Length1b)
	{--j; break;}
    }
    if (j < 0) { /* should happen only if we couldn't fit sfLength */
      j = 0; adjustment = 0;
    } else {
      adjustment = part23Length1a+part23Length1aTruncation+part23Length1b
                   - hei.allBitOffsets[j];
    }
#ifdef DEBUG
    fprintf(stderr, "%d usifh-1: adjustment 1: %d\n", debugCount, adjustment);
#endif
    if (adjustment > part23Length1b) adjustment = part23Length1b; /*sanity*/
    part23Length1b -= adjustment;
    part23Length1bTruncation += adjustment;

    if (part23Length1aTruncation > 0) {
      /* Change the granule's 'big_values' field to reflect the truncation */
      gr->big_values = i;
    }
  }
#ifdef DEBUG
  fprintf(stderr, "usifh-end, new vals: %d (%d), %d (%d), %d (%d), %d (%d)\n",
	  part23Length0a, part23Length0aTruncation,
	  part23Length0b, part23Length0bTruncation,
	  part23Length1a, part23Length1aTruncation,
	  part23Length1b, part23Length1bTruncation);
#endif
}

static void rsf_getline(char* line, unsigned max, unsigned char**fi) {
  unsigned i;
  for (i = 0; i < max; ++i) {
    line[i] = *(*fi)++;
    if (line[i] == '\n') {
      line[i++] = '\0';
      return;
    }
  }
  line[i] = '\0';
}

static void rsfscanf(unsigned char **fi, unsigned int* v) {
  while (sscanf((char*)*fi, "%x", v) == 0) {
    /* skip past the next '\0' */
    while (*(*fi)++ != '\0') {}
  }

  /* skip past any white-space before the value: */
  while (*(*fi) <= ' ') ++(*fi);

  /* skip past the value: */
  while (*(*fi) > ' ') ++(*fi);
}

#define HUFFBITS unsigned long int
#define SIZEOF_HUFFBITS 4
#define HTN     34
#define MXOFF   250

struct huffcodetab {
  char tablename[3];	/*string, containing table_description	*/
  unsigned int xlen; 	/*max. x-index+			      	*/
  unsigned int ylen;	/*max. y-index+				*/
  unsigned int linbits; /*number of linbits			*/
  unsigned int linmax;	/*max number to be stored in linbits	*/
  int ref;		/*a positive value indicates a reference*/
  HUFFBITS *table;	/*pointer to array[xlen][ylen]		*/
  unsigned char *hlen;	/*pointer to array[xlen][ylen]		*/
  unsigned char(*val)[2];/*decoder tree				*/
  unsigned int treelen;	/*length of decoder tree		*/
};

static struct huffcodetab rsf_ht[HTN]; // array of all huffcodetable headers
				/* 0..31 Huffman code table 0..31	*/
				/* 32,33 count1-tables			*/

/* read the huffman decoder table */
static int read_decoder_table(unsigned char* fi) {
  int n,i,nn,t;
  unsigned int v0,v1;
  char command[100],line[100];
  for (n=0;n<HTN;n++) {
    rsf_ht[n].table = NULL;
    rsf_ht[n].hlen = NULL;

    /* .table number treelen xlen ylen linbits */
    do {
      rsf_getline(line,99,&fi);
    } while ((line[0] == '#') || (line[0] < ' '));

    sscanf(line,"%s %s %u %u %u %u",command,rsf_ht[n].tablename,
           &rsf_ht[n].treelen, &rsf_ht[n].xlen, &rsf_ht[n].ylen, &rsf_ht[n].linbits);
    if (strcmp(command,".end")==0)
      return n;
    else if (strcmp(command,".table")!=0) {
#ifdef DEBUG
      fprintf(stderr,"huffman table %u data corrupted\n",n);
#endif
      return -1;
    }
    rsf_ht[n].linmax = (1<<rsf_ht[n].linbits)-1;

    sscanf(rsf_ht[n].tablename,"%u",&nn);
    if (nn != n) {
#ifdef DEBUG
      fprintf(stderr,"wrong table number %u\n",n);
#endif
      return(-2);
    }
    do {
      rsf_getline(line,99,&fi);
    } while ((line[0] == '#') || (line[0] < ' '));

    sscanf(line,"%s %u",command,&t);
    if (strcmp(command,".reference")==0) {
      rsf_ht[n].ref   = t;
      rsf_ht[n].val   = rsf_ht[t].val;
      rsf_ht[n].treelen  = rsf_ht[t].treelen;
      if ( (rsf_ht[n].xlen != rsf_ht[t].xlen) ||
           (rsf_ht[n].ylen != rsf_ht[t].ylen)  ) {
#ifdef DEBUG
        fprintf(stderr,"wrong table %u reference\n",n);
#endif
        return (-3);
      };
      while ((line[0] == '#') || (line[0] < ' ') ) {
        rsf_getline(line,99,&fi);
      }
    }
    else if (strcmp(command,".treedata")==0) {
      rsf_ht[n].ref  = -1;
      rsf_ht[n].val = (unsigned char (*)[2])
        new unsigned char[2*(rsf_ht[n].treelen)];
      if ((rsf_ht[n].val == NULL) && ( rsf_ht[n].treelen != 0 )){
#ifdef DEBUG
    	fprintf(stderr, "heaperror at table %d\n",n);
#endif
    	abort();
      }
      for (i=0;(unsigned)i<rsf_ht[n].treelen; i++) {
        rsfscanf(&fi, &v0);
        rsfscanf(&fi, &v1);
/*replaces        fscanf(fi,"%x %x",&v0, &v1);*/
        rsf_ht[n].val[i][0]=(unsigned char)v0;
        rsf_ht[n].val[i][1]=(unsigned char)v1;
      }
      rsf_getline(line,99,&fi); /* read the rest of the line */
    }
    else {
#ifdef DEBUG
      fprintf(stderr,"huffman decodertable error at table %d\n",n);
#endif
    }
  }
  return n;
}

static void initialize_huffman() {
  static Boolean huffman_initialized = False;

   if (huffman_initialized) return;

   if (read_decoder_table(huffdec) != HTN) {
#ifdef DEBUG
      fprintf(stderr,"decoder table read error\n");
#endif
      abort();
      }
   huffman_initialized = True;
}

static unsigned char const slen[2][16] = {
  {0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
  {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3}
};

static unsigned char const stab[3][6][4] = {
  { { 6, 5, 5,5 } , { 6, 5, 7,3 } , { 11,10,0,0} ,
    { 7, 7, 7,0 } , { 6, 6, 6,3 } , {  8, 8,5,0} } ,
  { { 9, 9, 9,9 } , { 9, 9,12,6 } , { 18,18,0,0} ,
    {12,12,12,0 } , {12, 9, 9,6 } , { 15,12,9,0} } ,
  { { 6, 9, 9,9 } , { 6, 9,12,6 } , { 15,18,0,0} ,
    { 6,15,12,0 } , { 6,12, 9,6 } , {  6,18,9,0} }
};

static unsigned rsf_get_scale_factors_1(MP3SideInfo::gr_info_s_t *gr_info) {
   int numbits;
   int num0 = slen[0][gr_info->scalefac_compress];
   int num1 = slen[1][gr_info->scalefac_compress];

    if (gr_info->block_type == 2)
    {
      numbits = (num0 + num1) * 18;

      if (gr_info->mixed_block_flag) {
         numbits -= num0; /* num0 * 17 + num1 * 18 */
      }
    }
    else
    {
      int scfsi = gr_info->scfsi;

      if(scfsi < 0) { /* scfsi < 0 => granule == 0 */
         numbits = (num0 + num1) * 10 + num0;
      }
      else {
        numbits = 0;
        if(!(scfsi & 0x8)) {
          numbits += num0 * 6;
        }
        else {
        }

        if(!(scfsi & 0x4)) {
          numbits += num0 * 5;
        }
        else {
        }

        if(!(scfsi & 0x2)) {
          numbits += num1 * 5;
        }
        else {
        }

        if(!(scfsi & 0x1)) {
          numbits += num1 * 5;
        }
        else {
        }
      }
    }

    return numbits;
}

extern unsigned n_slen2[];
extern unsigned i_slen2[];

static unsigned rsf_get_scale_factors_2(MP3SideInfo::gr_info_s_t *gr_info) {
  unsigned char const* pnt;
  int i;
  unsigned int slen;
  int n = 0;
  int numbits = 0;

  slen = n_slen2[gr_info->scalefac_compress];

  gr_info->preflag = (slen>>15) & 0x1;

  n = 0;
  if( gr_info->block_type == 2 ) {
    n++;
    if(gr_info->mixed_block_flag)
      n++;
  }

  pnt = stab[n][(slen>>12)&0x7];

  for(i=0;i<4;i++) {
    int num = slen & 0x7;
    slen >>= 3;
    numbits += pnt[i] * num;
  }

  return numbits;
}

static unsigned getScaleFactorsLength(MP3SideInfo::gr_info_s_t* gr,
				      Boolean isMPEG2) {
  return isMPEG2 ? rsf_get_scale_factors_2(gr)
                 : rsf_get_scale_factors_1(gr);
}

static int rsf_huffman_decoder(BitVector& bv,
			       struct huffcodetab const* h,
			       int* x, int* y, int* v, int* w); // forward

void MP3HuffmanDecode(MP3SideInfo::gr_info_s_t* gr, int isMPEG2,
		      unsigned char const* fromBasePtr,
		      unsigned fromBitOffset, unsigned fromLength,
		      unsigned& scaleFactorsLength,
		      MP3HuffmanEncodingInfo& hei) {
   unsigned i;
   int x, y, v, w;
   struct huffcodetab *h;
   BitVector bv((unsigned char*)fromBasePtr, fromBitOffset, fromLength);

   /* Compute the size of the scale factors (& also advance bv): */
   scaleFactorsLength = getScaleFactorsLength(gr, isMPEG2);
   bv.skipBits(scaleFactorsLength);

   initialize_huffman();

   hei.reg1Start = hei.reg2Start = hei.numSamples = 0;

   /* Read bigvalues area. */
   if (gr->big_values < gr->region1start + gr->region2start) {
     gr->big_values = gr->region1start + gr->region2start; /* sanity check */
   }
   for (i = 0; i < gr->big_values; ++i) {
     if (i < gr->region1start) {
       /* in region 0 */
       h = &rsf_ht[gr->table_select[0]];
     } else if (i < gr->region2start) {
       /* in region 1 */
       h = &rsf_ht[gr->table_select[1]];
       if (hei.reg1Start == 0) {
	 hei.reg1Start = bv.curBitIndex();
       }
     } else {
       /* in region 2 */
       h = &rsf_ht[gr->table_select[2]];
       if (hei.reg2Start == 0) {
	 hei.reg2Start = bv.curBitIndex();
       }
     }

     hei.allBitOffsets[i] = bv.curBitIndex();
     rsf_huffman_decoder(bv, h, &x, &y, &v, &w);
     if (hei.decodedValues != NULL) {
       // Record the decoded values:
       unsigned* ptr = &hei.decodedValues[4*i];
       ptr[0] = x; ptr[1] = y; ptr[2] = v; ptr[3] = w;
     }
   }
   hei.bigvalStart = bv.curBitIndex();

   /* Read count1 area. */
   h = &rsf_ht[gr->count1table_select+32];
   while (bv.curBitIndex() < bv.totNumBits() &&  i < SSLIMIT*SBLIMIT) {
     hei.allBitOffsets[i] = bv.curBitIndex();
     rsf_huffman_decoder(bv, h, &x, &y, &v, &w);
     if (hei.decodedValues != NULL) {
       // Record the decoded values:
       unsigned* ptr = &hei.decodedValues[4*i];
       ptr[0] = x; ptr[1] = y; ptr[2] = v; ptr[3] = w;
     }
     ++i;
   }

   hei.allBitOffsets[i] = bv.curBitIndex();
   hei.numSamples = i;
}

HUFFBITS dmask = 1 << (SIZEOF_HUFFBITS*8-1);
unsigned int hs = SIZEOF_HUFFBITS*8;

/* do the huffman-decoding 						*/
static int rsf_huffman_decoder(BitVector& bv,
		struct huffcodetab const* h, // ptr to huffman code record
			/* unsigned */ int *x, // returns decoded x value
			/* unsigned */ int *y,  // returns decoded y value
			       int* v, int* w) {
  HUFFBITS level;
  unsigned point = 0;
  int error = 1;
  level     = dmask;
  *x = *y = *v = *w = 0;
  if (h->val == NULL) return 2;

  /* table 0 needs no bits */
  if (h->treelen == 0) return 0;

  /* Lookup in Huffman table. */

  do {
    if (h->val[point][0]==0) {   /*end of tree*/
      *x = h->val[point][1] >> 4;
      *y = h->val[point][1] & 0xf;

      error = 0;
      break;
    }
    if (bv.get1Bit()) {
      while (h->val[point][1] >= MXOFF) point += h->val[point][1];
      point += h->val[point][1];
    }
    else {
      while (h->val[point][0] >= MXOFF) point += h->val[point][0];
      point += h->val[point][0];
    }
    level >>= 1;
  } while (level  || (point < h->treelen) );
/////  } while (level  || (point < rsf_ht->treelen) );

  /* Check for error. */

  if (error) { /* set x and y to a medium value as a simple concealment */
    printf("Illegal Huffman code in data.\n");
    *x = ((h->xlen-1) << 1);
    *y = ((h->ylen-1) << 1);
  }

  /* Process sign encodings for quadruples tables. */

  if (h->tablename[0] == '3'
      && (h->tablename[1] == '2' || h->tablename[1] == '3')) {
     *v = (*y>>3) & 1;
     *w = (*y>>2) & 1;
     *x = (*y>>1) & 1;
     *y = *y & 1;

     if (*v)
        if (bv.get1Bit() == 1) *v = -*v;
     if (*w)
        if (bv.get1Bit() == 1) *w = -*w;
     if (*x)
        if (bv.get1Bit() == 1) *x = -*x;
     if (*y)
        if (bv.get1Bit() == 1) *y = -*y;
     }

  /* Process sign and escape encodings for dual tables. */

  else {
     if (h->linbits)
       if ((h->xlen-1) == (unsigned)*x)
         *x += bv.getBits(h->linbits);
     if (*x)
        if (bv.get1Bit() == 1) *x = -*x;
     if (h->linbits)
       if ((h->ylen-1) == (unsigned)*y)
         *y += bv.getBits(h->linbits);
     if (*y)
        if (bv.get1Bit() == 1) *y = -*y;
  }

  return error;
}

#ifdef DO_HUFFMAN_ENCODING
inline int getNextSample(unsigned char const*& fromPtr) {
  int sample
#ifdef FOUR_BYTE_SAMPLES
    = (fromPtr[0]<<24) | (fromPtr[1]<<16) | (fromPtr[2]<<8) | fromPtr[3];
#else
#ifdef TWO_BYTE_SAMPLES
    = (fromPtr[0]<<8) | fromPtr[1];
#else
    // ONE_BYTE_SAMPLES
    = fromPtr[0];
#endif
#endif
  fromPtr += BYTES_PER_SAMPLE_VALUE;
  return sample;
}

static void rsf_huffman_encoder(BitVector& bv,
				struct huffcodetab* h,
				int x, int y, int v, int w); // forward

unsigned MP3HuffmanEncode(MP3SideInfo::gr_info_s_t const* gr,
			  unsigned char const* fromPtr,
			  unsigned char* toPtr, unsigned toBitOffset,
			  unsigned numHuffBits) {
   unsigned i;
   struct huffcodetab *h;
   int x, y, v, w;
   BitVector bv(toPtr, toBitOffset, numHuffBits);

   initialize_huffman();

   // Encode big_values area:
   unsigned big_values = gr->big_values;
   if (big_values < gr->region1start + gr->region2start) {
     big_values = gr->region1start + gr->region2start; /* sanity check */
   }
   for (i = 0; i < big_values; ++i) {
     if (i < gr->region1start) {
       /* in region 0 */
       h = &rsf_ht[gr->table_select[0]];
     } else if (i < gr->region2start) {
       /* in region 1 */
       h = &rsf_ht[gr->table_select[1]];
     } else {
       /* in region 2 */
       h = &rsf_ht[gr->table_select[2]];
     }

     x = getNextSample(fromPtr);
     y = getNextSample(fromPtr);
     v = getNextSample(fromPtr);
     w = getNextSample(fromPtr);
     rsf_huffman_encoder(bv, h, x, y, v, w);
   }

   // Encode count1 area:
   h = &rsf_ht[gr->count1table_select+32];
   while (bv.curBitIndex() < bv.totNumBits() &&  i < SSLIMIT*SBLIMIT) {
     x = getNextSample(fromPtr);
     y = getNextSample(fromPtr);
     v = getNextSample(fromPtr);
     w = getNextSample(fromPtr);
     rsf_huffman_encoder(bv, h, x, y, v, w);
     ++i;
   }

   return i;
}

static Boolean lookupHuffmanTableEntry(struct huffcodetab const* h,
				       HUFFBITS bits, unsigned bitsLength,
				       unsigned char& xy) {
  unsigned point = 0;
  unsigned mask = 1;
  unsigned numBitsTestedSoFar = 0;
  do {
    if (h->val[point][0]==0) { // end of tree
      xy = h->val[point][1];
      if (h->hlen[xy] == 0) { // this entry hasn't already been used
	h->table[xy] = bits;
	h->hlen[xy] = bitsLength;
	return True;
      } else { // this entry has already been seen
	return False;
      }
    }

    if (numBitsTestedSoFar++ == bitsLength) {
      // We don't yet have enough bits for this prefix
      return False;
    }
    if (bits&mask) {
      while (h->val[point][1] >= MXOFF) point += h->val[point][1];
      point += h->val[point][1];
    } else {
      while (h->val[point][0] >= MXOFF) point += h->val[point][0];
      point += h->val[point][0];
    }
    mask <<= 1;
  } while (mask || (point < h->treelen));

  return False;
}

static void buildHuffmanEncodingTable(struct huffcodetab* h) {
  h->table = new unsigned long[256];
  h->hlen = new unsigned char[256];
  if (h->table == NULL || h->hlen == NULL) { h->table = NULL; return; }
  for (unsigned i = 0; i < 256; ++i) {
    h->table[i] = 0; h->hlen[i] = 0;
  }

  // Look up entries for each possible bit sequence length:
  unsigned maxNumEntries = h->xlen * h->ylen;
  unsigned numEntries = 0;
  unsigned powerOf2 = 1;
  for (unsigned bitsLength = 1;
       bitsLength <= 8*SIZEOF_HUFFBITS; ++bitsLength) {
    powerOf2 *= 2;
    for (HUFFBITS bits = 0; bits < powerOf2; ++bits) {
      // Find the table value - if any - for 'bits' (length 'bitsLength'):
      unsigned char xy;
      if (lookupHuffmanTableEntry(h, bits, bitsLength, xy)) {
	++numEntries;
	if (numEntries == maxNumEntries) return; // we're done
      }
    }
  }
#ifdef DEBUG
  fprintf(stderr, "Didn't find enough entries!\n"); // shouldn't happen
#endif
}

static void lookupXYandPutBits(BitVector& bv, struct huffcodetab const* h,
			       unsigned char xy) {
  HUFFBITS bits = h->table[xy];
  unsigned bitsLength = h->hlen[xy];

  // Note that "bits" is in reverse order, so read them from right-to-left:
  while (bitsLength-- > 0) {
    bv.put1Bit(bits&0x00000001);
    bits >>= 1;
  }
}

static void putLinbits(BitVector& bv, struct huffcodetab const* h,
		       HUFFBITS bits) {
  bv.putBits(bits, h->linbits);
}

static void rsf_huffman_encoder(BitVector& bv,
				struct huffcodetab* h,
				int x, int y, int v, int w) {
  if (h->val == NULL) return;

  /* table 0 produces no bits */
  if (h->treelen == 0) return;

  if (h->table == NULL) {
    // We haven't yet built the encoding array for this table; do it now:
    buildHuffmanEncodingTable(h);
    if (h->table == NULL) return;
  }

  Boolean xIsNeg = False, yIsNeg = False, vIsNeg = False, wIsNeg = False;
  unsigned char xy;

#ifdef FOUR_BYTE_SAMPLES
#else
#ifdef TWO_BYTE_SAMPLES
  // Convert 2-byte negative numbers to their 4-byte equivalents:
  if (x&0x8000) x |= 0xFFFF0000;
  if (y&0x8000) y |= 0xFFFF0000;
  if (v&0x8000) v |= 0xFFFF0000;
  if (w&0x8000) w |= 0xFFFF0000;
#else
  // ONE_BYTE_SAMPLES
  // Convert 1-byte negative numbers to their 4-byte equivalents:
  if (x&0x80) x |= 0xFFFFFF00;
  if (y&0x80) y |= 0xFFFFFF00;
  if (v&0x80) v |= 0xFFFFFF00;
  if (w&0x80) w |= 0xFFFFFF00;
#endif
#endif

  if (h->tablename[0] == '3'
      && (h->tablename[1] == '2' || h->tablename[1] == '3')) {// quad tables
    if (x < 0) { xIsNeg = True; x = -x; }
    if (y < 0) { yIsNeg = True; y = -y; }
    if (v < 0) { vIsNeg = True; v = -v; }
    if (w < 0) { wIsNeg = True; w = -w; }

    // Sanity check: x,y,v,w must all be 0 or 1:
    if (x>1 || y>1 || v>1 || w>1) {
#ifdef DEBUG
      fprintf(stderr, "rsf_huffman_encoder quad sanity check fails: %x,%x,%x,%x\n", x, y, v, w);
#endif
    }

    xy = (v<<3)|(w<<2)|(x<<1)|y;
    lookupXYandPutBits(bv, h, xy);

    if (v) bv.put1Bit(vIsNeg);
    if (w) bv.put1Bit(wIsNeg);
    if (x) bv.put1Bit(xIsNeg);
    if (y) bv.put1Bit(yIsNeg);
  } else { // dual tables
    // Sanity check: v and w must be 0:
    if (v != 0 || w != 0) {
#ifdef DEBUG
      fprintf(stderr, "rsf_huffman_encoder dual sanity check 1 fails: %x,%x,%x,%x\n", x, y, v, w);
#endif
    }

    if (x < 0) { xIsNeg = True; x = -x; }
    if (y < 0) { yIsNeg = True; y = -y; }

    // Sanity check: x and y must be <= 255:
    if (x > 255 || y > 255) {
#ifdef DEBUG
      fprintf(stderr, "rsf_huffman_encoder dual sanity check 2 fails: %x,%x,%x,%x\n", x, y, v, w);
#endif
    }

    int xl1 = h->xlen-1;
    int yl1 = h->ylen-1;
    unsigned linbitsX = 0; unsigned linbitsY = 0;

    if (((x < xl1) || (xl1 == 0)) && (y < yl1)) {
      // normal case
      xy = (x<<4)|y;
      lookupXYandPutBits(bv, h, xy);
      if (x) bv.put1Bit(xIsNeg);
      if (y) bv.put1Bit(yIsNeg);
    } else if (x >= xl1) {
      linbitsX = (unsigned)(x - xl1);
      if (linbitsX > h->linmax) {
#ifdef DEBUG
	fprintf(stderr,"warning: Huffman X table overflow\n");
#endif
	linbitsX = h->linmax;
      };

      if (y >= yl1) {
	xy = (xl1<<4)|yl1;
	lookupXYandPutBits(bv, h, xy);
	linbitsY = (unsigned)(y - yl1);
	if (linbitsY > h->linmax) {
#ifdef DEBUG
	  fprintf(stderr,"warning: Huffman Y table overflow\n");
#endif
	  linbitsY = h->linmax;
	};

	if (h->linbits) putLinbits(bv, h, linbitsX);
	if (x) bv.put1Bit(xIsNeg);
	if (h->linbits) putLinbits(bv, h, linbitsY);
	if (y) bv.put1Bit(yIsNeg);
      } else { /* x >= h->xlen, y < h->ylen */
	xy = (xl1<<4)|y;
	lookupXYandPutBits(bv, h, xy);
	if (h->linbits) putLinbits(bv, h, linbitsX);
	if (x) bv.put1Bit(xIsNeg);
	if (y) bv.put1Bit(yIsNeg);
      }
    } else { /* ((x < h->xlen) && (y >= h->ylen)) */
      xy = (x<<4)|yl1;
      lookupXYandPutBits(bv, h, xy);
      linbitsY = y-yl1;
      if (linbitsY > h->linmax) {
#ifdef DEBUG
	fprintf(stderr,"warning: Huffman Y table overflow\n");
#endif
	linbitsY = h->linmax;
      };
      if (x) bv.put1Bit(xIsNeg);
      if (h->linbits) putLinbits(bv, h, linbitsY);
      if (y) bv.put1Bit(yIsNeg);
    }
  }
}
#endif
