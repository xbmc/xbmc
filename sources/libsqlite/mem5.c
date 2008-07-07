/*
** 2007 October 14
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains the C functions that implement a memory
** allocation subsystem for use by SQLite. 
**
** This version of the memory allocation subsystem omits all
** use of malloc().  All dynamically allocatable memory is
** contained in a static array, mem.aPool[].  The size of this
** fixed memory pool is SQLITE_POW2_MEMORY_SIZE bytes.
**
** This version of the memory allocation subsystem is used if
** and only if SQLITE_POW2_MEMORY_SIZE is defined.
**
** $Id: mem5.c,v 1.4 2008/02/19 15:15:16 drh Exp $
*/
#include "sqliteInt.h"

/*
** This version of the memory allocator is used only when 
** SQLITE_POW2_MEMORY_SIZE is defined.
*/
#ifdef SQLITE_POW2_MEMORY_SIZE

/*
** Log2 of the minimum size of an allocation.  For example, if
** 4 then all allocations will be rounded up to at least 16 bytes.
** If 5 then all allocations will be rounded up to at least 32 bytes.
*/
#ifndef SQLITE_POW2_LOGMIN
# define SQLITE_POW2_LOGMIN 6
#endif
#define POW2_MIN (1<<SQLITE_POW2_LOGMIN)

/*
** Log2 of the maximum size of an allocation.
*/
#ifndef SQLITE_POW2_LOGMAX
# define SQLITE_POW2_LOGMAX 18
#endif
#define POW2_MAX (((unsigned int)1)<<SQLITE_POW2_LOGMAX)

/*
** Number of distinct allocation sizes.
*/
#define NSIZE (SQLITE_POW2_LOGMAX - SQLITE_POW2_LOGMIN + 1)

/*
** A minimum allocation is an instance of the following structure.
** Larger allocations are an array of these structures where the
** size of the array is a power of 2.
*/
typedef struct Mem5Block Mem5Block;
struct Mem5Block {
  union {
    char aData[POW2_MIN];
    struct {
      int next;       /* Index in mem.aPool[] of next free chunk */
      int prev;       /* Index in mem.aPool[] of previous free chunk */
    } list;
  } u;
};

/*
** Number of blocks of memory available for allocation.
*/
#define NBLOCK (SQLITE_POW2_MEMORY_SIZE/POW2_MIN)

/*
** The size in blocks of an POW2_MAX allocation
*/
#define SZ_MAX (1<<(NSIZE-1))

/*
** Masks used for mem.aCtrl[] elements.
*/
#define CTRL_LOGSIZE  0x1f    /* Log2 Size of this block relative to POW2_MIN */
#define CTRL_FREE     0x20    /* True if not checked out */

/*
** All of the static variables used by this module are collected
** into a single structure named "mem".  This is to keep the
** static variables organized and to reduce namespace pollution
** when this module is combined with other in the amalgamation.
*/
static struct {
  /*
  ** The alarm callback and its arguments.  The mem.mutex lock will
  ** be held while the callback is running.  Recursive calls into
  ** the memory subsystem are allowed, but no new callbacks will be
  ** issued.  The alarmBusy variable is set to prevent recursive
  ** callbacks.
  */
  sqlite3_int64 alarmThreshold;
  void (*alarmCallback)(void*, sqlite3_int64,int);
  void *alarmArg;
  int alarmBusy;
  
  /*
  ** Mutex to control access to the memory allocation subsystem.
  */
  sqlite3_mutex *mutex;

  /*
  ** Performance statistics
  */
  u64 nAlloc;         /* Total number of calls to malloc */
  u64 totalAlloc;     /* Total of all malloc calls - includes internal frag */
  u64 totalExcess;    /* Total internal fragmentation */
  u32 currentOut;     /* Current checkout, including internal fragmentation */
  u32 currentCount;   /* Current number of distinct checkouts */
  u32 maxOut;         /* Maximum instantaneous currentOut */
  u32 maxCount;       /* Maximum instantaneous currentCount */
  u32 maxRequest;     /* Largest allocation (exclusive of internal frag) */
  
  /*
  ** Lists of free blocks of various sizes.
  */
  int aiFreelist[NSIZE];

  /*
  ** Space for tracking which blocks are checked out and the size
  ** of each block.  One byte per block.
  */
  u8 aCtrl[NBLOCK];

  /*
  ** Memory available for allocation
  */
  Mem5Block aPool[NBLOCK];
} mem;

/*
** Unlink the chunk at mem.aPool[i] from list it is currently
** on.  It should be found on mem.aiFreelist[iLogsize].
*/
static void memsys5Unlink(int i, int iLogsize){
  int next, prev;
  assert( i>=0 && i<NBLOCK );
  assert( iLogsize>=0 && iLogsize<NSIZE );
  assert( (mem.aCtrl[i] & CTRL_LOGSIZE)==iLogsize );
  assert( sqlite3_mutex_held(mem.mutex) );

  next = mem.aPool[i].u.list.next;
  prev = mem.aPool[i].u.list.prev;
  if( prev<0 ){
    mem.aiFreelist[iLogsize] = next;
  }else{
    mem.aPool[prev].u.list.next = next;
  }
  if( next>=0 ){
    mem.aPool[next].u.list.prev = prev;
  }
}

/*
** Link the chunk at mem.aPool[i] so that is on the iLogsize
** free list.
*/
static void memsys5Link(int i, int iLogsize){
  int x;
  assert( sqlite3_mutex_held(mem.mutex) );
  assert( i>=0 && i<NBLOCK );
  assert( iLogsize>=0 && iLogsize<NSIZE );
  assert( (mem.aCtrl[i] & CTRL_LOGSIZE)==iLogsize );

  mem.aPool[i].u.list.next = x = mem.aiFreelist[iLogsize];
  mem.aPool[i].u.list.prev = -1;
  if( x>=0 ){
    assert( x<NBLOCK );
    mem.aPool[x].u.list.prev = i;
  }
  mem.aiFreelist[iLogsize] = i;
}

/*
** Enter the mutex mem.mutex. Allocate it if it is not already allocated.
**
** Also:  Initialize the memory allocation subsystem the first time
** this routine is called.
*/
static void memsys5Enter(void){
  if( mem.mutex==0 ){
    int i;
    assert( sizeof(Mem5Block)==POW2_MIN );
    assert( (SQLITE_POW2_MEMORY_SIZE % POW2_MAX)==0 );
    assert( SQLITE_POW2_MEMORY_SIZE>=POW2_MAX );
    mem.mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MEM);
    sqlite3_mutex_enter(mem.mutex);
    for(i=0; i<NSIZE; i++) mem.aiFreelist[i] = -1;
    for(i=0; i<=NBLOCK-SZ_MAX; i += SZ_MAX){
      mem.aCtrl[i] = (NSIZE-1) | CTRL_FREE;
      memsys5Link(i, NSIZE-1);
    }
  }else{
    sqlite3_mutex_enter(mem.mutex);
  }
}

/*
** Return the amount of memory currently checked out.
*/
sqlite3_int64 sqlite3_memory_used(void){
  return mem.currentOut;
}

/*
** Return the maximum amount of memory that has ever been
** checked out since either the beginning of this process
** or since the most recent reset.
*/
sqlite3_int64 sqlite3_memory_highwater(int resetFlag){
  sqlite3_int64 n;
  memsys5Enter();
  n = mem.maxOut;
  if( resetFlag ){
    mem.maxOut = mem.currentOut;
  }
  sqlite3_mutex_leave(mem.mutex);  
  return n;
}


/*
** Trigger the alarm 
*/
static void memsys5Alarm(int nByte){
  void (*xCallback)(void*,sqlite3_int64,int);
  sqlite3_int64 nowUsed;
  void *pArg;
  if( mem.alarmCallback==0 || mem.alarmBusy  ) return;
  mem.alarmBusy = 1;
  xCallback = mem.alarmCallback;
  nowUsed = mem.currentOut;
  pArg = mem.alarmArg;
  sqlite3_mutex_leave(mem.mutex);
  xCallback(pArg, nowUsed, nByte);
  sqlite3_mutex_enter(mem.mutex);
  mem.alarmBusy = 0;
}

/*
** Change the alarm callback.
**
** This is a no-op for the static memory allocator.  The purpose
** of the memory alarm is to support sqlite3_soft_heap_limit().
** But with this memory allocator, the soft_heap_limit is really
** a hard limit that is fixed at SQLITE_POW2_MEMORY_SIZE.
*/
int sqlite3_memory_alarm(
  void(*xCallback)(void *pArg, sqlite3_int64 used,int N),
  void *pArg,
  sqlite3_int64 iThreshold
){
  memsys5Enter();
  mem.alarmCallback = xCallback;
  mem.alarmArg = pArg;
  mem.alarmThreshold = iThreshold;
  sqlite3_mutex_leave(mem.mutex);
  return SQLITE_OK;
}

/*
** Return the size of an outstanding allocation, in bytes.  The
** size returned omits the 8-byte header overhead.  This only
** works for chunks that are currently checked out.
*/
int sqlite3MallocSize(void *p){
  int iSize = 0;
  if( p ){
    int i = ((Mem5Block*)p) - mem.aPool;
    assert( i>=0 && i<NBLOCK );
    iSize = 1 << ((mem.aCtrl[i]&CTRL_LOGSIZE) + SQLITE_POW2_LOGMIN);
  }
  return iSize;
}

/*
** Find the first entry on the freelist iLogsize.  Unlink that
** entry and return its index. 
*/
static int memsys5UnlinkFirst(int iLogsize){
  int i;
  int iFirst;

  assert( iLogsize>=0 && iLogsize<NSIZE );
  i = iFirst = mem.aiFreelist[iLogsize];
  assert( iFirst>=0 );
  while( i>0 ){
    if( i<iFirst ) iFirst = i;
    i = mem.aPool[i].u.list.next;
  }
  memsys5Unlink(iFirst, iLogsize);
  return iFirst;
}

/*
** Return a block of memory of at least nBytes in size.
** Return NULL if unable.
*/
static void *memsys5Malloc(int nByte){
  int i;           /* Index of a mem.aPool[] slot */
  int iBin;        /* Index into mem.aiFreelist[] */
  int iFullSz;     /* Size of allocation rounded up to power of 2 */
  int iLogsize;    /* Log2 of iFullSz/POW2_MIN */

  assert( sqlite3_mutex_held(mem.mutex) );

  /* Keep track of the maximum allocation request.  Even unfulfilled
  ** requests are counted */
  if( nByte>mem.maxRequest ){
    mem.maxRequest = nByte;
  }

  /* Simulate a memory allocation fault */
  if( sqlite3FaultStep(SQLITE_FAULTINJECTOR_MALLOC) ) return 0;

  /* Round nByte up to the next valid power of two */
  if( nByte>POW2_MAX ) return 0;
  for(iFullSz=POW2_MIN, iLogsize=0; iFullSz<nByte; iFullSz *= 2, iLogsize++){}

  /* If we will be over the memory alarm threshold after this allocation,
  ** then trigger the memory overflow alarm */
  if( mem.alarmCallback!=0 && mem.currentOut+iFullSz>=mem.alarmThreshold ){
    memsys5Alarm(iFullSz);
  }

  /* Make sure mem.aiFreelist[iLogsize] contains at least one free
  ** block.  If not, then split a block of the next larger power of
  ** two in order to create a new free block of size iLogsize.
  */
  for(iBin=iLogsize; mem.aiFreelist[iBin]<0 && iBin<NSIZE; iBin++){}
  if( iBin>=NSIZE ) return 0;
  i = memsys5UnlinkFirst(iBin);
  while( iBin>iLogsize ){
    int newSize;

    iBin--;
    newSize = 1 << iBin;
    mem.aCtrl[i+newSize] = CTRL_FREE | iBin;
    memsys5Link(i+newSize, iBin);
  }
  mem.aCtrl[i] = iLogsize;

  /* Update allocator performance statistics. */
  mem.nAlloc++;
  mem.totalAlloc += iFullSz;
  mem.totalExcess += iFullSz - nByte;
  mem.currentCount++;
  mem.currentOut += iFullSz;
  if( mem.maxCount<mem.currentCount ) mem.maxCount = mem.currentCount;
  if( mem.maxOut<mem.currentOut ) mem.maxOut = mem.currentOut;

  /* Return a pointer to the allocated memory. */
  return (void*)&mem.aPool[i];
}

/*
** Free an outstanding memory allocation.
*/
void memsys5Free(void *pOld){
  u32 size, iLogsize;
  int i;

  i = ((Mem5Block*)pOld) - mem.aPool;
  assert( sqlite3_mutex_held(mem.mutex) );
  assert( i>=0 && i<NBLOCK );
  assert( (mem.aCtrl[i] & CTRL_FREE)==0 );
  iLogsize = mem.aCtrl[i] & CTRL_LOGSIZE;
  size = 1<<iLogsize;
  assert( i+size-1<NBLOCK );
  mem.aCtrl[i] |= CTRL_FREE;
  mem.aCtrl[i+size-1] |= CTRL_FREE;
  assert( mem.currentCount>0 );
  assert( mem.currentOut>=0 );
  mem.currentCount--;
  mem.currentOut -= size*POW2_MIN;
  assert( mem.currentOut>0 || mem.currentCount==0 );
  assert( mem.currentCount>0 || mem.currentOut==0 );

  mem.aCtrl[i] = CTRL_FREE | iLogsize;
  while( iLogsize<NSIZE-1 ){
    int iBuddy;

    if( (i>>iLogsize) & 1 ){
      iBuddy = i - size;
    }else{
      iBuddy = i + size;
    }
    assert( iBuddy>=0 && iBuddy<NBLOCK );
    if( mem.aCtrl[iBuddy]!=(CTRL_FREE | iLogsize) ) break;
    memsys5Unlink(iBuddy, iLogsize);
    iLogsize++;
    if( iBuddy<i ){
      mem.aCtrl[iBuddy] = CTRL_FREE | iLogsize;
      mem.aCtrl[i] = 0;
      i = iBuddy;
    }else{
      mem.aCtrl[i] = CTRL_FREE | iLogsize;
      mem.aCtrl[iBuddy] = 0;
    }
    size *= 2;
  }
  memsys5Link(i, iLogsize);
}

/*
** Allocate nBytes of memory
*/
void *sqlite3_malloc(int nBytes){
  sqlite3_int64 *p = 0;
  if( nBytes>0 ){
    memsys5Enter();
    p = memsys5Malloc(nBytes);
    sqlite3_mutex_leave(mem.mutex);
  }
  return (void*)p; 
}

/*
** Free memory.
*/
void sqlite3_free(void *pPrior){
  if( pPrior==0 ){
    return;
  }
  assert( mem.mutex!=0 );
  sqlite3_mutex_enter(mem.mutex);
  memsys5Free(pPrior);
  sqlite3_mutex_leave(mem.mutex);  
}

/*
** Change the size of an existing memory allocation
*/
void *sqlite3_realloc(void *pPrior, int nBytes){
  int nOld;
  void *p;
  if( pPrior==0 ){
    return sqlite3_malloc(nBytes);
  }
  if( nBytes<=0 ){
    sqlite3_free(pPrior);
    return 0;
  }
  assert( mem.mutex!=0 );
  nOld = sqlite3MallocSize(pPrior);
  if( nBytes<=nOld ){
    return pPrior;
  }
  sqlite3_mutex_enter(mem.mutex);
  p = memsys5Malloc(nBytes);
  if( p ){
    memcpy(p, pPrior, nOld);
    memsys5Free(pPrior);
  }
  sqlite3_mutex_leave(mem.mutex);
  return p;
}

/*
** Open the file indicated and write a log of all unfreed memory 
** allocations into that log.
*/
void sqlite3MemdebugDump(const char *zFilename){
#ifdef SQLITE_DEBUG
  FILE *out;
  int i, j, n;

  if( zFilename==0 || zFilename[0]==0 ){
    out = stdout;
  }else{
    out = fopen(zFilename, "w");
    if( out==0 ){
      fprintf(stderr, "** Unable to output memory debug output log: %s **\n",
                      zFilename);
      return;
    }
  }
  memsys5Enter();
  for(i=0; i<NSIZE; i++){
    for(n=0, j=mem.aiFreelist[i]; j>=0; j = mem.aPool[j].u.list.next, n++){}
    fprintf(out, "freelist items of size %d: %d\n", POW2_MIN << i, n);
  }
  fprintf(out, "mem.nAlloc       = %llu\n", mem.nAlloc);
  fprintf(out, "mem.totalAlloc   = %llu\n", mem.totalAlloc);
  fprintf(out, "mem.totalExcess  = %llu\n", mem.totalExcess);
  fprintf(out, "mem.currentOut   = %u\n", mem.currentOut);
  fprintf(out, "mem.currentCount = %u\n", mem.currentCount);
  fprintf(out, "mem.maxOut       = %u\n", mem.maxOut);
  fprintf(out, "mem.maxCount     = %u\n", mem.maxCount);
  fprintf(out, "mem.maxRequest   = %u\n", mem.maxRequest);
  sqlite3_mutex_leave(mem.mutex);
  if( out==stdout ){
    fflush(stdout);
  }else{
    fclose(out);
  }
#endif
}


#endif /* !SQLITE_POW2_MEMORY_SIZE */
