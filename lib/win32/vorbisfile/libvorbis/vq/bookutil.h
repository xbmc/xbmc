/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: utility functions for loading .vqh and .vqd files
 last mod: $Id: bookutil.h 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#ifndef _V_BOOKUTIL_H_
#define _V_BOOKUTIL_H_

#include <stdio.h>
#include <sys/time.h>

#include "localcodebook.h"

extern char     *get_line(FILE *in);
extern char     *setup_line(FILE *in);
extern int       get_line_value(FILE *in,float *value);
extern int       get_next_value(FILE *in,float *value);
extern int       get_next_ivalue(FILE *in,long *ivalue);
extern void      reset_next_value(void);
extern int       get_vector(codebook *b,FILE *in,int start,int num,float *a);
extern char     *find_seek_to(FILE *in,char *s);

extern codebook *codebook_load(char *filename);
extern void write_codebook(FILE *out,char *name,const static_codebook *c);

extern void spinnit(char *s,int n);
extern void build_tree_from_lengths(int vals, long *hist, long *lengths);
extern void build_tree_from_lengths0(int vals, long *hist, long *lengths);

#endif

