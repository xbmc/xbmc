/*
 * balloc.c -- Block allocation module
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: balloc.c,v 1.4 2002/12/02 15:34:19 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	This module implements a very fast block allocation scheme suitable for
 *	ROMed environments. It maintains block class queues for rapid allocation
 *	and minimal fragmentation. This module does not coalesce blocks. The 
 *	storage space may be populated statically or via the traditional malloc 
 *	mechanisms. Large blocks greater than the maximum class size may be 
 *	allocated from the O/S or run-time system via malloc. To permit the use 
 *	of malloc, call bopen with flags set to B_USE_MALLOC (this is the default).
 *	It is recommended that bopen be called first thing in the application. 
 *	If it is not, it will be called with default values on the first call to 
 *	balloc(). Note that this code is not designed for multi-threading purposes
 *	and it depends on newly declared variables being initialized to zero.
 */  

/********************************* Includes ***********************************/

#define IN_BALLOC

#ifdef UEMF
	#include	"uemf.h"
#else
	#include	"basic/basicInternal.h"
#endif

#include	<stdarg.h>
#include	<stdlib.h>

#ifndef NO_BALLOC
/********************************* Defines ************************************/

/*
 *	Define B_STATS if you wish to track memory block and stack usage
 */
#ifdef B_STATS
/*
 *	Optional statistics
 */

typedef struct {
	long	alloc;								/* Block allocation calls */
	long	inuse;								/* Blocks in use */
} bStatsType;

typedef struct {
	char_t 	file[FNAMESIZE];
	long	allocated;							/* Bytes currently allocated */
	long	count;								/* Current block count */
	long	times;								/* Count of alloc attempts */
	long	largest;							/* largest allocated here */
	int		q;
} bStatsFileType;

/*
 *	This one is very expensive but great stats
 */
typedef struct {
	void			*ptr;						/* Pointer to memory */
	bStatsFileType	*who;						/* Who allocated the memory */
} bStatsBlkType;

static bStatsType		bStats[B_MAX_CLASS];	/* Per class stats */
static bStatsFileType 	bStatsFiles[B_MAX_FILES];/* Per file stats */
static bStatsBlkType 	bStatsBlks[B_MAX_BLOCKS];/* Per block stats */
static int			 	bStatsBlksMax = 0;		/* Max block entry */
static int			 	bStatsFilesMax = 0;		/* Max file entry */
static int 				bStatsMemInUse = 0;		/* Memory currently in use */
static int 				bStatsBallocInUse = 0;	/* Memory currently balloced */
static int 				bStatsMemMax = 0;		/* Max memory ever used */
static int 				bStatsBallocMax = 0;	/* Max memory ever balloced */
static void				*bStackMin = (void*) -1;/* Miniumum stack position */
static void				*bStackStart;			/* Starting stack position */
static int 				bStatsMemMalloc = 0;	/* Malloced memory */
#endif /* B_STATS */

/*
 *	ROUNDUP4(size) returns the next higher integer value of size that is 
 *	divisible by 4, or the value of size if size is divisible by 4.
 *	ROUNDUP4() is used in aligning memory allocations on 4-byte boundaries.
 *
 *	Note:  ROUNDUP4() is only required on some operating systems (IRIX).
 */

#define ROUNDUP4(size) ((size) % 4) ? (size) + (4 - ((size) % 4)) : (size)

/********************************** Locals ************************************/
/*
 *	bQhead blocks are created as the original memory allocation is freed up.
 *	See bfree.
 */
static bType			*bQhead[B_MAX_CLASS];	/* Per class block q head */
static char				*bFreeBuf;				/* Pointer to free memory */
static char				*bFreeNext;				/* Pointer to next free mem */
static int				bFreeSize;				/* Size of free memory */
static int				bFreeLeft;				/* Size of free left for use */
static int				bFlags = B_USE_MALLOC;	/* Default to auto-malloc */
static int				bopenCount = 0;			/* Num tasks using balloc */

/*************************** Forward Declarations *****************************/

#ifdef B_STATS
static void bStatsAlloc(B_ARGS_DEC, void *ptr, int q, int size);
static void bStatsFree(B_ARGS_DEC, void *ptr, int q, int size);
static void bstatsWrite(int handle, char_t *fmt, ...);
static int 	bStatsFileSort(const void *cp1, const void *cp2);
#endif /* B_STATS */

#if (defined (B_FILL) || defined (B_VERIFY_CAUSES_SEVERE_OVERHEAD))
static void bFillBlock(void *buf, int bufsize);
#endif

#ifdef B_VERIFY_CAUSES_SEVERE_OVERHEAD
static void verifyUsedBlock(bType *bp, int q);
static void verifyFreeBlock(bType *bp, int q);
void verifyBallocSpace();
#endif

static int ballocGetSize(int size, int *q);

/********************************** Code **************************************/
/*
 *	Initialize the balloc module. bopen should be called the very first thing
 *	after the application starts and bclose should be called the last thing 
 *	before exiting. If bopen is not called, it will be called on the first 
 *	allocation with default values. "buf" points to memory to use of size 
 *	"bufsize". If buf is NULL, memory is allocated using malloc. flags may 
 *	be set to B_USE_MALLOC if using malloc is okay. This routine will allocate
 *	an initial buffer of size bufsize for use by the application.
 */

int bopen(void *buf, int bufsize, int flags)
{
	bFlags = flags;

#ifdef BASTARD_TESTING
	srand(time(0L));
#endif /* BASTARD_TESTING */

/*
 *	If bopen already called by a shared process, just increment the count
 *	and return;
 */
	if (++bopenCount > 1) {
		return 0;
	}

	if (buf == NULL) {
		if (bufsize == 0) {
			bufsize = B_DEFAULT_MEM;
		}
#ifdef IRIX
		bufsize = ROUNDUP4(bufsize);
#endif
		if ((buf = malloc(bufsize)) == NULL) {
         /* resetting bopenCount lets client code decide to attempt to call
          * bopen() again with a smaller memory request, should it desire to.
          * Fix suggested by Simon Byholm.
          */
         --bopenCount;
			return -1;
		}
#ifdef B_STATS
		bStatsMemMalloc += bufsize;
#endif
	} else {
		bFlags |= B_USER_BUF;
	}

	bFreeSize = bFreeLeft = bufsize;
	bFreeBuf = bFreeNext = buf;
	memset(bQhead, 0, sizeof(bQhead));
#if (defined (B_FILL) || defined (B_VERIFY_CAUSES_SEVERE_OVERHEAD))
	bFillBlock(buf, bufsize);
#endif
#ifdef B_STATS
	bStackStart = &buf;
#endif
#ifdef B_VERIFY_CAUSES_SEVERE_OVERHEAD
	verifyFreeBlock(buf, bufsize);
#endif
	return 0;
}

/******************************************************************************/
/*
 *	Close down the balloc module and free all malloced memory.
 */

void bclose()
{
#ifdef B_VERIFY_CAUSES_SEVERE_OVERHEAD
	verifyBallocSpace();
#endif
	if (--bopenCount <= 0 && !(bFlags & B_USER_BUF)) {
		free(bFreeBuf);
		bopenCount = 0;
	}
}

/******************************************************************************/
/*
 *	Allocate a block of the requested size. First check the block 
 *	queues for a suitable one.
 */

void *balloc(B_ARGS_DEC, int size)
{
	bType	*bp;
	int		q, memSize;

/*
 *	Call bopen with default values if the application has not yet done so
 */
	if (bFreeBuf == NULL) {
		if (bopen(NULL, B_DEFAULT_MEM, 0) < 0) {
			return NULL;
		}
	}
#ifdef B_VERIFY_CAUSES_SEVERE_OVERHEAD
	verifyBallocSpace();
#endif
	if (size < 0) {
		return NULL;
	}

#ifdef BASTARD_TESTING
	if (rand() == 0x7fff) {
		return NULL;
	}
#endif /* BASTARD_TESTING */


	memSize = ballocGetSize(size, &q);

	if (q >= B_MAX_CLASS) {
/*
 *		Size if bigger than the maximum class. Malloc if use has been okayed
 */
		if (bFlags & B_USE_MALLOC) {
#ifdef B_STATS
			bstats(0, NULL);
#endif
#ifdef IRIX
			memSize = ROUNDUP4(memSize);
#endif
			bp = (bType*) malloc(memSize);
			if (bp == NULL) {
				traceRaw(T("B: malloc failed\n"));
				return NULL;
			}
#ifdef B_STATS
			bStatsMemMalloc += memSize;
#endif
#if (defined (B_FILL) || defined (B_VERIFY_CAUSES_SEVERE_OVERHEAD))
			bFillBlock(bp, memSize);
#endif

		} else {
			traceRaw(T("B: malloc failed\n"));
			return NULL;
		}

/*
 *		the u.size is the actual size allocated for data
 */
		bp->u.size = memSize - sizeof(bType);
		bp->flags = B_MALLOCED;

	} else if ((bp = bQhead[q]) != NULL) {
/*
 *		Take first block off the relevant q if non-empty
 */
		bQhead[q] = bp->u.next;
#ifdef B_VERIFY_CAUSES_SEVERE_OVERHEAD
		verifyFreeBlock(bp, q);
#endif
#if (defined (B_FILL) || defined (B_VERIFY_CAUSES_SEVERE_OVERHEAD))
		bFillBlock(bp, memSize);
#endif
		bp->u.size = memSize - sizeof(bType);
		bp->flags = 0;

	} else {
		if (bFreeLeft > memSize) {
/*
 *			The q was empty, and the free list has spare memory so 
 *			create a new block out of the primary free block
 */
			bp = (bType*) bFreeNext;
#ifdef B_VERIFY_CAUSES_SEVERE_OVERHEAD
			verifyFreeBlock(bp, q);
#endif
			bFreeNext += memSize;
			bFreeLeft -= memSize;
#if (defined (B_FILL) || defined (B_VERIFY_CAUSES_SEVERE_OVERHEAD))
			bFillBlock(bp, memSize);
#endif
			bp->u.size = memSize - sizeof(bType);
			bp->flags = 0;

		} else if (bFlags & B_USE_MALLOC) {
#ifdef B_STATS
			static int once = 0;
			if (once++ == 0) {
				bstats(0, NULL);
			}
#endif
/*
 *			Nothing left on the primary free list, so malloc a new block
 */
#ifdef IRIX
			memSize = ROUNDUP4(memSize);
#endif
			if ((bp = (bType*) malloc(memSize)) == NULL) {
				traceRaw(T("B: malloc failed\n"));
				return NULL;
			}
#ifdef B_STATS
			bStatsMemMalloc += memSize;
#endif
#if (defined (B_FILL) || defined (B_VERIFY_CAUSES_SEVERE_OVERHEAD))
			bFillBlock(bp, memSize);
#endif
			bp->u.size = memSize - sizeof(bType);
			bp->flags = B_MALLOCED;

		} else {
			traceRaw(T("B: malloc failed\n"));
			return NULL;
		}
	}

#ifdef B_STATS
	bStatsAlloc(B_ARGS, bp, q, memSize);
#endif
	bp->flags |= B_INTEGRITY;

/*
 *	The following is a good place to put a breakpoint when trying to reduce
 *	determine and reduce maximum memory use.
 */
#if 0
#ifdef B_STATS
	if (bStatsBallocInUse == bStatsBallocMax) {
		bstats(0, NULL);
	}
#endif
#endif
	return (void*) ((char*) bp + sizeof(bType));
}

/******************************************************************************/
/*
 *	Free a block back to the relevant free q. We don't free back to the O/S
 *	or run time system unless the block is greater than the maximum class size.
 *	We also do not coalesce blocks.
 */

void bfree(B_ARGS_DEC, void *mp)
{
	bType	*bp;
	int		q, memSize;

#ifdef B_VERIFY_CAUSES_SEVERE_OVERHEAD
	verifyBallocSpace();
#endif
	bp = (bType*) ((char*) mp - sizeof(bType));

	a_assert((bp->flags & B_INTEGRITY_MASK) == B_INTEGRITY);

	if ((bp->flags & B_INTEGRITY_MASK) != B_INTEGRITY) {
		return;
	}

	memSize = ballocGetSize(bp->u.size, &q);

#ifdef B_VERIFY_CAUSES_SEVERE_OVERHEAD
	verifyUsedBlock(bp,q);
#endif
#ifdef B_STATS
	bStatsFree(B_ARGS, bp, q, bp->u.size+sizeof(bType));
#endif
	if (bp->flags & B_MALLOCED) {
		free(bp);
		return;
	}
		
#ifdef B_VERIFY_CAUSES_SEVERE_OVERHEAD
	bFillBlock(bp, memSize);
#endif

/*
 *	Simply link onto the head of the relevant q
 */
	bp->u.next = bQhead[q];
	bQhead[q] = bp;

	bp->flags = B_FILL_WORD;
}

/******************************************************************************/
/*
 *	Safe free
 */

void bfreeSafe(B_ARGS_DEC, void *mp)
{
	if (mp) {
		bfree(B_ARGS, mp);
	}
}

/******************************************************************************/
#ifdef UNICODE
/*
 *	Duplicate a string, allow NULL pointers and then dup an empty string.
 */

char *bstrdupA(B_ARGS_DEC, char *s)
{
	char	*cp;
	int		len;

	if (s == NULL) {
		s = "";
	}
	len = strlen(s) + 1;
	if (cp = balloc(B_ARGS, len)) {
		strcpy(cp, s);
	}
	return cp;
}

#endif /* UNICODE */
/******************************************************************************/
/*
 *	Duplicate an ascii string, allow NULL pointers and then dup an empty string.
 *	If UNICODE, bstrdup above works with wide chars, so we need this routine
 *	for ascii strings. 
 */

char_t *bstrdup(B_ARGS_DEC, char_t *s)
{
	char_t	*cp;
	int		len;

	if (s == NULL) {
		s = T("");
	}
	len = gstrlen(s) + 1;
	if ((cp = balloc(B_ARGS, len * sizeof(char_t))) != NULL) {
		gstrcpy(cp, s);
	}
	return cp;
}

/******************************************************************************/
/*
 *	Reallocate a block. Allow NULL pointers and just do a malloc.
 *	Note: if the realloc fails, we return NULL and the previous buffer is 
 *	preserved.
 */

void *brealloc(B_ARGS_DEC, void *mp, int newsize)
{
	bType	*bp;
	void	*newbuf;

	if (mp == NULL) {
		return balloc(B_ARGS, newsize);
	}
	bp = (bType*) ((char*) mp - sizeof(bType));
	a_assert((bp->flags & B_INTEGRITY_MASK) == B_INTEGRITY);

/*
 *	If the allocated memory already has enough room just return the previously
 *	allocated address.
 */
	if (bp->u.size >= newsize) {
		return mp;
	}
	if ((newbuf = balloc(B_ARGS, newsize)) != NULL) {
		memcpy(newbuf, mp, bp->u.size);
		bfree(B_ARGS, mp);
	}
	return newbuf;
}

/******************************************************************************/
/*
 *	Find the size of the block to be balloc'ed.  It takes in a size, finds the 
 *	smallest binary block it fits into, adds an overhead amount and returns.
 *	q is the binary size used to keep track of block sizes in use.  Called
 *	from both balloc and bfree.
 */

static int ballocGetSize(int size, int *q)
{
	int	mask;

	mask = (size == 0) ? 0 : (size-1) >> B_SHIFT;
	for (*q = 0; mask; mask >>= 1) {
		*q = *q + 1;
	}
	return ((1 << (B_SHIFT + *q)) + sizeof(bType));
}

/******************************************************************************/
#if (defined (B_FILL) || defined (B_VERIFY_CAUSES_SEVERE_OVERHEAD))
/*
 *	Fill the block (useful during development to catch zero fill assumptions)
 */

static void bFillBlock(void *buf, int bufsize)
{
	memset(buf, B_FILL_CHAR, bufsize);
}
#endif

/******************************************************************************/
#ifdef B_STATS
/*
 *	Statistics. Do output via calling the writefn callback function with 
 *	"handle" as the output file handle. 
 */

void bstats(int handle, void (*writefn)(int handle, char_t *fmt, ...))
{
	bStatsFileType	*fp, *files;
	bStatsBlkType	*blkp;
	bType			*bp;
	char_t			*cp;
	int				q, count, mem, total, len;
	static	int 	recurseProtect = 0;

	if (recurseProtect++ > 0) {
		recurseProtect--;
		return;
	}

	if (writefn == NULL) {
		writefn = bstatsWrite;
	}

/*
 *	Print stats for each memory block
 */
	(*writefn)(handle, T("\nMemory Stats\n"));

/*
 *	The following tabular format is now used for the output.
 *   Q  Size  Free Bytes Inuse Bytes Allocs
 *	dd ddddd   ddd ddddd  dddd ddddd   dddd
 */
	(*writefn)(handle, " Q  Size   Free  Bytes Inuse Bytes Allocs\n");

	total = 0;
	for (q = 0; q < B_MAX_CLASS; q++) {
		count = 0;
		for (bp = bQhead[q]; bp; bp = bp->u.next) {
			count++;
		}
		mem = count * (1 << (q + B_SHIFT));
		total += mem;
		(*writefn)(handle, 
			T("%2d %5d   %4d %6d  %4d %5d   %4d\n"),
			q, 1 << (q + B_SHIFT), count, mem, bStats[q].inuse, 
			bStats[q].inuse * (1 << (q + B_SHIFT)), bStats[q].alloc);
	}

	(*writefn)(handle, T("\n"));

/*
 *	Print summary stats
 *
 *	bFreeSize			Initial memory reserved with bopen call
 *	bStatsMemMalloc		memory from calls to system MALLOC
 *	bStatsMemMax		
 *	bStatsBallocMax		largest amount of memory from balloc calls
 *	bStatsMemInUse
 *	bStatsBallocInUse	present balloced memory being used
 *	bStatsBlksMax);
 *	bStackStart
 *	bStackMin);
 *	total);
 *	bFreeLeft);
 *
 */
	(*writefn)(handle, T("Initial free list size    %7d\n"), bFreeSize);
	(*writefn)(handle, T("Max memory malloced       %7d\n"), bStatsMemMalloc);
	(*writefn)(handle, T("Max memory ever used      %7d\n"), bStatsMemMax);
	(*writefn)(handle, T("Max memory ever balloced  %7d\n"), bStatsBallocMax);
	(*writefn)(handle, T("Memory currently in use   %7d\n"), bStatsMemInUse);
	(*writefn)(handle, T("Memory currently balloced %7d\n"), bStatsBallocInUse);
	(*writefn)(handle, T("Max blocks allocated      %7d\n"), bStatsBlksMax);
	(*writefn)(handle, T("Maximum stack used        %7d\n"), 
		(int) bStackStart - (int) bStackMin);

	(*writefn)(handle, T("Free memory on all queues %7d\n"), total);
	(*writefn)(handle, T("Free list buffer left     %7d\n"), bFreeLeft);
	(*writefn)(handle, T("Total free memory         %7d\n"), bFreeLeft + total);

/*
 *	Print per file allocation stats. Sort the copied table.
 */
	len = sizeof(bStatsFileType) * B_MAX_FILES;
	files = malloc(len);
	if (files == NULL) {
		(*writefn)(handle, T("Can't allocate stats memory\n"));
		recurseProtect--;
		return;
	}
	memcpy(files, bStatsFiles, len);
	qsort(files, bStatsFilesMax, sizeof(bStatsFileType), bStatsFileSort);
	
	(*writefn)(handle, T("\nMemory Currently Allocated\n"));
	total = 0;
	(*writefn)(handle, 
		T("                      bytes, blocks in use, total times,")
		T("largest,   q\n"));

	for (fp = files; fp < &files[bStatsFilesMax]; fp++) {
		if (fp->file[0]) {
			(*writefn)(handle, T("%18s, %7d,         %5d,      %6d, %7d,%4d\n"),
				fp->file, fp->allocated, fp->count, fp->times, fp->largest, 
				fp->q);
			total += fp->allocated;
		}
	}
	(*writefn)(handle, T("\nTotal allocated %7d\n\n"), total);

/*
 *	Dump the actual strings
 */
	(*writefn)(handle, T("\nStrings\n"));
	for (blkp = &bStatsBlks[bStatsBlksMax - 1]; blkp >= bStatsBlks; blkp--) {
		if (blkp->ptr) {
			cp = (char_t*) ((char*) blkp->ptr + sizeof(bType));
			fp = blkp->who;
			if (gisalnum(*cp)) {
				(*writefn)(handle, T("%-50s allocated by %s\n"), cp, 
					fp->file);
			}
		}
	}
	free(files);
	recurseProtect--;
}

/******************************************************************************/
/*
 *	File sort function. Used to sort per file stats
 */

static int bStatsFileSort(const void *cp1, const void *cp2)
{
	bStatsFileType	*s1, *s2;

	s1 = (bStatsFileType*) cp1;
	s2 = (bStatsFileType*) cp2;

	if (s1->allocated < s2->allocated)
		return -1;
	else if (s1->allocated == s2->allocated)
		return 0;
	return 1;
}

/******************************************************************************/
/*
 *	Accumulate allocation statistics
 */

static void bStatsAlloc(B_ARGS_DEC, void *ptr, int q, int size)
{
	int				memSize;
	bStatsFileType	*fp;
	bStatsBlkType	*bp;
	char_t			name[FNAMESIZE + 10];

	gsprintf(name, T("%s:%d"), B_ARGS);

	bStats[q].alloc++;
	bStats[q].inuse++;
	bStatsMemInUse += size;
	if (bStatsMemInUse > bStatsMemMax) {
		bStatsMemMax = bStatsMemInUse;
	}
	memSize = (1 << (B_SHIFT + q)) + sizeof(bType);
	bStatsBallocInUse += memSize;
	if (bStatsBallocInUse > bStatsBallocMax) {
		bStatsBallocMax = bStatsBallocInUse;
	}

/*
 *	Track maximum stack usage. Assumes a stack growth down. Approximate as
 *	we only measure this on block allocation.
 */
	if ((void*) &file < bStackMin) {
		bStackMin = (void*) &file;
	}

/*
 *	Find the file and adjust the stats for this file
 */
	for (fp = bStatsFiles; fp < &bStatsFiles[bStatsFilesMax]; fp++) {
		if (fp->file[0] == file[0] && gstrcmp(fp->file, name) == 0) {
			fp->allocated += size;
			fp->count++;
			fp->times++;
			if (fp->largest < size) {
				fp->largest = size;
				fp->q = q;
			}
			break;
		}
	}

/*
 *	New entry: find the first free slot and create a new entry
 */
	if (fp >= &bStatsFiles[bStatsFilesMax]) {
		for (fp = bStatsFiles; fp < &bStatsFiles[B_MAX_FILES]; fp++) {
			if (fp->file[0] == '\0') {
				gstrncpy(fp->file, name, TSZ(fp->file));
				fp->allocated += size;
				fp->count++;
				fp->times++;
				fp->largest = size;
				fp->q = q;
				if ((fp - bStatsFiles) >= bStatsFilesMax) {
					bStatsFilesMax = (fp - bStatsFiles) + 1;
				}
				break;
			}
		}
	}

/*
 *	Update the per block stats. Allocate a new slot.
 */
	for (bp = bStatsBlks; bp < &bStatsBlks[B_MAX_BLOCKS]; bp++) {
		if (bp->ptr == NULL) {
			bp->ptr = ptr;
			bp->who = fp;
			if ((bp - bStatsBlks) >= bStatsBlksMax) {
				bStatsBlksMax = (bp - bStatsBlks) + 1;
			}
			break;
		}
	}
}

/******************************************************************************/
/*
 *	Free statistics
 */

static void bStatsFree(B_ARGS_DEC, void *ptr, int q, int size)
{
	int				memSize;
	bStatsFileType	*fp;
	bStatsBlkType	*bp;

	memSize = (1 << (B_SHIFT + q)) + sizeof(bType);
	bStatsMemInUse -= size;
	bStatsBallocInUse -= memSize;
	bStats[q].inuse--;

/*
 *	Update the per block stats. Try from the end first 
 */
	for (bp = &bStatsBlks[bStatsBlksMax - 1]; bp >= bStatsBlks; bp--) {
		if (bp->ptr == ptr) {
			bp->ptr = NULL;
			fp = bp->who;
			bp->who = NULL;
			fp->allocated -= size;
			fp->count--;
			return;
		}
	}
}

/******************************************************************************/
/*
 *	Default output function. Just send to trace channel.
 */

#undef sprintf
static void bstatsWrite(int handle, char_t *fmt, ...)
{
	va_list		args;
	char_t		buf[BUF_MAX];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	traceRaw(buf);
}


#else /* not B_STATS */
/******************************************************************************/
/*
 *	Dummy bstats for external calls that aren't protected by #if B_STATS.
 */

void bstats(int handle, void (*writefn)(int handle, char_t *fmt, ...))
{
}
#endif /* B_STATS */

/******************************************************************************/
#ifdef B_VERIFY_CAUSES_SEVERE_OVERHEAD
/*
 *	The following routines verify the integrity of the balloc memory space.
 *	These functions use the B_FILL feature.  Corruption is defined
 *	as bad integrity flags in allocated blocks or data other than B_FILL_CHAR
 *	being found anywhere in the space which is unallocated and that is not a
 *	next pointer in the free queues. a_assert is called if any corruption is
 *	found.  CAUTION:  These functions add severe processing overhead and should
 *	only be used when searching for a tough corruption problem.
 */

/******************************************************************************/
/*
 *	verifyUsedBlock verifies that a block which was previously allocated is
 *	still uncorrupted.  
 */

static void verifyUsedBlock(bType *bp, int q)
{
	int		memSize, size;
	char	*p;

	memSize = (1 << (B_SHIFT + q)) + sizeof(bType);
	a_assert((bp->flags & ~B_MALLOCED) == B_INTEGRITY);
	size = bp->u.size;
	for (p = ((char *)bp)+sizeof(bType)+size; p < ((char*)bp)+memSize; p++) {
		a_assert(*p == B_FILL_CHAR);
	}
}

/******************************************************************************/
/*
 *	verifyFreeBlock verifies that a previously free'd block in one of the queues
 *	is still uncorrupted.
 */

static void verifyFreeBlock(bType *bp, int q)
{
	int		memSize;
	char	*p;

	memSize = (1 << (B_SHIFT + q)) + sizeof(bType);
	for (p = ((char *)bp)+sizeof(void*); p < ((char*)bp)+memSize; p++) {
		a_assert(*p == B_FILL_CHAR);
	}
	bp = (bType *)p;
	a_assert((bp->flags & ~B_MALLOCED) == B_INTEGRITY ||
		bp->flags == B_FILL_WORD);
}

/******************************************************************************/
/*
 *	verifyBallocSpace reads through the entire balloc memory space and
 *	verifies that all allocated blocks are uncorrupted and that, with the
 *	exception of free list next pointers, all other unallocated space is
 *	filled with B_FILL_CHAR.
 */

void verifyBallocSpace()
{
	int		q;
	char	*p;
	bType	*bp;

/*
 *	First verify all the free blocks.
 */
	for (q = 0; q < B_MAX_CLASS; q++) {	
		for (bp = bQhead[q]; bp != NULL; bp = bp->u.next) {
			verifyFreeBlock(bp, q);
		}
	}

/*
 *	Now verify other space
 */
	p = bFreeBuf;
	while (p < (bFreeBuf + bFreeSize)) {
		bp = (bType *)p;
		if (bp->u.size > 0xFFFFF) {
			p += sizeof(bp->u);
			while (p < (bFreeBuf + bFreeSize) && *p == B_FILL_CHAR) {
				p++;
			}
		} else {
			a_assert(((bp->flags & ~B_MALLOCED) == B_INTEGRITY) ||
				bp->flags == B_FILL_WORD);
			p += (sizeof(bType) + bp->u.size);
			while (p < (bFreeBuf + bFreeSize) && *p == B_FILL_CHAR) {
				p++;
			}
		}
	}
}
#endif /* B_VERIFY_CAUSES_SEVERE_OVERHEAD */

/******************************************************************************/

#else /* NO_BALLOC */
int bopen(void *buf, int bufsize, int flags)
{
	return 0;
}

/******************************************************************************/

void bclose()
{
}

/******************************************************************************/

void bstats(int handle, void (*writefn)(int handle, char_t *fmt, ...))
{
}

/******************************************************************************/

char_t *bstrdupNoBalloc(char_t *s)
{
#ifdef UNICODE
	if (s) {
		return wcsdup(s);
	} else {
		return wcsdup(T(""));
	}
#else
	return bstrdupANoBalloc(s);
#endif
}

/******************************************************************************/

char *bstrdupANoBalloc(char *s)
{
	char*	buf;

	if (s == NULL) {
		s = "";
	}
	buf = malloc(strlen(s)+1);
	strcpy(buf, s);
	return buf;
}

#endif /* NO_BALLOC */
/******************************************************************************/


