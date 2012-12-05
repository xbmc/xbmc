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

//#ifndef __powerpc__
#if !defined(__powerpc__) && !defined(__ppc__) && !defined(__arm__)

#include "ldt_keeper.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include "mmap_anon.h"
#if defined( __linux__ ) && !defined(__powerpc__)
#include <asm/unistd.h>
#include <asm/ldt.h>
/* 2.5.xx+ calls this user_desc: */
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,47)
#define modify_ldt_ldt_s user_desc
#endif
/* prototype it here, so we won't depend on kernel headers */
#ifdef  __cplusplus
extern "C" {
#endif
/* declare modify_ldt with the _syscall3 macro for older glibcs */
#if defined(__GLIBC__) &&  (__GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ == 0))
_syscall3( int, modify_ldt, int, func, void *, ptr, unsigned long, bytecount );
#else
#if defined(__ANDROID__) && defined(__i386__) && !defined(modify_ldt)
#define modify_ldt(a,b,c) syscall( __NR_modify_ldt,  a, b, c);
#else
int modify_ldt(int func, void *ptr, unsigned long bytecount);
#endif
#endif
#ifdef  __cplusplus
}
#endif
#else
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#include <machine/segments.h>
#include <machine/sysarch.h>
#endif
#if defined(TARGET_DARWIN)
#include <i386/user_ldt.h>
#endif

#ifdef __svr4__
#include <sys/segment.h>
#include <sys/sysi86.h>

/* solaris x86: add missing prototype for sysi86(), but only when sysi86(int, void*) is known to be valid */
#ifdef HAVE_SYSI86_iv
#ifdef  __cplusplus
extern "C" {
#endif
int sysi86(int, void*);
#ifdef  __cplusplus
}
#endif
#endif

#ifndef NUMSYSLDTS             /* SunOS 2.5.1 does not define NUMSYSLDTS */
#define NUMSYSLDTS     6       /* Let's hope the SunOS 5.8 value is OK */
#endif

#define TEB_SEL_IDX NUMSYSLDTS
#endif

#define LDT_ENTRIES     8192
#define LDT_ENTRY_SIZE  8
#pragma pack(4)
struct modify_ldt_ldt_s {
  unsigned int  entry_number;
  unsigned long base_addr;
  unsigned int  limit;
  unsigned int  seg_32bit:1;
  unsigned int  contents:2;
  unsigned int  read_exec_only:1;
  unsigned int  limit_in_pages:1;
  unsigned int  seg_not_present:1;
  unsigned int  useable:1;
};

#define MODIFY_LDT_CONTENTS_DATA        0
#define MODIFY_LDT_CONTENTS_STACK       1
#define MODIFY_LDT_CONTENTS_CODE        2
#endif


/* user level (privilege level: 3) ldt (1<<2) segment selector */
#define       LDT_SEL(idx) ((idx) << 3 | 1 << 2 | 3)

/* i got this value from wine sources, it's the first free LDT entry */
#if (defined(TARGET_DARWIN) || defined(__FreeBSD__)) && defined(LDT_AUTO_ALLOC)
#define TEB_SEL_IDX LDT_AUTO_ALLOC
#define USE_LDT_AA
#endif

#ifndef TEB_SEL_IDX
#define TEB_SEL_IDX     17
#endif

static unsigned int fs_ldt = TEB_SEL_IDX;


/**
 * here is a small logical problem with Restore for multithreaded programs -
 * in C++ we use static class for this...
 */

#ifdef __cplusplus
extern "C"
#endif
void Setup_FS_Segment(void)
{
  unsigned int ldt_desc = LDT_SEL(fs_ldt);

  __asm__ __volatile__(
  	"movl %0,%%eax; movw %%ax, %%fs" : : "r" (ldt_desc)
	  :"eax"
  );
}

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(TARGET_DARWIN)
static void LDT_EntryToBytes( unsigned long *buffer, const struct modify_ldt_ldt_s *content )
{
  *buffer++ = ((content->base_addr & 0x0000ffff) << 16) |
	  (content->limit & 0x0ffff);
  *buffer = (content->base_addr & 0xff000000) |
	  ((content->base_addr & 0x00ff0000)>>16) |
	   (content->limit & 0xf0000) |
	   (content->contents << 10) |
	  ((content->read_exec_only == 0) << 9) |
	  ((content->seg_32bit != 0) << 22) |
	  ((content->limit_in_pages != 0) << 23) |
	  0xf000;
}
#endif

void* fs_seg=0;

ldt_fs_t* Setup_LDT_Keeper(void)
{
  struct modify_ldt_ldt_s array;
  int ret;
  int sret;
  ldt_fs_t* ldt_fs = (ldt_fs_t*) malloc(sizeof(ldt_fs_t));

  if (!ldt_fs)
	  return NULL;

#if defined(TARGET_DARWIN)
  if (getenv("DYLD_BIND_AT_LAUNCH") == NULL)
    printf("DYLD_BIND_AT_LAUNCH");
#endif // TARGET_DARWIN
    
  sret = sysconf(_SC_PAGE_SIZE);
  if (sret == -1)
  {
	  perror("ERROR: Couldn't allocate memory for fs segment");
    free(ldt_fs);
    return NULL;
  }

  fs_seg = ldt_fs->fs_seg = mmap_anon(NULL, sret, PROT_READ | PROT_WRITE, MAP_PRIVATE, 0);
  if (ldt_fs->fs_seg == (void*)-1)
  {
	  perror("ERROR: Couldn't allocate memory for fs segment");
    free(ldt_fs);
	  return NULL;
  }
  *(void**)((char*)ldt_fs->fs_seg + 0x18) = ldt_fs->fs_seg;
  memset(&array, 0, sizeof(array));
  array.base_addr = (long)ldt_fs->fs_seg;
  array.entry_number = TEB_SEL_IDX;
  array.limit = array.base_addr+sysconf(_SC_PAGE_SIZE) - 1;
  array.seg_32bit = 1;
  array.read_exec_only = 0;
  array.seg_not_present = 0;
  array.contents=MODIFY_LDT_CONTENTS_DATA;
  array.limit_in_pages = 0;
#ifdef __linux__
  /* ret=LDT_Modify(0x1, &array, sizeof(struct modify_ldt_ldt_s)); */
  ret = modify_ldt(0x1, &array, sizeof(struct modify_ldt_ldt_s));
  if (ret < 0)
  {
	  perror("install_fs");
	  printf("Couldn't install fs segment, expect segfault\n");
  }
#endif /*linux*/

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(TARGET_DARWIN)
  {
    unsigned long d[2];

    LDT_EntryToBytes( d, &array );
#ifdef USE_LDT_AA
    ret = i386_set_ldt(LDT_AUTO_ALLOC, (const union ldt_entry *)d, 1);
    array.entry_number = ret;
    fs_ldt = ret;
#else
    ret = i386_set_ldt(array.entry_number, (const union ldt_entry *)d, 1);
#endif
    if (ret < 0)
    {
      perror("install_fs");
	    printf("Couldn't install fs segment, expect segfault\n");
      printf("Did you reconfigure the kernel with \"options USER_LDT\"?\n");
#ifdef __OpenBSD__
	    printf("On newer OpenBSD systems did you set machdep.userldt to 1?\n");
#endif
    }
  }
#endif  // __NetBSD__ || __FreeBSD__ || __OpenBSD__ || __DragonFly__ || TARGET_DARWIN

#if defined(__svr4__)
  {
	  struct ssd ssd;
	  ssd.sel = LDT_SEL(TEB_SEL_IDX);
	  ssd.bo = array.base_addr;
	  ssd.ls = array.limit - array.base_addr;
	  ssd.acc1 = ((array.read_exec_only == 0) << 1) |
	    (array.contents << 2) |
	    0xf0;   /* P(resent) | DPL3 | S */
	  ssd.acc2 = 0x4;   /* byte limit, 32-bit segment */
	  if (sysi86(SI86DSCR, &ssd) < 0)
    {
	    perror("sysi86(SI86DSCR)");
	    printf("Couldn't install fs segment, expect segfault\n");
	  }
  }
#endif

  Setup_FS_Segment();

  ldt_fs->prev_struct = malloc(8);
  *(void**)array.base_addr = ldt_fs->prev_struct;

  return ldt_fs;
}

void Restore_LDT_Keeper(ldt_fs_t* ldt_fs)
{
  if (ldt_fs == NULL || ldt_fs->fs_seg == 0)
	  return;
  if (ldt_fs->prev_struct)
	  free(ldt_fs->prev_struct);
  
  int sret = sysconf(_SC_PAGE_SIZE);
  if (sret != -1)
    munmap((char*)ldt_fs->fs_seg, sret);
  ldt_fs->fs_seg = 0;
  free(ldt_fs);
}

#endif
