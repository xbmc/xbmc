/*
** 2007 August 15
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
** $Id: mem2.c,v 1.26 2008/04/10 14:57:25 drh Exp $
*/
#include "sqliteInt.h"

/*
** This version of the memory allocator is used only if the
** SQLITE_MEMDEBUG macro is defined
*/
#ifdef SQLITE_MEMDEBUG

/*
** The backtrace functionality is only available with GLIBC
*/
#ifdef __GLIBC__
  extern int backtrace(void**,int);
  extern void backtrace_symbols_fd(void*const*,int,int);
#else
# define backtrace(A,B) 0
# define backtrace_symbols_fd(A,B,C)
#endif
#include <stdio.h>

/*
** Each memory allocation looks like this:
**
**  ------------------------------------------------------------------------
**  | Title |  backtrace pointers |  MemBlockHdr |  allocation |  EndGuard |
**  ------------------------------------------------------------------------
**
** The application code sees only a pointer to the allocation.  We have
** to back up from the allocation pointer to find the MemBlockHdr.  The
** MemBlockHdr tells us the size of the allocation and the number of
** backtrace pointers.  There is also a guard word at the end of the
** MemBlockHdr.
*/
struct MemBlockHdr {
  i64 iSize;                          /* Size of this allocation */
  struct MemBlockHdr *pNext, *pPrev;  /* Linked list of all unfreed memory */
  char nBacktrace;                    /* Number of backtraces on this alloc */
  char nBacktraceSlots;               /* Available backtrace slots */
  short nTitle;                       /* Bytes of title; includes '\0' */
  int iForeGuard;                     /* Guard word for sanity */
};

/*
** Guard words
*/
#define FOREGUARD 0x80F5E153
#define REARGUARD 0xE4676B53

/*
** Number of malloc size increments to track.
*/
#define NCSIZE  1000

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
  void (*alarmCallback)(void*, sqlite3_int64, int);
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
  ** Head and tail of a linked list of all outstanding allocations
  */
  struct MemBlockHdr *pFirst;
  struct MemBlockHdr *pLast;
  
  /*
  ** The number of levels of backtrace to save in new allocations.
  */
  int nBacktrace;
  void (*xBacktrace)(int, int, void **);

  /*
  ** Title text to insert in front of each block
  */
  int nTitle;        /* Bytes of zTitle to save.  Includes '\0' and padding */
  char zTitle[100];  /* The title text */

  /* 
  ** sqlite3MallocDisallow() increments the following counter.
  ** sqlite3MallocAllow() decrements it.
  */
  int disallow; /* Do not allow memory allocation */

  /*
  ** Gather statistics on the sizes of memory allocations.
  ** sizeCnt[i] is the number of allocation attempts of i*8
  ** bytes.  i==NCSIZE is the number of allocation attempts for
  ** sizes more than NCSIZE*8 bytes.
  */
  int sizeCnt[NCSIZE];

} mem;


/*
** Enter the mutex mem.mutex. Allocate it if it is not already allocated.
*/
static void enterMem(void){
  if( mem.mutex==0 ){
    mem.mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MEM);
  }
  sqlite3_mutex_enter(mem.mutex);
}

/*
** Return the amount of memory currently checked out.
*/
sqlite3_int64 sqlite3_memory_used(void){
  sqlite3_int64 n;
  enterMem();
  n = mem.nowUsed;
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
  enterMem();
  n = mem.mxUsed;
  if( resetFlag ){
    mem.mxUsed = mem.nowUsed;
  }
  sqlite3_mutex_leave(mem.mutex);  
  return n;
}

/*
** Change the alarm callback
*/
int sqlite3_memory_alarm(
  void(*xCallback)(void *pArg, sqlite3_int64 used, int N),
  void *pArg,
  sqlite3_int64 iThreshold
){
  enterMem();
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
** Given an allocation, find the MemBlockHdr for that allocation.
**
** This routine checks the guards at either end of the allocation and
** if they are incorrect it asserts.
*/
static struct MemBlockHdr *sqlite3MemsysGetHeader(void *pAllocation){
  struct MemBlockHdr *p;
  int *pInt;
  u8 *pU8;
  int nReserve;

  p = (struct MemBlockHdr*)pAllocation;
  p--;
  assert( p->iForeGuard==FOREGUARD );
  nReserve = (p->iSize+7)&~7;
  pInt = (int*)pAllocation;
  pU8 = (u8*)pAllocation;
  assert( pInt[nReserve/sizeof(int)]==REARGUARD );
  assert( (nReserve-0)<=p->iSize || pU8[nReserve-1]==0x65 );
  assert( (nReserve-1)<=p->iSize || pU8[nReserve-2]==0x65 );
  assert( (nReserve-2)<=p->iSize || pU8[nReserve-3]==0x65 );
  return p;
}

/*
** Return the number of bytes currently allocated at address p.
*/
int sqlite3MallocSize(void *p){
  struct MemBlockHdr *pHdr;
  if( !p ){
    return 0;
  }
  pHdr = sqlite3MemsysGetHeader(p);
  return pHdr->iSize;
}

/*
** Allocate nByte bytes of memory.
*/
void *sqlite3_malloc(int nByte){
  struct MemBlockHdr *pHdr;
  void **pBt;
  char *z;
  int *pInt;
  void *p = 0;
  int totalSize;

  if( nByte>0 ){
    int nReserve;
    enterMem();
    assert( mem.disallow==0 );
    if( mem.alarmCallback!=0 && mem.nowUsed+nByte>=mem.alarmThreshold ){
      sqlite3MemsysAlarm(nByte);
    }
    nReserve = (nByte+7)&~7;
    if( nReserve/8>NCSIZE-1 ){
      mem.sizeCnt[NCSIZE-1]++;
    }else{
      mem.sizeCnt[nReserve/8]++;
    }
    totalSize = nReserve + sizeof(*pHdr) + sizeof(int) +
                 mem.nBacktrace*sizeof(void*) + mem.nTitle;
    if( sqlite3FaultStep(SQLITE_FAULTINJECTOR_MALLOC) ){
      p = 0;
    }else{
      p = malloc(totalSize);
      if( p==0 ){
        sqlite3MemsysAlarm(nByte);
        p = malloc(totalSize);
      }
    }
    if( p ){
      z = p;
      pBt = (void**)&z[mem.nTitle];
      pHdr = (struct MemBlockHdr*)&pBt[mem.nBacktrace];
      pHdr->pNext = 0;
      pHdr->pPrev = mem.pLast;
      if( mem.pLast ){
        mem.pLast->pNext = pHdr;
      }else{
        mem.pFirst = pHdr;
      }
      mem.pLast = pHdr;
      pHdr->iForeGuard = FOREGUARD;
      pHdr->nBacktraceSlots = mem.nBacktrace;
      pHdr->nTitle = mem.nTitle;
      if( mem.nBacktrace ){
        void *aAddr[40];
        pHdr->nBacktrace = backtrace(aAddr, mem.nBacktrace+1)-1;
        memcpy(pBt, &aAddr[1], pHdr->nBacktrace*sizeof(void*));
	if( mem.xBacktrace ){
          mem.xBacktrace(nByte, pHdr->nBacktrace-1, &aAddr[1]);
	}
      }else{
        pHdr->nBacktrace = 0;
      }
      if( mem.nTitle ){
        memcpy(z, mem.zTitle, mem.nTitle);
      }
      pHdr->iSize = nByte;
      pInt = (int*)&pHdr[1];
      pInt[nReserve/sizeof(int)] = REARGUARD;
      memset(pInt, 0x65, nReserve);
      mem.nowUsed += nByte;
      if( mem.nowUsed>mem.mxUsed ){
        mem.mxUsed = mem.nowUsed;
      }
      p = (void*)pInt;
    }
    sqlite3_mutex_leave(mem.mutex);
  }
  return p; 
}

/*
** Free memory.
*/
void sqlite3_free(void *pPrior){
  struct MemBlockHdr *pHdr;
  void **pBt;
  char *z;
  if( pPrior==0 ){
    return;
  }
  assert( mem.mutex!=0 );
  pHdr = sqlite3MemsysGetHeader(pPrior);
  pBt = (void**)pHdr;
  pBt -= pHdr->nBacktraceSlots;
  sqlite3_mutex_enter(mem.mutex);
  mem.nowUsed -= pHdr->iSize;
  if( pHdr->pPrev ){
    assert( pHdr->pPrev->pNext==pHdr );
    pHdr->pPrev->pNext = pHdr->pNext;
  }else{
    assert( mem.pFirst==pHdr );
    mem.pFirst = pHdr->pNext;
  }
  if( pHdr->pNext ){
    assert( pHdr->pNext->pPrev==pHdr );
    pHdr->pNext->pPrev = pHdr->pPrev;
  }else{
    assert( mem.pLast==pHdr );
    mem.pLast = pHdr->pPrev;
  }
  z = (char*)pBt;
  z -= pHdr->nTitle;
  memset(z, 0x2b, sizeof(void*)*pHdr->nBacktraceSlots + sizeof(*pHdr) +
                  pHdr->iSize + sizeof(int) + pHdr->nTitle);
  free(z);
  sqlite3_mutex_leave(mem.mutex);  
}

/*
** Change the size of an existing memory allocation.
**
** For this debugging implementation, we *always* make a copy of the
** allocation into a new place in memory.  In this way, if the 
** higher level code is using pointer to the old allocation, it is 
** much more likely to break and we are much more liking to find
** the error.
*/
void *sqlite3_realloc(void *pPrior, int nByte){
  struct MemBlockHdr *pOldHdr;
  void *pNew;
  if( pPrior==0 ){
    return sqlite3_malloc(nByte);
  }
  if( nByte<=0 ){
    sqlite3_free(pPrior);
    return 0;
  }
  assert( mem.disallow==0 );
  pOldHdr = sqlite3MemsysGetHeader(pPrior);
  pNew = sqlite3_malloc(nByte);
  if( pNew ){
    memcpy(pNew, pPrior, nByte<pOldHdr->iSize ? nByte : pOldHdr->iSize);
    if( nByte>pOldHdr->iSize ){
      memset(&((char*)pNew)[pOldHdr->iSize], 0x2b, nByte - pOldHdr->iSize);
    }
    sqlite3_free(pPrior);
  }
  return pNew;
}

/*
** Set the number of backtrace levels kept for each allocation.
** A value of zero turns of backtracing.  The number is always rounded
** up to a multiple of 2.
*/
void sqlite3MemdebugBacktrace(int depth){
  if( depth<0 ){ depth = 0; }
  if( depth>20 ){ depth = 20; }
  depth = (depth+1)&0xfe;
  mem.nBacktrace = depth;
}

void sqlite3MemdebugBacktraceCallback(void (*xBacktrace)(int, int, void **)){
  mem.xBacktrace = xBacktrace;
}

/*
** Set the title string for subsequent allocations.
*/
void sqlite3MemdebugSettitle(const char *zTitle){
  int n = strlen(zTitle) + 1;
  enterMem();
  if( n>=sizeof(mem.zTitle) ) n = sizeof(mem.zTitle)-1;
  memcpy(mem.zTitle, zTitle, n);
  mem.zTitle[n] = 0;
  mem.nTitle = (n+7)&~7;
  sqlite3_mutex_leave(mem.mutex);
}

void sqlite3MemdebugSync(){
  struct MemBlockHdr *pHdr;
  for(pHdr=mem.pFirst; pHdr; pHdr=pHdr->pNext){
    void **pBt = (void**)pHdr;
    pBt -= pHdr->nBacktraceSlots;
    mem.xBacktrace(pHdr->iSize, pHdr->nBacktrace-1, &pBt[1]);
  }
}

/*
** Open the file indicated and write a log of all unfreed memory 
** allocations into that log.
*/
void sqlite3MemdebugDump(const char *zFilename){
  FILE *out;
  struct MemBlockHdr *pHdr;
  void **pBt;
  int i;
  out = fopen(zFilename, "w");
  if( out==0 ){
    fprintf(stderr, "** Unable to output memory debug output log: %s **\n",
                    zFilename);
    return;
  }
  for(pHdr=mem.pFirst; pHdr; pHdr=pHdr->pNext){
    char *z = (char*)pHdr;
    z -= pHdr->nBacktraceSlots*sizeof(void*) + pHdr->nTitle;
    fprintf(out, "**** %lld bytes at %p from %s ****\n", 
            pHdr->iSize, &pHdr[1], pHdr->nTitle ? z : "???");
    if( pHdr->nBacktrace ){
      fflush(out);
      pBt = (void**)pHdr;
      pBt -= pHdr->nBacktraceSlots;
      backtrace_symbols_fd(pBt, pHdr->nBacktrace, fileno(out));
      fprintf(out, "\n");
    }
  }
  fprintf(out, "COUNTS:\n");
  for(i=0; i<NCSIZE-1; i++){
    if( mem.sizeCnt[i] ){
      fprintf(out, "   %3d: %d\n", i*8+8, mem.sizeCnt[i]);
    }
  }
  if( mem.sizeCnt[NCSIZE-1] ){
    fprintf(out, "  >%3d: %d\n", NCSIZE*8, mem.sizeCnt[NCSIZE-1]);
  }
  fclose(out);
}

/*
** Return the number of times sqlite3_malloc() has been called.
*/
int sqlite3MemdebugMallocCount(){
  int i;
  int nTotal = 0;
  for(i=0; i<NCSIZE; i++){
    nTotal += mem.sizeCnt[i];
  }
  return nTotal;
}


#endif /* SQLITE_MEMDEBUG */
