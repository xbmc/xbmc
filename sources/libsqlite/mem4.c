/*
** 2007 August 14
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
** $Id: mem4.c,v 1.2 2008/02/14 23:26:56 drh Exp $
*/
#include "sqliteInt.h"

/*
** This version of the memory allocator attempts to obtain memory
** from mmap() if the size of the allocation is close to the size
** of a virtual memory page.  If the size of the allocation is different
** from the virtual memory page size, then ordinary malloc() is used.
** Ordinary malloc is also used if space allocated to mmap() is
** exhausted.
**
** Enable this memory allocation by compiling with -DSQLITE_MMAP_HEAP_SIZE=nnn
** where nnn is the maximum number of bytes of mmap-ed memory you want 
** to support.   This module may choose to use less memory than requested.
**
*/
#ifdef SQLITE_MMAP_HEAP_SIZE

/*
** This is a test version of the memory allocator that attempts to
** use mmap() and madvise() for allocations and frees of approximately
** the virtual memory page size.
*/
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>


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
  ** Current allocation and high-water mark.
  */
  sqlite3_int64 nowUsed;
  sqlite3_int64 mxUsed;

  /*
  ** Current allocation and high-water marks for mmap allocated memory.
  */
  sqlite3_int64 nowUsedMMap;
  sqlite3_int64 mxUsedMMap;

  /*
  ** Size of a single mmap page.  Obtained from sysconf().
  */
  int szPage;
  int mnPage;

  /*
  ** The number of available mmap pages.
  */
  int nPage;

  /*
  ** Index of the first free page.  0 means no pages have been freed.
  */
  int firstFree;

  /* First unused page on the top of the heap.
  */
  int firstUnused;

  /*
  ** Bulk memory obtained from from mmap().
  */
  char *mmapHeap;   /* first byte of the heap */ 

} mem;


/*
** Enter the mutex mem.mutex. Allocate it if it is not already allocated.
** The mmap() region is initialized the first time this routine is called.
*/
static void memsys4Enter(void){
  if( mem.mutex==0 ){
    mem.mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MEM);
  }
  sqlite3_mutex_enter(mem.mutex);
}

/*
** Attempt to free memory to the mmap heap.  This only works if
** the pointer p is within the range of memory addresses that
** comprise the mmap heap.  Return 1 if the memory was freed
** successfully.  Return 0 if the pointer is out of range.
*/
static int mmapFree(void *p){
  char *z;
  int idx, *a;
  if( mem.mmapHeap==MAP_FAILED || mem.nPage==0 ){
    return 0;
  }
  z = (char*)p;
  idx = (z - mem.mmapHeap)/mem.szPage;
  if( idx<1 || idx>=mem.nPage ){
    return 0;
  }
  a = (int*)mem.mmapHeap;
  a[idx] = a[mem.firstFree];
  mem.firstFree = idx;
  mem.nowUsedMMap -= mem.szPage;
  madvise(p, mem.szPage, MADV_DONTNEED);
  return 1;
}

/*
** Attempt to allocate nBytes from the mmap heap.  Return a pointer
** to the allocated page.  Or, return NULL if the allocation fails.
** 
** The allocation will fail if nBytes is not the right size.
** Or, the allocation will fail if the mmap heap has been exhausted.
*/
static void *mmapAlloc(int nBytes){
  int idx = 0;
  if( nBytes>mem.szPage || nBytes<mem.mnPage ){
    return 0;
  }
  if( mem.nPage==0 ){
    mem.szPage = sysconf(_SC_PAGE_SIZE);
    mem.mnPage = mem.szPage - mem.szPage/10;
    mem.nPage = SQLITE_MMAP_HEAP_SIZE/mem.szPage;
    if( mem.nPage * sizeof(int) > mem.szPage ){
      mem.nPage = mem.szPage/sizeof(int);
    }
    mem.mmapHeap =  mmap(0, mem.szPage*mem.nPage, PROT_WRITE|PROT_READ,
                         MAP_ANONYMOUS|MAP_SHARED, -1, 0);
    if( mem.mmapHeap==MAP_FAILED ){
      mem.firstUnused = errno;
    }else{
      mem.firstUnused = 1;
      mem.nowUsedMMap = mem.szPage;
    }
  }
  if( mem.mmapHeap==MAP_FAILED ){
    return 0;
  }
  if( mem.firstFree ){
    int idx = mem.firstFree;
    int *a = (int*)mem.mmapHeap;
    mem.firstFree = a[idx];
  }else if( mem.firstUnused<mem.nPage ){
    idx = mem.firstUnused++;
  }
  if( idx ){
    mem.nowUsedMMap += mem.szPage;
    if( mem.nowUsedMMap>mem.mxUsedMMap ){
      mem.mxUsedMMap = mem.nowUsedMMap;
    }
    return (void*)&mem.mmapHeap[idx*mem.szPage];
  }else{
    return 0;
  }
}

/*
** Release the mmap-ed memory region if it is currently allocated and
** is not in use.
*/
static void mmapUnmap(void){
  if( mem.mmapHeap==MAP_FAILED ) return;
  if( mem.nPage==0 ) return;
  if( mem.nowUsedMMap>mem.szPage ) return;
  munmap(mem.mmapHeap, mem.nPage*mem.szPage);
  mem.nowUsedMMap = 0;
  mem.nPage = 0;
}
    

/*
** Return the amount of memory currently checked out.
*/
sqlite3_int64 sqlite3_memory_used(void){
  sqlite3_int64 n;
  memsys4Enter();
  n = mem.nowUsed + mem.nowUsedMMap;
  sqlite3_mutex_leave(mem.mutex);  
  return n;
}

/*
** Return the maximum amount of memory that has ever been
** checked out since either the beginning of this process
** or since the most recent reset.
*/
sqlite3_int64 sqlite3_memory_highwater(int resetFlag){
  sqlite3_int64 n;
  memsys4Enter();
  n = mem.mxUsed + mem.mxUsedMMap;
  if( resetFlag ){
    mem.mxUsed = mem.nowUsed;
    mem.mxUsedMMap = mem.nowUsedMMap;
  }
  sqlite3_mutex_leave(mem.mutex);  
  return n;
}

/*
** Change the alarm callback
*/
int sqlite3_memory_alarm(
  void(*xCallback)(void *pArg, sqlite3_int64 used,int N),
  void *pArg,
  sqlite3_int64 iThreshold
){
  memsys4Enter();
  mem.alarmCallback = xCallback;
  mem.alarmArg = pArg;
  mem.alarmThreshold = iThreshold;
  sqlite3_mutex_leave(mem.mutex);
  return SQLITE_OK;
}

/*
** Trigger the alarm 
*/
static void sqlite3MemsysAlarm(int nByte){
  void (*xCallback)(void*,sqlite3_int64,int);
  sqlite3_int64 nowUsed;
  void *pArg;
  if( mem.alarmCallback==0 || mem.alarmBusy  ) return;
  mem.alarmBusy = 1;
  xCallback = mem.alarmCallback;
  nowUsed = mem.nowUsed;
  pArg = mem.alarmArg;
  sqlite3_mutex_leave(mem.mutex);
  xCallback(pArg, nowUsed, nByte);
  sqlite3_mutex_enter(mem.mutex);
  mem.alarmBusy = 0;
}

/*
** Allocate nBytes of memory
*/
static void *memsys4Malloc(int nBytes){
  sqlite3_int64 *p = 0;
  if( mem.alarmCallback!=0
         && mem.nowUsed+mem.nowUsedMMap+nBytes>=mem.alarmThreshold ){
    sqlite3MemsysAlarm(nBytes);
  }
  if( (p = mmapAlloc(nBytes))==0 ){
    p = malloc(nBytes+8);
    if( p==0 ){
      sqlite3MemsysAlarm(nBytes);
      p = malloc(nBytes+8);
    }
    if( p ){
      p[0] = nBytes;
      p++;
      mem.nowUsed += nBytes;
      if( mem.nowUsed>mem.mxUsed ){
        mem.mxUsed = mem.nowUsed;
      }
    }
  }
  return (void*)p; 
}

/*
** Return the size of a memory allocation
*/
static int memsys4Size(void *pPrior){
  char *z = (char*)pPrior;
  int idx = mem.nPage ? (z - mem.mmapHeap)/mem.szPage : 0;
  int nByte;
  if( idx>=1 && idx<mem.nPage ){
    nByte = mem.szPage;
  }else{
    sqlite3_int64 *p = pPrior;
    p--;
    nByte = (int)*p;
  }
  return nByte;
}

/*
** Free memory.
*/
static void memsys4Free(void *pPrior){
  sqlite3_int64 *p;
  int nByte;
  if( mmapFree(pPrior)==0 ){
    p = pPrior;
    p--;
    nByte = (int)*p;
    mem.nowUsed -= nByte;
    free(p);
    if( mem.nowUsed==0 ){
      mmapUnmap();
    }      
  }
}

/*
** Allocate nBytes of memory
*/
void *sqlite3_malloc(int nBytes){
  sqlite3_int64 *p = 0;
  if( nBytes>0 ){
    memsys4Enter();
    p = memsys4Malloc(nBytes);
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
  memsys4Free(pPrior);
  sqlite3_mutex_leave(mem.mutex);  
}



/*
** Change the size of an existing memory allocation
*/
void *sqlite3_realloc(void *pPrior, int nBytes){
  int nOld;
  sqlite3_int64 *p;
  if( pPrior==0 ){
    return sqlite3_malloc(nBytes);
  }
  if( nBytes<=0 ){
    sqlite3_free(pPrior);
    return 0;
  }
  nOld = memsys4Size(pPrior);
  if( nBytes<=nOld && nBytes>=nOld-128 ){
    return pPrior;
  }
  assert( mem.mutex!=0 );
  sqlite3_mutex_enter(mem.mutex);
  p = memsys4Malloc(nBytes);
  if( p ){
    if( nOld<nBytes ){
      memcpy(p, pPrior, nOld);
    }else{
      memcpy(p, pPrior, nBytes);
    }
    memsys4Free(pPrior);
  }
  assert( mem.mutex!=0 );
  sqlite3_mutex_leave(mem.mutex);
  return (void*)p;
}

#endif /* SQLITE_MMAP_HEAP_SIZE */
