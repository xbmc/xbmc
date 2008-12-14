///
///	@file 	malloc.cpp
/// @brief 	Fast and safe malloc replacemen for embedded use
/// @overview This memory allocator is fast, thread-safe, SMP scaling. 
///		It tracks memory leaks and keeps allocation statistics with 
///		minimal overhead. It also extends the standard memory allocation 
///		API with "safer" APIs.
///	@remarks This module is thread-safe.
//
////////////////////////////////// Copyright ///////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
////////////////////////////////// Includes ////////////////////////////////////

#if BLD_FEATURE_MALLOC
#if WIN
#define malloc		_X_malloc
#define free		_X_free
#define calloc		_X_calloc
#define realloc		_X_realloc
#define strdup		_X_strdup
#define _msize		_X_msize
#define _expand		_X_expand
#define _heapchk	_X_heapchk
#define _heapmin	_X_heapmin
#define _heapwalk	_X_heapwalk
#define _heapset	_X_heapset
#endif
#endif

#include	"mpr.h"

#if WIN
#if BLD_FEATURE_MALLOC
#undef malloc
#undef free
#undef calloc 
#undef realloc
#undef strdup
#undef _msize
#undef _expand
#undef _heapchk
#undef _heapmin
#undef _heapwalk
#undef _heapset

#pragma warning(disable: 4074)	// initializers put in compiler reserved area
#pragma init_seg(compiler)
#pragma comment(linker, "/merge:.CRT=.data")

#define JUMP_OP						0xe9
#define MAKE_OFFSET(to,from)		((uint)((char*)(to)-(char*)(from)) - 5)

struct Hook 
{
	char		*functionName;
	FARPROC		newAddr;
	FARPROC		oldAddr;
	uchar		code[5];
};

namespace std {
	struct nothrow_t;
};

#define _CRTDBG_ALLOC_MEM_DF        0x01  /* Turn on debug allocation */
typedef int _CrtMemState;
typedef void(*_CRT_ALLOC_HOOK)(void);
typedef void(*_CRT_DUMP_CLIENT)(void);

#endif	//	BLD_FEATURE_MALLOC
#if !DOXYGEN
extern "C" BOOL WINAPI _CRT_INIT(HANDLE hinst, DWORD reason, LPVOID preserved);
#endif
#endif	// WIN

////////////////////////////////// Defines /////////////////////////////////////

#if BLD_FEATURE_MALLOC
#define MPR_DEFAULT_MEM_CHUNK	(4096)
#define MPR_FIRST_BLOCK			4
#define MPR_MAX_MEM_LIST		25						// 16 bytes to 256MB
#define MPR_ROUND(x)			(((x + sizeof(void*) - 1) / sizeof(void*)) * \
									sizeof(void*))
#if BLD_FEATURE_MALLOC_STATS
#define MPR_FILL_CHAR			0xdc
#define MPR_FREE_FILL_CHAR		0xc3
#define MPR_FREE_CHAR			0xa9
#define MPR_ZERO_CHAR			0xea
#define MPR_ALLOC_CHAR			0xb1
#define MPR_CHECK_FREE(x)		if ((x) != MPR_FREE_CHAR) memBreakPoint()
#define MPR_CHECK_ALLOC(x)		if ((x) != MPR_ALLOC_CHAR) memBreakPoint()
#define MPR_FILL(x, size)		memset((void*) x, MPR_FILL_CHAR, size)
#define MPR_FREE_FILL(x, size)	memset((void*) x, MPR_FREE_FILL_CHAR, size)
#define MPR_MARK_FREE(x)		x = MPR_FREE_CHAR
#define MPR_MARK_ALLOC(x)		x = MPR_ALLOC_CHAR
#define MPR_STACK(p)			statsStack(p);
#define MPR_STATS_ALLOC(bp, blk, size, addr)	\
								statsAllocBlk(bp, blk, size, addr)
#define MPR_STATS_FREE(bp, blk, size) statsFreeBlk(bp, blk, size);
#define MPR_STATS_NEW(bp, blk, size, addr) statsNewBlk(bp, blk, size, addr)
#define MPR_ZERO(x, size)		memset((void*) x, MPR_ZERO_CHAR, size)
#undef assert
#define assert(x)				if (!(x)) memBreakPoint()

#else	// !MALLOC_STATS
#define MPR_CHECK_ALLOC(x)
#define MPR_CHECK_FREE(x)
#define MPR_FILL(x, size)
#define MPR_FREE_FILL(x, size)
#define MPR_MARK_FREE(x)
#define MPR_MARK_ALLOC(x)
#define MPR_STACK(p)
#define MPR_STATS_ALLOC(bp, blk, size, addr)
#define MPR_STATS_FREE(bp, blk, size)
#define MPR_STATS_NEW(bp, blk, size, addr)
#define MPR_ZERO(x, size)
#undef assert
#define assert(x)
#endif	// MALLOC_STATS

struct Blk {
	union {
		Blk		*next;					// Free list next pointer
		int		size;					// Block size when in use
	};
#if BLD_FEATURE_MALLOC_LEAK
	void		*ip;
	Blk			*forw;
	Blk			*back;
#endif
	uchar		marker;
	char		bucketNum;
};

struct Bucket {
	Blk			*next;
	int			size;
#if BLD_FEATURE_MALLOC_STATS
	int			inUse;					// Count of blocks currently in use
	int			freeBlocks;				// Count of blocks on the free list
	int			count;					// Allocation count
	int			requested;				// Total requested bytes
#endif
};

struct Heap {
	void		*buf;
	int			size;
	void		*servp;
	int			available;
	Heap		*nextHeap;
#if BLD_FEATURE_MALLOC_STATS
	int			inUse;
	int			count;
#endif
};

#if BLD_FEATURE_MALLOC_STATS
struct Stats {
	int			allocInUse;
	int			allocMaxUse;
	int			heapInUse;
	int			heapMaxUse;
	void		*initialStack;
	int			mallocInUse;
	int			mallocMaxUse;
	void		*minStack;
	int			heapCount;
};
#endif

/////////////////////////////////// Locals /////////////////////////////////////

static Heap			firstHeap;
static Heap			*currentHeap;
static Heap			*depletedHeaps;
static Bucket		buckets[MPR_MAX_MEM_LIST];
static MprMemProc	userCback;
static int			defaultHeapSize;
static int			heapLimit;
static int			totalHeapMem;
static bool			printMemStats;

#if BLD_FEATURE_MULTITHREAD
static MprMutex		*mutex;
#endif

#if BLD_FEATURE_MALLOC_STATS
static Stats		stats;
#endif

#if WIN
static HANDLE		mprHeap;
#endif

#if BLD_FEATURE_MALLOC_LEAK
static Blk			blkHead;

static uint			stopOnSize = 0xffffffff;
static void			*stopOnAddr = 0;
static void			*stopOnBlkAddr = 0;
#endif

///////////////////////////// Forward Declarations /////////////////////////////

static void		transferHeapToBuckets(Heap *currentHeap);

#if BLD_FEATURE_MALLOC_STATS
static void		print(char *fmt, ...);
static void		statsNewBlk(Bucket *bp, Blk *blk, uint size, void *a);
static void		statsFreeBlk(Bucket *bp, Blk *blk, uint size);
static void		statsAllocBlk(Bucket *bp, Blk *blk, uint size, void *a);
static void		statsStack(void *p);
#endif

#if !BLD_FEATURE_MULTITHREAD
inline void		lock() {};
inline void		unlock() {};
#endif

///////////////////////////////////// Code /////////////////////////////////////
#if BLD_FEATURE_MALLOC_STATS

static void memBreakPoint()
{
	mprAssert(0 == "memBreakPoint");
	//	Set breakpoint here in debugger if you wish
}

#endif
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD

static inline void lock()
{
	if (mutex) {
		mutex->lock();
	}
}

////////////////////////////////////////////////////////////////////////////////

static inline void unlock()
{
	if (mutex) {
		mutex->unlock();
	}
}

#endif	// BLD_FEATURE_MULTITHREAD
////////////////////////////////////////////////////////////////////////////////

static inline void *heapAlloc(int size)
{
	void	*ptr;
#if WIN
	ptr = HeapAlloc(mprHeap, 0, size);
#elif CYGWIN || LINUX || MACOSX || FREEBSD
	ptr = sbrk(size);
#else
	ptr = malloc(size);
#endif
	if (ptr == 0) {
#if BLD_FEATURE_STATS
		memBreakPoint();
#endif
		if (userCback) {
			userCback(size, totalHeapMem, heapLimit);
		}
	}
	return ptr;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Initial setup for the mem module
//

static void initMem()
{
	int		bucketNum;

	currentHeap = &firstHeap;
#if BLD_FEATURE_MALLOC_STATS
	int		localVariable;
	stats.initialStack = &localVariable;
	stats.minStack = &localVariable;
#if BLD_FEATURE_MALLOC_LEAK
	blkHead.forw = blkHead.back = &blkHead;
	blkHead.size = 0;
#endif
#endif
	for (bucketNum = 0; bucketNum < MPR_MAX_MEM_LIST; bucketNum++) {
		buckets[bucketNum].size = (1 << (bucketNum + MPR_FIRST_BLOCK)) +
			sizeof(Blk);
		buckets[bucketNum].next = 0;
	}
#if WIN
	mprHeap = HeapCreate(0, 0, 0);
#endif
}

////////////////////////////////////////////////////////////////////////////////
extern "C" {

int mprCreateMemHeap(char *userBuf, int initialSize, int limit)
{
	assert(initialSize > 0);

	lock();
	if (currentHeap == 0) {
		initMem();
	}

	heapLimit = limit;
	defaultHeapSize = initialSize;

	if (currentHeap->buf) {
		transferHeapToBuckets(currentHeap);
	}

	if (userBuf == 0) {
		initialSize = MPR_ROUND(initialSize);
		if ((currentHeap->buf = heapAlloc(initialSize)) < 0) {
			unlock();
			return MPR_ERR_CANT_INITIALIZE;
		}
		totalHeapMem += initialSize;
		currentHeap->servp = currentHeap->buf;
		currentHeap->size = initialSize;
		currentHeap->available = initialSize;
		MPR_ZERO(currentHeap->buf, initialSize);

	} else {
		assert(currentHeap->buf == 0);
		initialSize = MPR_ROUND(initialSize);
		totalHeapMem += initialSize;
		currentHeap->buf = userBuf;
		currentHeap->servp = currentHeap->buf;
		currentHeap->size = initialSize;
		currentHeap->available = initialSize;
		MPR_ZERO(currentHeap->buf, initialSize);
	}

#if BLD_FEATURE_MALLOC_STATS
	stats.heapCount += 1;
#endif

#if BLD_FEATURE_MULTITHREAD
#if CYGWIN || LINUX || MACOSX || SOLARIS || VXWORKS || WIN || FREEBSD
	if (mutex == 0) {
		mutex = new MprMutex();
		return 0;
	}
#endif
#endif
	unlock();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void mprMemStop()
{
#if BLD_FEATURE_MULTITHREAD
	MprMutex	*mp;

	if (mutex) {
		mp = mutex;
		mutex = 0;
		delete mp;
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////

void mprMemClose()
{
	mprMemStop();
	if (printMemStats) {
		mprPrintMemStats();
	}
}

////////////////////////////////////////////////////////////////////////////////

void mprSetMemHandler(MprMemProc cback)
{
	userCback = cback;
}

////////////////////////////////////////////////////////////////////////////////

void *mprMalloc(uint size)
{
	Bucket		*bp;
	Blk			*blk;
	int			bucketNum, bucketSize, bucketBits, bytes;

	//
	//	PHP requests memory of size 0 -- Ugh !!
	//
	if (size == 0) {
		size = 1;
	}

#if BLD_FEATURE_MALLOC_STATS
	void	*addr;
	MPR_GET_RETURN(addr);
#if BLD_FEATURE_MALLOC_LEAK
	if (size == stopOnSize) {
		size = stopOnSize;
	}
	if (addr == stopOnAddr) {
		addr = stopOnAddr;
	}
#endif
#endif

	if (currentHeap == 0) {
		if (mprCreateMemHeap(0, MPR_DEFAULT_MEM_CHUNK, MAXINT) < 0) {
			return 0;
		}
	}

	assert(currentHeap > 0);
	MPR_STACK(&bucketNum);

	size = MPR_ROUND(size);
	bucketBits = (((uint) size - 1) >> MPR_FIRST_BLOCK);
	for (bucketNum = 0; bucketBits; bucketBits >>= 1) {
		bucketNum++; 
	}
	bucketSize = (1 << (bucketNum + MPR_FIRST_BLOCK)) + sizeof(Blk);
	assert(bucketNum < MPR_MAX_MEM_LIST);

	if (bucketNum >= MPR_MAX_MEM_LIST) {
		if (userCback) {
			userCback(size, totalHeapMem, heapLimit);
		}
		return 0;
	}

	lock();
	bp = &buckets[bucketNum];
	blk = bp->next;
	if (blk != 0) {
		//
		//	Take the first block off the free list
		//
		bp->next = blk->next;
		MPR_STATS_ALLOC(bp, blk, size, addr);
		unlock();
		MPR_CHECK_FREE(blk->marker);
		MPR_FILL((void*) ((int) blk + sizeof(Blk)), 
			bucketSize - sizeof(Blk));
		MPR_MARK_ALLOC(blk->marker);
		blk->size = size;
//		mprStaticPrintf("mprMalloc reuse %x size %d, bucket %d\n", 
//			((int) blk + sizeof(Blk)), blk->size, blk->bucketNum);
		return (void*) ((int) blk + sizeof(Blk));
	}

	if (currentHeap->available < bucketSize) {
		//
		//	No more memory available. Need to create a new heap
		//	Transfer remaining ram in depleted pool
		//
		transferHeapToBuckets(currentHeap);
		currentHeap->nextHeap = depletedHeaps;
		depletedHeaps = currentHeap;

		bytes = (sizeof(Heap) + bp->size + defaultHeapSize);
		defaultHeapSize *= 2;
		defaultHeapSize = min(defaultHeapSize, MPR_MAX_HEAP_SIZE);
		bytes = max(bytes, defaultHeapSize);
		bytes = min(bytes, (heapLimit - totalHeapMem));
		if (bytes < bucketSize) {
			assert(0);
			if (userCback) {
				userCback(size, totalHeapMem, heapLimit);
			}
			unlock();
			return 0;
		}

		currentHeap = (Heap*) heapAlloc(bytes);
		if (currentHeap == 0) {
			unlock();
			return 0;
		}
#if BLD_FEATURE_MALLOC_STATS
		totalHeapMem += bytes;
		stats.heapCount++;
#endif
		//
		//	Zero just to hebp with debugging
		//
		MPR_ZERO(currentHeap, bytes);
		currentHeap->buf = (Blk*) 
			((void*) ((int) currentHeap + sizeof(Heap)));
		currentHeap->servp = currentHeap->buf;
		currentHeap->size = bytes - sizeof(Heap);
		currentHeap->available = currentHeap->size;
	}

	//
	//	Craft a new block out of the existing heap
	//
	blk = (Blk*) currentHeap->servp;
	currentHeap->servp = (void*) ((int) currentHeap->servp + bucketSize);
	currentHeap->available -= bucketSize;
	MPR_STATS_NEW(bp, blk, size, addr);
	unlock();

	MPR_FILL((void*) ((int) blk + sizeof(Blk)), bucketSize - sizeof(Blk));
	MPR_MARK_ALLOC(blk->marker);

	blk->size = size;
	blk->bucketNum = bucketNum;

//	mprStaticPrintf("mprMalloc new alloc %x size %d, bucket %d\n", 
//		((int) blk + sizeof(Blk)), blk->size, blk->bucketNum);
	return (void*) ((int) blk + sizeof(Blk));
}

////////////////////////////////////////////////////////////////////////////////

void mprFree(void *ptr)
{
	Bucket		*bp;
	Blk			*blk;

	assert(currentHeap);

	if (ptr) {
		blk = (Blk*) ((int) ptr - sizeof(Blk));
#if BLD_FEATURE_MALLOC_LEAK
		if ((uint) blk->size == stopOnSize) {
			blk->size = stopOnSize;
		}
#endif
//		mprStaticPrintf("Free %x size %d, bucket %d\n", ptr, blk->size, 
//			blk->bucketNum);

		MPR_CHECK_ALLOC(blk->marker);
		MPR_MARK_FREE(blk->marker);
		assert(blk->bucketNum >= 0 && blk->bucketNum < MPR_MAX_MEM_LIST);

		lock();

		bp = &buckets[blk->bucketNum];
		MPR_FREE_FILL((void*) ((int) blk + sizeof(Blk)), 
			bp->size - sizeof(Blk));
		MPR_STATS_FREE(bp, blk, blk->size);

		blk->next = bp->next;
		bp->next = blk;

		unlock();
	}
}

////////////////////////////////////////////////////////////////////////////////

void *mprCalloc(uint numElem, uint size)
{
	void	*ptr;

	ptr = mprMalloc(numElem * size);
	memset(ptr, 0, numElem * size);
#if BLD_FEATURE_MALLOC_LEAK
	void	*addr;
	Blk *blk = (Blk*) ((int) ptr - sizeof(Blk));
	MPR_GET_RETURN(addr);
	blk->ip = addr;
#endif
	return ptr;
}

////////////////////////////////////////////////////////////////////////////////

void *mprRealloc(void *ptr, uint size)
{
	Blk			*blk;
	Bucket		*bp;
	void		*newPtr;

	if (ptr == 0) {
		newPtr = mprMalloc(size);
#if BLD_FEATURE_MALLOC_LEAK
		void	*addr;
		blk = (Blk*) ((int) newPtr - sizeof(Blk));
		MPR_GET_RETURN(addr);
		blk->ip = addr;
#endif
		return newPtr;
	}

	blk = (Blk*) ((int) ptr - sizeof(Blk));
	assert(blk->bucketNum >= 0 && blk->bucketNum < MPR_MAX_MEM_LIST);
	MPR_CHECK_ALLOC(blk->marker);

	bp = &buckets[blk->bucketNum];
	if (size <= ((uint) bp->size - sizeof(Blk))) {
		//
		//	If reducing the size, just adjust the allocated bytes
		//
#if BLD_FEATURE_MALLOC_LEAK
		void	*addr;
		MPR_GET_RETURN(addr);
#endif
		MPR_STATS_FREE(bp, blk, blk->size);
		MPR_STATS_ALLOC(bp, blk, size, addr);
		blk->size = size;
		return ptr;
	}
	newPtr = mprMalloc(size);
	memcpy(newPtr, ptr, blk->size);
	mprFree(ptr);

#if BLD_FEATURE_MALLOC_LEAK
	void	*addr;
	blk = (Blk*) ((int) newPtr - sizeof(Blk));
	MPR_GET_RETURN(addr);
	blk->ip = addr;
#endif
	return newPtr;
}

////////////////////////////////////////////////////////////////////////////////
#if UNUSED
int mprMemSize(void *ptr)
{
	Blk		*blk;

	blk = (Blk*) ((int) ptr - sizeof(Blk));
	assert(blk->bucketNum >= 0 && blk->bucketNum < MPR_MAX_MEM_LIST);
	MPR_CHECK_ALLOC(blk->marker);

	bp = &buckets[bucketNum];
	return (uint) bp->size - sizeof(Blk);
}
#endif
////////////////////////////////////////////////////////////////////////////////

char *mprStrdup(const char *str)
{
	char	*newStr;

	if (str == 0) {
		str = "";
	}
	newStr = (char*) mprMalloc(strlen(str) + 1);
	strcpy(newStr, str);
#if BLD_FEATURE_MALLOC_LEAK
	void	*addr;
	Blk	*blk = (Blk*) ((int) newStr - sizeof(Blk));
	MPR_GET_RETURN(addr);
	blk->ip = addr;
#endif
	return newStr;
}

}	//	extern "C"
////////////////////////////////////////////////////////////////////////////////
//
//	Must be called locked
//

static void transferHeapToBuckets(Heap *heap)
{
	Blk			*blk;
	Bucket		*bp;
	uint		bucketBits;
	int			bucketNum, bucketSize, available;

	while (1) {
		//
		//	Round down to the next lowest bucket size
		//
		available = heap->available - sizeof(Blk);
		if (available < 0) {
			return;
		}
		bucketBits = ((uint) available) >> (MPR_FIRST_BLOCK + 1);
		if (bucketBits == 0) {
			return;
		}
		for (bucketNum = 0; bucketBits; bucketBits >>= 1) {
			bucketNum++; 
		}
		bucketSize = (1 << (bucketNum + MPR_FIRST_BLOCK)) + sizeof(Blk);
		bp = &buckets[bucketNum];

		blk = (Blk*) heap->servp;
		blk->size = bucketSize - sizeof(Blk);
		blk->bucketNum = bucketNum;

		heap->servp = (void*) ((int) heap->servp + bucketSize);
		heap->available -= bucketSize;
		assert(heap->available >= 0);

#if BLD_FEATURE_MALLOC_STATS
		bp->freeBlocks++; 
		blk->forw = blk->back = 0;
#endif
		MPR_MARK_FREE(blk->marker);

		blk->next = bp->next;
		bp->next = blk;
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// C++ /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MALLOC_HOOK

void *operator new(size_t size)
{
	void	*ptr;

	ptr = mprMalloc(size);

#if BLD_FEATURE_MALLOC_LEAK
	char	*addr;
	Blk *blk = (Blk*) ((int) ptr - sizeof(Blk));
	MPR_GET_RETURN(addr);
	blk->ip = addr;
#endif
	return ptr;
}

////////////////////////////////////////////////////////////////////////////////

void *operator new[](size_t size)
{
	void	*ptr;

	ptr = mprMalloc(size);
#if BLD_FEATURE_MALLOC_LEAK
	char	*addr;
	Blk *blk = (Blk*) ((int) ptr - sizeof(Blk));
	MPR_GET_RETURN(addr);
	blk->ip = addr;
#endif
	return ptr;
}

////////////////////////////////////////////////////////////////////////////////

void operator delete(void *ptr)
{
	mprFree(ptr);
}

////////////////////////////////////////////////////////////////////////////////

void operator delete[](void *ptr)
{
	mprFree(ptr);
}

////////////////////////////////////////////////////////////////////////////////
#if CYGWIN || LINUX || MACOSX || SOLARIS || FREEBSD

#undef malloc
void *malloc(uint size)
{
	return mprMalloc(size);
}

////////////////////////////////////////////////////////////////////////////////

#undef free
void free(void *ptr)
{
	mprFree(ptr);
}

////////////////////////////////////////////////////////////////////////////////

#undef realloc
void *realloc(void *ptr, uint size)
{
	return mprRealloc(ptr, size);
}

////////////////////////////////////////////////////////////////////////////////

#undef calloc
void *calloc(uint numElem, uint size)
{
	return mprCalloc(numElem, size);
}

#endif // CYGWIN || LINUX || MACOSX || SOLARIS || FREEBSD
////////////////////////////////////////////////////////////////////////////////
#endif // BLD_FEATURE_MALLOC_HOOK
////////////////////////////////////////////////////////////////////////////////

extern "C" {

void mprRequestMemStats(bool on)
{
	printMemStats = on;
}

////////////////////////////////////////////////////////////////////////////////

void mprPrintMemStats()
{
#if BLD_FEATURE_MALLOC_STATS
	Bucket		*bp;
	int			bucketNum, avg;

	print("Memory Usage Summary\n");
	print("--------------------\n");
	print("Total heap memory        %8d\n", totalHeapMem);
	print("Total heaps              %8d\n", stats.heapCount);

	print("Malloced memory in use   %8d\n", stats.mallocInUse);
	print("Malloced memory max use  %8d\n", stats.mallocMaxUse);

	print("Allocated in use         %8d\n", stats.allocInUse);
	print("Aallocted max use        %8d "
		  "(inc. fragmentation)\n", stats.allocMaxUse);

#if UNUSED
	//
	//	Really can only test this for the main thread as other thread stacks
	//	are at wildly different locations
	//
	print("Maximum stack use        %8d\n", 
		(uint) stats.initialStack - (uint) stats.minStack);
#endif

	print("Per block overhead       %8d\n", sizeof(Blk));
	print("Total heap in use        %8d "
		  "(inc. overhead and round to power of 2)\n", stats.allocInUse);
	print("Heap max use             %8d\n", stats.allocMaxUse);

	print("Block fragmentation     %9.1f%%\n",
		stats.mallocMaxUse * 100.0 / stats.allocMaxUse);
	print("Heap efficiency         %9.1f%%\n\n",
		stats.mallocMaxUse * 100.0 / stats.heapMaxUse);

	print("Block Size Allocation Report\n");
	print("----------------------------------------"
	      "--------------------------------------\n");
	print("Real-Size Chunk-Size InUse   (bytes)    "
		  "Free    (bytes)   Requests    Avg-Size\n");
	for (bucketNum = 0; bucketNum < 22; bucketNum++) {
		bp = &buckets[bucketNum];
		if (bp->count == 0) {
			avg = 0;
		} else {
			avg = bp->requested / bp->count;
		}
		print("%9d  %9d %5d  %8d  %6d  %9d   %8d    %8d\n", 
			bp->size - sizeof(Blk), bp->size, bp->inUse, 
			bp->inUse * bp->size, bp->freeBlocks, 
			bp->freeBlocks * bp->size, bp->count, avg);
	}

#if BLD_FEATURE_MALLOC_LEAK
	Blk *blk = blkHead.forw;
	if (blk != &blkHead) {
		print("\nMemory Leak Report (Size, Address)\n");
		print(  "----------------------------------\n");
		print("    Size           IP    Blk Addr\n");
		do {
			print("%8d   0x%x    0x%x\n", blk->size, blk->ip, blk);
			blk = blk->forw;
		} while (blk != &blkHead);
	}
#endif
#endif
}

}	//	extern "C"
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MALLOC_STATS
//
//	Don't force a malloc here
//

static void print(char *fmt, ...)
{
	va_list		ap;
	char		buf[MPR_MAX_STRING];
	int			len;

	va_start(ap, fmt);
	len = mprVsprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (len >= 0) {
		write(1, buf, len);
	}
}

////////////////////////////////////////////////////////////////////////////////

static void statsNewBlk(Bucket *bp, Blk *blk, uint size, void *addr)
{
	bp->freeBlocks++;
	blk->forw = blk->back = 0;
	statsAllocBlk(bp, blk, size, addr);
}

////////////////////////////////////////////////////////////////////////////////

static void statsAllocBlk(Bucket *bp, Blk *blk, uint size, void *addr)
{
	bp->inUse++; 
	bp->freeBlocks--; 
	bp->count++; 
	bp->requested += size;

#if BLD_FEATURE_MALLOC_LEAK
	blk->ip = addr;
	if (blk->back != 0)  {
		memBreakPoint();
	}
	if (blk->forw != 0)  {
		memBreakPoint();
	}
	blk->forw = &blkHead;
	blk->back = blkHead.back;
	blkHead.back->forw = blk;
	blkHead.back = blk;
	blkHead.size++;

	if (blk == stopOnBlkAddr) {
		stopOnBlkAddr = stopOnBlkAddr;
	}
#endif

	stats.allocInUse += (bp->size - sizeof(Blk));
	if (stats.allocInUse > stats.allocMaxUse) {
		stats.allocMaxUse = stats.allocInUse;
	}
	stats.mallocInUse += size;
	if (stats.mallocInUse > stats.mallocMaxUse) {
		stats.mallocMaxUse = stats.mallocInUse;
	}
	stats.heapInUse += bp->size;
	if (stats.heapInUse > stats.heapMaxUse) {
		stats.heapMaxUse = stats.heapInUse;
	}
}

////////////////////////////////////////////////////////////////////////////////

static void statsFreeBlk(Bucket *bp, Blk *blk, uint size)
{
#if BLD_FEATURE_MALLOC_LEAK
	blk->forw->back = blk->back;
	blk->back->forw = blk->forw;
	blk->back = 0;
	blk->forw = 0;
	blk->ip = 0;
	blkHead.size--;
#endif

	bp->inUse--; 
	bp->freeBlocks++; 

	stats.allocInUse -= (bp->size - sizeof(Blk));
	stats.mallocInUse -= size;
	stats.heapInUse -= bp->size;
}

////////////////////////////////////////////////////////////////////////////////

static void statsStack(void *p)
{
	if (p < stats.minStack) {
		stats.minStack = p;
	}
}

#endif	// BLD_FEATURE_MALLOC_STATS
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// Windows //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if WIN
extern "C" {

__declspec(dllexport) size_t mprMemMsize(void *memblock)
{
	Bucket		*bp;
	Blk			*blk;

	blk = (Blk*) ((int) memblock - sizeof(Blk));
	bp = &buckets[blk->bucketNum];
	return bp->size - sizeof(blk);
}

////////////////////////////////////////////////////////////////////////////////

void *mprMemExpand(void * mem, size_t sz) 
{ 
	return NULL; 
}

////////////////////////////////////////////////////////////////////////////////

int mprMemHeapchk(void) 
{ 
	return _HEAPOK; 
}

////////////////////////////////////////////////////////////////////////////////

int mprMemHeapmin(void) 
{ 
	return 0; 
}

////////////////////////////////////////////////////////////////////////////////

int mprMemHeapset(unsigned int) 
{ 
	return _HEAPOK; 
}

////////////////////////////////////////////////////////////////////////////////

int mprMemHeapwalk(_HEAPINFO *) 
{ 
	return _HEAPEND; 
}

////////////////////////////////////////////////////////////////////////////////

void *mprNewNoThrow(uint sz, struct std::nothrow_t const &) 
{
	return mprMalloc(sz);
}

////////////////////////////////////////////////////////////////////////////////

void *mprMemNhMalloc(size_t sz, size_t, int, int, const char *, int) 
{
	return mprMalloc(sz);
}

////////////////////////////////////////////////////////////////////////////////

int mprCrtCheckMemory(void) 
{ 
	return 1; 
}

////////////////////////////////////////////////////////////////////////////////

void mprCrtDoForAllClientObjects(void(*pfn)(void*,void*), 
	void *context) 
{ 
}

////////////////////////////////////////////////////////////////////////////////

int mprCrtDumpMemoryLeaks(void) 
{ 
	return 0; 
}

////////////////////////////////////////////////////////////////////////////////

int mprCrtIsMemoryBlock(const void *userData, uint size, 
	long *requestNumber, char **filename, int *linenumber) 
{ 
	return 1; 
}

////////////////////////////////////////////////////////////////////////////////

int mprCrtIsValidHeapPointer(const void *userData) 
{ 
	return 1; 
}

////////////////////////////////////////////////////////////////////////////////

void mprCrtMemCheckpoint(_CrtMemState *state) 
{ 
}

////////////////////////////////////////////////////////////////////////////////

int mprCrtMemDifference(_CrtMemState *stateDiff, 
	const _CrtMemState *oldState, const _CrtMemState *newState) 
{ 
	return 0; 
}

////////////////////////////////////////////////////////////////////////////////

void mprCrtMemDumpAllObjectsSince(const _CrtMemState *state) 
{ 
}

////////////////////////////////////////////////////////////////////////////////

void mprCrtMemDumpStatistics(const _CrtMemState *state) 
{ 
}

////////////////////////////////////////////////////////////////////////////////

_CRT_ALLOC_HOOK mprCrtSetAllocHook(_CRT_ALLOC_HOOK allocHook) 
{ 
	return 0; 
}

////////////////////////////////////////////////////////////////////////////////

long mprCrtSetBreakAlloc(long lBreakAlloc) 
{ 
	return 0; 
}

////////////////////////////////////////////////////////////////////////////////

int mprCrtSetDbgFlag(int newFlag) 
{ 
	return _CRTDBG_ALLOC_MEM_DF; 
}

////////////////////////////////////////////////////////////////////////////////

_CRT_DUMP_CLIENT mprCrtSetDumpClient(_CRT_DUMP_CLIENT dumpClient) 
{ 
	return 0; 
}

////////////////////////////////////////////////////////////////////////////////
//
//	Called by the MS CRT on exit. These blocks are allocated by the MS heap
//	FUTURE -- really should unpatch our patches and let them free it normally
//

void mprFreeDebug(void *ptr)
{ 
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MALLOC_HOOK

static Hook hooks[] = {
	{"??2@YAPAXI@Z",					(FARPROC) mprMalloc,			0},
	{"??_U@YAPAXI@Z",					(FARPROC) mprMalloc,			0},
	{"??3@YAXPAX@Z",					(FARPROC) mprFree,				0},
	{"??_V@YAXPAX@Z",					(FARPROC) mprFree,				0},
	{"??2@YAPAXIABUnothrow_t@std@@@Z",	(FARPROC) mprNewNoThrow,		0},
	{"??_U@YAPAXIABUnothrow_t@std@@@Z", (FARPROC) mprNewNoThrow,		0},
	{"_expand",							(FARPROC) mprMemExpand,			0},
	{"_heapchk",						(FARPROC) mprMemHeapchk,		0},
	{"_heapmin",						(FARPROC) mprMemHeapmin,		0},
	{"_heapset",						(FARPROC) mprMemHeapset,		0},
	{"_heapwalk",						(FARPROC) mprMemHeapwalk,		0},
	{"_msize",							(FARPROC) mprMemMsize,			0},
	{"calloc",							(FARPROC) mprCalloc,			0},
	{"malloc",							(FARPROC) mprMalloc,			0},
	{"realloc",							(FARPROC) mprRealloc,			0},
	{"free",							(FARPROC) mprFree,				0},

	{"_CrtCheckMemory",					(FARPROC) mprCrtCheckMemory,	0},
	{"_CrtDoForAllClientObjects",		(FARPROC) 
											mprCrtDoForAllClientObjects,0},
	{"_CrtDumpMemoryLeaks",				(FARPROC) mprCrtDumpMemoryLeaks,0},
	{"_CrtIsMemoryBlock",				(FARPROC) mprCrtIsMemoryBlock,	0},
	{"_CrtIsValidHeapPointer",			(FARPROC) mprCrtIsValidHeapPointer,	0},
	{"_CrtMemCheckpoint",				(FARPROC) mprCrtMemCheckpoint,	0},
	{"_CrtMemDifference",				(FARPROC) mprCrtMemDifference,	0},
	{"_CrtMemDumpAllObjectsSince",		(FARPROC) 
											mprCrtMemDumpAllObjectsSince,0},
	{"_CrtMemDumpStatistics",			(FARPROC) mprCrtMemDumpStatistics,0},
	{"_CrtSetAllocHook",				(FARPROC) mprCrtSetAllocHook,	0},
	{"_CrtSetBreakAlloc",				(FARPROC) mprCrtSetBreakAlloc,	0},
	{"_CrtSetDbgFlag",					(FARPROC) mprCrtSetDbgFlag,		0},
	{"_CrtSetDumpClient",				(FARPROC) mprCrtSetDumpClient,	0},
	{"_expand_dbg",						(FARPROC) mprMemExpand,			0},
	{"_free_dbg",						(FARPROC) mprFreeDebug,			0},
	{"_malloc_dbg",						(FARPROC) mprMalloc,			0},
	{"_msize",							(FARPROC) mprMemMsize,			0},
	{"_msize_dbg",						(FARPROC) mprMemMsize,			0},
	{"_realloc_dbg",					(FARPROC) mprRealloc,			0},
	{"_calloc_dbg",						(FARPROC) mprCalloc,			0},
	{"??2@YAPAXIHPBDH@Z",               (FARPROC) mprMalloc,			0},
	{"??3@YAXPAXHPBDH@Z",               (FARPROC) mprFree,				0},
	{"_nh_malloc_dbg",                  (FARPROC) mprMalloc,    		0},
};

////////////////////////////////////////////////////////////////////////////////

static void hookFunction(Hook *hook)
{
	MEMORY_BASIC_INFORMATION	info;
	uchar						*targetOp;
	uint						*targetAddr;

	VirtualQuery((void*) hook->oldAddr, &info, 
			sizeof(MEMORY_BASIC_INFORMATION));
	VirtualProtect(info.BaseAddress, info.RegionSize, PAGE_EXECUTE_READWRITE, 
		&info.Protect);

	memcpy(hook->code, hook->oldAddr, sizeof(hook->code));
	targetOp = (uchar*) hook->oldAddr;
	*targetOp = JUMP_OP;
	targetAddr = (uint*) (targetOp + 1);
	*targetAddr = MAKE_OFFSET(hook->newAddr, hook->oldAddr);
	
	VirtualProtect(info.BaseAddress, info.RegionSize, info.Protect, 
		&info.Protect);
}

////////////////////////////////////////////////////////////////////////////////

void mprHookMalloc(void)
{
#if BLD_FEATURE_MALLOC_HOOK
	static int	hooked = 0;
	int			i;
	HMODULE		crt;

#if BLD_DEBUG
	crt = GetModuleHandle("MSVCRTD.DLL");
#else
	crt = GetModuleHandle("MSVCRT.DLL");
#endif
	for (i = 0; i < sizeof(hooks) / sizeof(*hooks); i++) {
		hooks[i].oldAddr = GetProcAddress(crt, hooks[i].functionName);
		if (hooks[i].oldAddr != 0) {
			hookFunction(&hooks[i]);
		}
	}
	hooked++;
#endif
}

#endif
////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI mprDllMain(HANDLE hinst, DWORD reason, LPVOID preserved)
{
	switch (reason)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls((HMODULE) hinst);

#if BLD_FEATURE_MALLOC_HOOK
			mprHookMalloc();
#endif

			mprCreateMemHeap(0, MPR_DEFAULT_MEM_CHUNK, MAXINT);
			if (!_CRT_INIT(hinst, reason, preserved)) {
				return FALSE;
			}
			if (mutex == 0) {
				mutex = new MprMutex();
			}
			return TRUE;

		case DLL_PROCESS_DETACH:
			// 
			//	Now called explicitly in the application
			//
			//		mprMemClose();
			//
			return TRUE;
	}
	return FALSE;
}

}		//	extern "C"
#endif	//	WIN

////////////////////////////////////////////////////////////////////////////////
#else	// !BLD_FEATURE_MALLOC
extern "C" {
////////////////////////////////////////////////////////////////////////////////

int mprCreateMemHeap(char *userBuf, int initialSize, int limit)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void mprSetMemHandler(MprMemProc cback)
{
}

////////////////////////////////////////////////////////////////////////////////

int mprMemOpen(char *userBuf, int bufsize, int limit, MprMemProc cback)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void mprStop()
{
}

////////////////////////////////////////////////////////////////////////////////

void mprMemClose()
{
}

////////////////////////////////////////////////////////////////////////////////

void mprMemStop()
{
}

////////////////////////////////////////////////////////////////////////////////

void mprRequestMemStats(bool on)
{
}

////////////////////////////////////////////////////////////////////////////////

void mprStats()
{
}

////////////////////////////////////////////////////////////////////////////////

void *mprMalloc(uint size)
{
	#undef malloc
	return malloc(size);
}

////////////////////////////////////////////////////////////////////////////////

void mprFree(void *ptr)
{
	#undef free
	if (ptr) {
		free(ptr);
	}
}

////////////////////////////////////////////////////////////////////////////////

void *mprRealloc(void *ptr, uint size)
{
	#undef realloc
	if (ptr == 0) {
		return malloc(size);
	}
	return realloc(ptr, size);
}

////////////////////////////////////////////////////////////////////////////////

void *mprCalloc(uint numElem, uint size)
{
	void	*ptr;

	ptr = mprMalloc(numElem * size);
	memset(ptr, 0, numElem * size);
	return ptr;
}

////////////////////////////////////////////////////////////////////////////////
#if UNUSED
int mprMemSize(void *ptr)
{
#if WIN
	return _msize(ptr);
#else
	return 0;
#endif
}
#endif
////////////////////////////////////////////////////////////////////////////////

char *mprStrdup(const char *str)
{
	if (str == 0) {
		str = "";
	}
#if VXWORKS
	char	*s;

	s = (char*) malloc(strlen(str) + 1);
	strcpy(s, str);
	return s;
#else
	return strdup(str);
#endif
}

////////////////////////////////////////////////////////////////////////////////

void mprPrintMemStats()
{
}

////////////////////////////////////////////////////////////////////////////////
#if WIN
BOOL WINAPI mprDllMain(HANDLE hinst, DWORD reason, LPVOID preserved)
{
	switch (reason)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls((HMODULE) hinst);
			if (!_CRT_INIT(hinst, reason, preserved)) {
				return FALSE;
			}
			return TRUE;

		case DLL_PROCESS_DETACH:
			return TRUE;
	}
	return FALSE;
}

#endif	//	WIN
////////////////////////////////////////////////////////////////////////////////
}		// extern "C"
#endif	// BLD_FEATURE_MALLOC

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
