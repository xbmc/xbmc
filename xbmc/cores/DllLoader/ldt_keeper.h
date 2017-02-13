/**
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * This file MUST be in main library because LDT must
 * be modified before program creates first thread
 * - avifile includes this file from C++ code
 * and initializes it at the start of player!
 * it might sound like a hack and it really is - but
 * as aviplay is deconding video with more than just one
 * thread currently it's necessary to do it this way
 * this might change in the future
 */

/* applied some modification to make make our xine friend more happy */

/*
 * Modified for use with MPlayer, detailed changelog at
 * http://svn.mplayerhq.hu/mplayer/trunk/
 * $Id: ldt_keeper.c 22733 2007-03-18 22:18:11Z nicodvb $
 */

#ifndef LDT_KEEPER_H
#define LDT_KEEPER_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
  void* fs_seg;
  char* prev_struct;
  int fd;
} ldt_fs_t;

#if !defined(__mips__)
void      Setup_FS_Segment(void);
ldt_fs_t* Setup_LDT_Keeper(void);
void      Restore_LDT_Keeper(ldt_fs_t* ldt_fs);
#endif /*!defined(__mips__)*/

#ifdef __cplusplus
}
#endif

#endif /* LDT_KEEPER_H */
