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
** fixed memory pool is SQLITE_MEMORY_SIZE bytes.
**
** This version of the memory allocation subsystem is used if
** and only if SQLITE_MEMORY_SIZE is defined.
**
** $Id: mem3.c,v 1.12 2008/02/19 15:15:16 drh Exp $
*/
#include "sqliteInt.h"

/*
** This version of the memory allocator is used only when 
** SQLITE_MEMORY_SIZE is defined.
*/
#ifdef SQLITE_MEMORY_SIZE

/*
** Maximum size (in Mem3Blocks) of a "small" chunk.
*/
#define MX_SMALL 10


/*
** Number of freelist hash slots
*/
#define N_HASH  61

/*
** A memory allocation (also called a "chunk") consists of two or 
** more blocks where each block is 8 bytes.  The first 8 bytes are 
** a header that is not returned to the user.
**
** A chunk is two or more blocks that is either checked out or
** free.  The first block has format u.hdr.  u.hdr.size4x is 4 times the
** size of the allocation in blocks if the allocation is free.
** The u.hdr.size4x&1 bit is true if the chunk is checked out and
** false if the chunk is on the freelist.  The u.hdr.size4x&2 bit
** is true if the previous chunk is checked out and false if the
** previous chunk is free.  The u.hdr.prevSize field is the size of
** the previous chunk in blocks if the previous chunk is on the
** freelist. If the previous chunk is checked out, then
** u.hdr.prevSize can be part of the data for that chunk and should
** not be read or written.
**
** We often identify a chunk by its index in mem.aPool[].  When
** this is done, the chunk index refers to the second block of
** the chunk.  In this way, the first chunk has an index of 1.
** A chunk index of 0 means "no such chunk" and is the equivalent
** of a NULL pointer.
**
** The second block of free chunks is of the form u.list.  The
** two fields form a double-linked list of chunks of related sizes.
** Pointers to the head of the list are stored in mem.aiSmall[] 
** for smaller chunks and mem.aiHash[] for larger chunks.
**
** The second block of a chunk is user data if the chunk is checked 
** out.  If a chunk is checked out, the user data may extend into
** the u.hdr.prevSize value of the following chunk.
*/
typedef struct Mem3Block Mem3Block;
struct Mem3Block {
  union {
    struct {
      u32 prevSize;   /* Size of previous chunk in Mem3Block elements */
      u32 size4x;     /* 4x the size of current chunk in Mem3Block elements */
    } hdr;
    struct {
      u32 next;       /* Index in mem.aPool[] of next free chunk */
      u32 prev;       /* Index in mem.aPool[] of previous free chunk */
    } list;
  } u;
};

/*
** All of the static variables used by this module are collected
** into a single structure named "mem".  This is to keep the
** static variables organized and to reduce namespace pollution
** when this module is combined with other in the amalgamation.
*/
static struct {
  /*
  ** True if we are evaluating an out-of-memory callback.
  */
  int alarmBusy;
  
  /*
  ** Mutex to control access to the memory allocation subsystem.
  */
  sqlite3_mutex *mutex;
  
  /*
  ** The minimum amount of free space that we have seen.
  */
  u32 mnMaster;

  /*
  ** iMaster is the index of the master chunk.  Most new allocations
  ** occur off of this chunk.  szMaster is the size (in Mem3Blocks)
  ** of the current master.  iMaster is 0 if there is not master chunk.
  ** The master chunk is not in either the aiHash[] or aiSmall[].
  */
  u32 iMaster;
  u32 szMaster;

  /*
  ** Array of lists of free blocks according to the block size 
  ** for smaller chunks, or a hash on the block size for larger
  ** chunks.
  */
  u32 aiSmall[MX_SMALL-1];   /* For sizes 2 through MX_SMALL, inclusive */
  u32 aiHash[N_HASH];        /* For sizes MX_SMALL+1 and larger */

  /*
  ** Memory available for allocation
  */
  Mem3Block aPool[SQLITE_MEMORY_SIZE/sizeof(Mem3Block)+2];
} mem;

/*
** Unlink the chunk at mem.aPool[i] from list it is currently
** on.  *pRoot is the list that i is a member of.
*/
static void memsys3UnlinkFromList(u32 i, u32 *pRoot){
  u32 next = mem.aPool[i].u.list.next;
  u32 prev = mem.aPool[i].u.list.prev;
  assert( sqlite3_mutex_held(mem.mutex) );
  if( prev==0 ){
    *pRoot = next;
  }else{
    mem.aPool[prev].u.list.next = next;
  }
  if( next ){
    mem.aPool[next].u.list.prev = prev;
  }
  mem.aPool[i].u.list.next = 0;
  mem.aPool[i].u.list.prev = 0;
}

/*
** Unlink the chunk at index i from 
** whatever list is currently a member of.
*/
static void memsys3Unlink(u32 i){
  u32 size, hash;
  assert( sqlite3_mutex_held(mem.mutex) );
  assert( (mem.aPool[i-1].u.hdr.size4x & 1)==0 );
  assert( i>=1 );
  size = mem.aPool[i-1].u.hdr.size4x/4;
  assert( size==mem.aPool[i+size-1].u.hdr.prevSize );
  assert( size>=2 );
  if( size <= MX_SMALL ){
    memsys3UnlinkFromList(i, &mem.aiSmall[size-2]);
  }else{
    hash = size % N_HASH;
    memsys3UnlinkFromList(i, &mem.aiHash[hash]);
  }
}

/*
** Link the chunk at mem.aPool[i] so that is on the list rooted
** at *pRoot.
*/
static void memsys3LinkIntoList(u32 i, u32 *pRoot){
  assert( sqlite3_mutex_held(mem.mutex) );
  mem.aPool[i].u.list.next = *pRoot;
  mem.aPool[i].u.list.prev = 0;
  if( *pRoot ){
    mem.aPool[*pRoot].u.list.prev = i;
  }
  *pRoot = i;
}

/*
** Link the chunk at index i into either the appropriate
** small chunk list, or into the large chunk hash table.
*/
static void memsys3Link(u32 i){
  u32 size, hash;
  assert( sqlite3_mutex_held(mem.mutex) );
  assert( i>=1 );
  assert( (mem.aPool[i-1].u.hdr.size4x & 1)==0 );
  size = mem.aPool[i-1].u.hdr.size4x/4;
  assert( size==mem.aPool[i+size-1].u.hdr.prevSize );
  assert( size>=2 );
  if( size <= MX_SMALL ){
    memsys3LinkIntoList(i, &mem.aiSmall[size-2]);
  }else{
    hash = size % N_HASH;
    memsys3LinkIntoList(i, &mem.aiHash[hash]);
  }
}

/*
** Enter the mutex mem.mutex. Allocate it if it is not already allocated.
**
** Also:  Initialize the memory allocation subsystem the first time
** this routine is called.
*/
static void memsys3Enter(void){
  if( mem.mutex==0 ){
    mem.mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MEM);
    mem.aPool[0].u.hdr.size4x = SQLITE_MEMORY_SIZE/2 + 2;
    mem.aPool[SQLITE_MEMORY_SIZE/8].u.hdr.prevSize = SQLITE_MEMORY_SIZE/8;
    mem.aPool[SQLITE_MEMORY_SIZE/8].u.hdr.size4x = 1;
    mem.iMaster = 1;
    mem.szMaster = SQLITE_MEMORY_SIZE/8;
    mem.mnMaster = mem.szMaster;
  }
  sqlite3_mutex_enter(mem.mutex);
}

/*
** Return the amount of memory currently checked out.
*/
sqlite3_int64 sqlite3_memory_used(void){
  sqlite3_int64 n;
  memsys3Enter();
  n = SQLITE_MEMORY_SIZE - mem.szMaster*8;
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
  memsys3Enter();
  n = SQLITE_MEMORY_SIZE - mem.mnMaster*8;
  if( resetFlag ){
    mem.mnMaster = mem.szMaster;
  }
  sqlite3_mutex_leave(mem.mutex);  
  return n;
}

/*
** Change the alarm callback.
**
** This is a no-op for the static memory allocator.  The purpose
** of the memory alarm is to support sqlite3_soft_heap_limit().
** But with this memory allocator, the soft_heap_limit is really
** a hard limit that is fixed at SQLITE_MEMORY_SIZE.
*/
int sqlite3_memory_alarm(
  void(*xCallback)(void *pArg, sqlite3_int64 used,int N),
  void *pArg,
  sqlite3_int64 iThreshold
){
  return SQLITE_OK;
}

/*
** Called when we are unable to satisfy an allocation of nBytes.
*/
static void memsys3OutOfMemory(int nByte){
  if( !mem.alarmBusy ){
    mem.alarmBusy = 1;
    assert( sqlite3_mutex_held(mem.mutex) );
    sqlite3_mutex_leave(mem.mutex);
    sqlite3_release_memory(nByte);
    sqlite3_mutex_enter(mem.mutex);
    mem.alarmBusy = 0;
  }
}

/*
** Return the size of an outstanding allocation, in bytes.  The
** size returned omits the 8-byte header overhead.  This only
** works for chunks that are currently checked out.
*/
int sqlite3MallocSize(void *p){
  int iSize = 0;
  if( p ){
    Mem3Block *pBlock = (Mem3Block*)p;
    assert( (pBlock[-1].u.hdr.size4x&1)!=0 );
    iSize = (pBlock[-1].u.hdr.size4x&~3)*2 - 4;
  }
  return iSize;
}

/*
** Chunk i is a free chunk that has been unlinked.  Adjust its 
** size parameters for check-out and return a pointer to the 
** user portion of the chunk.
*/
static void *memsys3Checkout(u32 i, int nBlock){
  u32 x;
  assert( sqlite3_mutex_held(mem.mutex) );
  assert( i>=1 );
  assert( mem.aPool[i-1].u.hdr.size4x/4==nBlock );
  assert( mem.aPool[i+nBlock-1].u.hdr.prevSize==nBlock );
  x = mem.aPool[i-1].u.hdr.size4x;
  mem.aPool[i-1].u.hdr.size4x = nBlock*4 | 1 | (x&2);
  mem.aPool[i+nBlock-1].u.hdr.prevSize = nBlock;
  mem.aPool[i+nBlock-1].u.hdr.size4x |= 2;
  return &mem.aPool[i];
}

/*
** Carve a piece off of the end of the mem.iMaster free chunk.
** Return a pointer to the new allocation.  Or, if the master chunk
** is not large enough, return 0.
*/
static void *memsys3FromMaster(int nBlock){
  assert( sqlite3_mutex_held(mem.mutex) );
  assert( mem.szMaster>=nBlock );
  if( nBlock>=mem.szMaster-1 ){
    /* Use the entire master */
    void *p = memsys3Checkout(mem.iMaster, mem.szMaster);
    mem.iMaster = 0;
    mem.szMaster = 0;
    mem.mnMaster = 0;
    return p;
  }else{
    /* Split the master block.  Return the tail. */
    u32 newi, x;
    newi = mem.iMaster + mem.szMaster - nBlock;
    assert( newi > mem.iMaster+1 );
    mem.aPool[mem.iMaster+mem.szMaster-1].u.hdr.prevSize = nBlock;
    mem.aPool[mem.iMaster+mem.szMaster-1].u.hdr.size4x |= 2;
    mem.aPool[newi-1].u.hdr.size4x = nBlock*4 + 1;
    mem.szMaster -= nBlock;
    mem.aPool[newi-1].u.hdr.prevSize = mem.szMaster;
    x = mem.aPool[mem.iMaster-1].u.hdr.size4x & 2;
    mem.aPool[mem.iMaster-1].u.hdr.size4x = mem.szMaster*4 | x;
    if( mem.szMaster < mem.mnMaster ){
      mem.mnMaster = mem.szMaster;
    }
    return (void*)&mem.aPool[newi];
  }
}

/*
** *pRoot is the head of a list of free chunks of the same size
** or same size hash.  In other words, *pRoot is an entry in either
** mem.aiSmall[] or mem.aiHash[].  
**
** This routine examines all entries on the given list and tries
** to coalesce each entries with adjacent free chunks.  
**
** If it sees a chunk that is larger than mem.iMaster, it replaces 
** the current mem.iMaster with the new larger chunk.  In order for
** this mem.iMaster replacement to work, the master chunk must be
** linked into the hash tables.  That is not the normal state of
** affairs, of course.  The calling routine must link the master
** chunk before invoking this routine, then must unlink the (possibly
** changed) master chunk once this routine has finished.
*/
static void memsys3Merge(u32 *pRoot){
  u32 iNext, prev, size, i, x;

  assert( sqlite3_mutex_held(mem.mutex) );
  for(i=*pRoot; i>0; i=iNext){
    iNext = mem.aPool[i].u.list.next;
    size = mem.aPool[i-1].u.hdr.size4x;
    assert( (size&1)==0 );
    if( (size&2)==0 ){
      memsys3UnlinkFromList(i, pRoot);
      assert( i > mem.aPool[i-1].u.hdr.prevSize );
      prev = i - mem.aPool[i-1].u.hdr.prevSize;
      if( prev==iNext ){
        iNext = mem.aPool[prev].u.list.next;
      }
      memsys3Unlink(prev);
      size = i + size/4 - prev;
      x = mem.aPool[prev-1].u.hdr.size4x & 2;
      mem.aPool[prev-1].u.hdr.size4x = size*4 | x;
      mem.aPool[prev+size-1].u.hdr.prevSize = size;
      memsys3Link(prev);
      i = prev;
    }else{
      size /= 4;
    }
    if( size>mem.szMaster ){
      mem.iMaster = i;
      mem.szMaster = size;
    }
  }
}

/*
** Return a block of memory of at least nBytes in size.
** Return NULL if unable.
*/
static void *memsys3Malloc(int nByte){
  u32 i;
  int nBlock;
  int toFree;

  assert( sqlite3_mutex_held(mem.mutex) );
  assert( sizeof(Mem3Block)==8 );
  if( nByte<=12 ){
    nBlock = 2;
  }else{
    nBlock = (nByte + 11)/8;
  }
  assert( nBlock >= 2 );

  /* STEP 1:
  ** Look for an entry of the correct size in either the small
  ** chunk table or in the large chunk hash table.  This is
  ** successful most of the time (about 9 times out of 10).
  */
  if( nBlock <= MX_SMALL ){
    i = mem.aiSmall[nBlock-2];
    if( i>0 ){
      memsys3UnlinkFromList(i, &mem.aiSmall[nBlock-2]);
      return memsys3Checkout(i, nBlock);
    }
  }else{
    int hash = nBlock % N_HASH;
    for(i=mem.aiHash[hash]; i>0; i=mem.aPool[i].u.list.next){
      if( mem.aPool[i-1].u.hdr.size4x/4==nBlock ){
        memsys3UnlinkFromList(i, &mem.aiHash[hash]);
        return memsys3Checkout(i, nBlock);
      }
    }
  }

  /* STEP 2:
  ** Try to satisfy the allocation by carving a piece off of the end
  ** of the master chunk.  This step usually works if step 1 fails.
  */
  if( mem.szMaster>=nBlock ){
    return memsys3FromMaster(nBlock);
  }


  /* STEP 3:  
  ** Loop through the entire memory pool.  Coalesce adjacent free
  ** chunks.  Recompute the master chunk as the largest free chunk.
  ** Then try again to satisfy the allocation by carving a piece off
  ** of the end of the master chunk.  This step happens very
  ** rarely (we hope!)
  */
  for(toFree=nBlock*16; toFree<SQLITE_MEMORY_SIZE*2; toFree *= 2){
    memsys3OutOfMemory(toFree);
    if( mem.iMaster ){
      memsys3Link(mem.iMaster);
      mem.iMaster = 0;
      mem.szMaster = 0;
    }
    for(i=0; i<N_HASH; i++){
      memsys3Merge(&mem.aiHash[i]);
    }
    for(i=0; i<MX_SMALL-1; i++){
      memsys3Merge(&mem.aiSmall[i]);
    }
    if( mem.szMaster ){
      memsys3Unlink(mem.iMaster);
      if( mem.szMaster>=nBlock ){
        return memsys3FromMaster(nBlock);
      }
    }
  }

  /* If none of the above worked, then we fail. */
  return 0;
}

/*
** Free an outstanding memory allocation.
*/
void memsys3Free(void *pOld){
  Mem3Block *p = (Mem3Block*)pOld;
  int i;
  u32 size, x;
  assert( sqlite3_mutex_held(mem.mutex) );
  assert( p>mem.aPool && p<&mem.aPool[SQLITE_MEMORY_SIZE/8] );
  i = p - mem.aPool;
  assert( (mem.aPool[i-1].u.hdr.size4x&1)==1 );
  size = mem.aPool[i-1].u.hdr.size4x/4;
  assert( i+size<=SQLITE_MEMORY_SIZE/8+1 );
  mem.aPool[i-1].u.hdr.size4x &= ~1;
  mem.aPool[i+size-1].u.hdr.prevSize = size;
  mem.aPool[i+size-1].u.hdr.size4x &= ~2;
  memsys3Link(i);

  /* Try to expand the master using the newly freed chunk */
  if( mem.iMaster ){
    while( (mem.aPool[mem.iMaster-1].u.hdr.size4x&2)==0 ){
      size = mem.aPool[mem.iMaster-1].u.hdr.prevSize;
      mem.iMaster -= size;
      mem.szMaster += size;
      memsys3Unlink(mem.iMaster);
      x = mem.aPool[mem.iMaster-1].u.hdr.size4x & 2;
      mem.aPool[mem.iMaster-1].u.hdr.size4x = mem.szMaster*4 | x;
      mem.aPool[mem.iMaster+mem.szMaster-1].u.hdr.prevSize = mem.szMaster;
    }
    x = mem.aPool[mem.iMaster-1].u.hdr.size4x & 2;
    while( (mem.aPool[mem.iMaster+mem.szMaster-1].u.hdr.size4x&1)==0 ){
      memsys3Unlink(mem.iMaster+mem.szMaster);
      mem.szMaster += mem.aPool[mem.iMaster+mem.szMaster-1].u.hdr.size4x/4;
      mem.aPool[mem.iMaster-1].u.hdr.size4x = mem.szMaster*4 | x;
      mem.aPool[mem.iMaster+mem.szMaster-1].u.hdr.prevSize = mem.szMaster;
    }
  }
}

/*
** Allocate nBytes of memory
*/
void *sqlite3_malloc(int nBytes){
  sqlite3_int64 *p = 0;
  if( nBytes>0 ){
    memsys3Enter();
    p = memsys3Malloc(nBytes);
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
  memsys3Free(pPrior);
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
  if( nBytes<=nOld && nBytes>=nOld-128 ){
    return pPrior;
  }
  sqlite3_mutex_enter(mem.mutex);
  p = memsys3Malloc(nBytes);
  if( p ){
    if( nOld<nBytes ){
      memcpy(p, pPrior, nOld);
    }else{
      memcpy(p, pPrior, nBytes);
    }
    memsys3Free(pPrior);
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
  int i, j;
  u32 size;
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
  memsys3Enter();
  fprintf(out, "CHUNKS:\n");
  for(i=1; i<=SQLITE_MEMORY_SIZE/8; i+=size/4){
    size = mem.aPool[i-1].u.hdr.size4x;
    if( size/4<=1 ){
      fprintf(out, "%p size error\n", &mem.aPool[i]);
      assert( 0 );
      break;
    }
    if( (size&1)==0 && mem.aPool[i+size/4-1].u.hdr.prevSize!=size/4 ){
      fprintf(out, "%p tail size does not match\n", &mem.aPool[i]);
      assert( 0 );
      break;
    }
    if( ((mem.aPool[i+size/4-1].u.hdr.size4x&2)>>1)!=(size&1) ){
      fprintf(out, "%p tail checkout bit is incorrect\n", &mem.aPool[i]);
      assert( 0 );
      break;
    }
    if( size&1 ){
      fprintf(out, "%p %6d bytes checked out\n", &mem.aPool[i], (size/4)*8-8);
    }else{
      fprintf(out, "%p %6d bytes free%s\n", &mem.aPool[i], (size/4)*8-8,
                  i==mem.iMaster ? " **master**" : "");
    }
  }
  for(i=0; i<MX_SMALL-1; i++){
    if( mem.aiSmall[i]==0 ) continue;
    fprintf(out, "small(%2d):", i);
    for(j = mem.aiSmall[i]; j>0; j=mem.aPool[j].u.list.next){
      fprintf(out, " %p(%d)", &mem.aPool[j],
              (mem.aPool[j-1].u.hdr.size4x/4)*8-8);
    }
    fprintf(out, "\n"); 
  }
  for(i=0; i<N_HASH; i++){
    if( mem.aiHash[i]==0 ) continue;
    fprintf(out, "hash(%2d):", i);
    for(j = mem.aiHash[i]; j>0; j=mem.aPool[j].u.list.next){
      fprintf(out, " %p(%d)", &mem.aPool[j],
              (mem.aPool[j-1].u.hdr.size4x/4)*8-8);
    }
    fprintf(out, "\n"); 
  }
  fprintf(out, "master=%d\n", mem.iMaster);
  fprintf(out, "nowUsed=%d\n", SQLITE_MEMORY_SIZE - mem.szMaster*8);
  fprintf(out, "mxUsed=%d\n", SQLITE_MEMORY_SIZE - mem.mnMaster*8);
  sqlite3_mutex_leave(mem.mutex);
  if( out==stdout ){
    fflush(stdout);
  }else{
    fclose(out);
  }
#endif
}


#endif /* !SQLITE_MEMORY_SIZE */
