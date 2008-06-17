/*
** 2006 Feb 14
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
**
** This file contains code that is specific to OS/2.
*/

#include "sqliteInt.h"

#if OS_OS2

/*
** A Note About Memory Allocation:
**
** This driver uses malloc()/free() directly rather than going through
** the SQLite-wrappers sqlite3_malloc()/sqlite3_free().  Those wrappers
** are designed for use on embedded systems where memory is scarce and
** malloc failures happen frequently.  OS/2 does not typically run on
** embedded systems, and when it does the developers normally have bigger
** problems to worry about than running out of memory.  So there is not
** a compelling need to use the wrappers.
**
** But there is a good reason to not use the wrappers.  If we use the
** wrappers then we will get simulated malloc() failures within this
** driver.  And that causes all kinds of problems for our tests.  We
** could enhance SQLite to deal with simulated malloc failures within
** the OS driver, but the code to deal with those failure would not
** be exercised on Linux (which does not need to malloc() in the driver)
** and so we would have difficulty writing coverage tests for that
** code.  Better to leave the code out, we think.
**
** The point of this discussion is as follows:  When creating a new
** OS layer for an embedded system, if you use this file as an example,
** avoid the use of malloc()/free().  Those routines work ok on OS/2
** desktops but not so well in embedded systems.
*/

/*
** Macros used to determine whether or not to use threads.
*/
#if defined(SQLITE_THREADSAFE) && SQLITE_THREADSAFE
# define SQLITE_OS2_THREADS 1
#endif

/*
** Include code that is common to all os_*.c files
*/
#include "os_common.h"

/*
** The os2File structure is subclass of sqlite3_file specific for the OS/2
** protability layer.
*/
typedef struct os2File os2File;
struct os2File {
  const sqlite3_io_methods *pMethod;  /* Always the first entry */
  HFILE h;                  /* Handle for accessing the file */
  char* pathToDel;          /* Name of file to delete on close, NULL if not */
  unsigned char locktype;   /* Type of lock currently held on this file */
};

#define LOCK_TIMEOUT 10L /* the default locking timeout */

/*****************************************************************************
** The next group of routines implement the I/O methods specified
** by the sqlite3_io_methods object.
******************************************************************************/

/*
** Close a file.
*/
int os2Close( sqlite3_file *id ){
  APIRET rc = NO_ERROR;
  os2File *pFile;
  if( id && (pFile = (os2File*)id) != 0 ){
    OSTRACE2( "CLOSE %d\n", pFile->h );
    rc = DosClose( pFile->h );
    pFile->locktype = NO_LOCK;
    if( pFile->pathToDel != NULL ){
      rc = DosForceDelete( (PSZ)pFile->pathToDel );
      free( pFile->pathToDel );
      pFile->pathToDel = NULL;
    }
    id = 0;
    OpenCounter( -1 );
  }

  return rc == NO_ERROR ? SQLITE_OK : SQLITE_IOERR;
}

/*
** Read data from a file into a buffer.  Return SQLITE_OK if all
** bytes were read successfully and SQLITE_IOERR if anything goes
** wrong.
*/
int os2Read(
  sqlite3_file *id,               /* File to read from */
  void *pBuf,                     /* Write content into this buffer */
  int amt,                        /* Number of bytes to read */
  sqlite3_int64 offset            /* Begin reading at this offset */
){
  ULONG fileLocation = 0L;
  ULONG got;
  os2File *pFile = (os2File*)id;
  assert( id!=0 );
  SimulateIOError( return SQLITE_IOERR_READ );
  OSTRACE3( "READ %d lock=%d\n", pFile->h, pFile->locktype );
  if( DosSetFilePtr(pFile->h, offset, FILE_BEGIN, &fileLocation) != NO_ERROR ){
    return SQLITE_IOERR;
  }
  if( DosRead( pFile->h, pBuf, amt, &got ) != NO_ERROR ){
    return SQLITE_IOERR_READ;
  }
  if( got == (ULONG)amt )
    return SQLITE_OK;
  else {
    memset(&((char*)pBuf)[got], 0, amt-got);
    return SQLITE_IOERR_SHORT_READ;
  }
}

/*
** Write data from a buffer into a file.  Return SQLITE_OK on success
** or some other error code on failure.
*/
int os2Write(
  sqlite3_file *id,               /* File to write into */
  const void *pBuf,               /* The bytes to be written */
  int amt,                        /* Number of bytes to write */
  sqlite3_int64 offset            /* Offset into the file to begin writing at */
){
  ULONG fileLocation = 0L;
  APIRET rc = NO_ERROR;
  ULONG wrote;
  os2File *pFile = (os2File*)id;
  assert( id!=0 );
  SimulateIOError( return SQLITE_IOERR_WRITE );
  SimulateDiskfullError( return SQLITE_FULL );
  OSTRACE3( "WRITE %d lock=%d\n", pFile->h, pFile->locktype );
  if( DosSetFilePtr(pFile->h, offset, FILE_BEGIN, &fileLocation) != NO_ERROR ){
    return SQLITE_IOERR;
  }
  assert( amt>0 );
  while( amt > 0 &&
         ( rc = DosWrite( pFile->h, (PVOID)pBuf, amt, &wrote ) ) == NO_ERROR &&
         wrote > 0
  ){
    amt -= wrote;
    pBuf = &((char*)pBuf)[wrote];
  }

  return ( rc != NO_ERROR || amt > (int)wrote ) ? SQLITE_FULL : SQLITE_OK;
}

/*
** Truncate an open file to a specified size
*/
int os2Truncate( sqlite3_file *id, i64 nByte ){
  APIRET rc = NO_ERROR;
  os2File *pFile = (os2File*)id;
  OSTRACE3( "TRUNCATE %d %lld\n", pFile->h, nByte );
  SimulateIOError( return SQLITE_IOERR_TRUNCATE );
  rc = DosSetFileSize( pFile->h, nByte );
  return rc == NO_ERROR ? SQLITE_OK : SQLITE_IOERR;
}

#ifdef SQLITE_TEST
/*
** Count the number of fullsyncs and normal syncs.  This is used to test
** that syncs and fullsyncs are occuring at the right times.
*/
int sqlite3_sync_count = 0;
int sqlite3_fullsync_count = 0;
#endif

/*
** Make sure all writes to a particular file are committed to disk.
*/
int os2Sync( sqlite3_file *id, int flags ){
  os2File *pFile = (os2File*)id;
  OSTRACE3( "SYNC %d lock=%d\n", pFile->h, pFile->locktype );
#ifdef SQLITE_TEST
  if( flags & SQLITE_SYNC_FULL){
    sqlite3_fullsync_count++;
  }
  sqlite3_sync_count++;
#endif
  return DosResetBuffer( pFile->h ) == NO_ERROR ? SQLITE_OK : SQLITE_IOERR;
}

/*
** Determine the current size of a file in bytes
*/
int os2FileSize( sqlite3_file *id, sqlite3_int64 *pSize ){
  APIRET rc = NO_ERROR;
  FILESTATUS3 fsts3FileInfo;
  memset(&fsts3FileInfo, 0, sizeof(fsts3FileInfo));
  assert( id!=0 );
  SimulateIOError( return SQLITE_IOERR );
  rc = DosQueryFileInfo( ((os2File*)id)->h, FIL_STANDARD, &fsts3FileInfo, sizeof(FILESTATUS3) );
  if( rc == NO_ERROR ){
    *pSize = fsts3FileInfo.cbFile;
    return SQLITE_OK;
  }else{
    return SQLITE_IOERR;
  }
}

/*
** Acquire a reader lock.
*/
static int getReadLock( os2File *pFile ){
  FILELOCK  LockArea,
            UnlockArea;
  APIRET res;
  memset(&LockArea, 0, sizeof(LockArea));
  memset(&UnlockArea, 0, sizeof(UnlockArea));
  LockArea.lOffset = SHARED_FIRST;
  LockArea.lRange = SHARED_SIZE;
  UnlockArea.lOffset = 0L;
  UnlockArea.lRange = 0L;
  res = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, LOCK_TIMEOUT, 1L );
  OSTRACE3( "GETREADLOCK %d res=%d\n", pFile->h, res );
  return res;
}

/*
** Undo a readlock
*/
static int unlockReadLock( os2File *id ){
  FILELOCK  LockArea,
            UnlockArea;
  APIRET res;
  memset(&LockArea, 0, sizeof(LockArea));
  memset(&UnlockArea, 0, sizeof(UnlockArea));
  LockArea.lOffset = 0L;
  LockArea.lRange = 0L;
  UnlockArea.lOffset = SHARED_FIRST;
  UnlockArea.lRange = SHARED_SIZE;
  res = DosSetFileLocks( id->h, &UnlockArea, &LockArea, LOCK_TIMEOUT, 1L );
  OSTRACE3( "UNLOCK-READLOCK file handle=%d res=%d?\n", id->h, res );
  return res;
}

/*
** Lock the file with the lock specified by parameter locktype - one
** of the following:
**
**     (1) SHARED_LOCK
**     (2) RESERVED_LOCK
**     (3) PENDING_LOCK
**     (4) EXCLUSIVE_LOCK
**
** Sometimes when requesting one lock state, additional lock states
** are inserted in between.  The locking might fail on one of the later
** transitions leaving the lock state different from what it started but
** still short of its goal.  The following chart shows the allowed
** transitions and the inserted intermediate states:
**
**    UNLOCKED -> SHARED
**    SHARED -> RESERVED
**    SHARED -> (PENDING) -> EXCLUSIVE
**    RESERVED -> (PENDING) -> EXCLUSIVE
**    PENDING -> EXCLUSIVE
**
** This routine will only increase a lock.  The os2Unlock() routine
** erases all locks at once and returns us immediately to locking level 0.
** It is not possible to lower the locking level one step at a time.  You
** must go straight to locking level 0.
*/
int os2Lock( sqlite3_file *id, int locktype ){
  int rc = SQLITE_OK;       /* Return code from subroutines */
  APIRET res = NO_ERROR;    /* Result of an OS/2 lock call */
  int newLocktype;       /* Set pFile->locktype to this value before exiting */
  int gotPendingLock = 0;/* True if we acquired a PENDING lock this time */
  FILELOCK  LockArea,
            UnlockArea;
  os2File *pFile = (os2File*)id;
  memset(&LockArea, 0, sizeof(LockArea));
  memset(&UnlockArea, 0, sizeof(UnlockArea));
  assert( pFile!=0 );
  OSTRACE4( "LOCK %d %d was %d\n", pFile->h, locktype, pFile->locktype );

  /* If there is already a lock of this type or more restrictive on the
  ** os2File, do nothing. Don't use the end_lock: exit path, as
  ** sqlite3_mutex_enter() hasn't been called yet.
  */
  if( pFile->locktype>=locktype ){
    OSTRACE3( "LOCK %d %d ok (already held)\n", pFile->h, locktype );
    return SQLITE_OK;
  }

  /* Make sure the locking sequence is correct
  */
  assert( pFile->locktype!=NO_LOCK || locktype==SHARED_LOCK );
  assert( locktype!=PENDING_LOCK );
  assert( locktype!=RESERVED_LOCK || pFile->locktype==SHARED_LOCK );

  /* Lock the PENDING_LOCK byte if we need to acquire a PENDING lock or
  ** a SHARED lock.  If we are acquiring a SHARED lock, the acquisition of
  ** the PENDING_LOCK byte is temporary.
  */
  newLocktype = pFile->locktype;
  if( pFile->locktype==NO_LOCK
      || (locktype==EXCLUSIVE_LOCK && pFile->locktype==RESERVED_LOCK)
  ){
    LockArea.lOffset = PENDING_BYTE;
    LockArea.lRange = 1L;
    UnlockArea.lOffset = 0L;
    UnlockArea.lRange = 0L;

    /* wait longer than LOCK_TIMEOUT here not to have to try multiple times */
    res = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, 100L, 0L );
    if( res == NO_ERROR ){
      gotPendingLock = 1;
      OSTRACE3( "LOCK %d pending lock boolean set.  res=%d\n", pFile->h, res );
    }
  }

  /* Acquire a shared lock
  */
  if( locktype==SHARED_LOCK && res == NO_ERROR ){
    assert( pFile->locktype==NO_LOCK );
    res = getReadLock(pFile);
    if( res == NO_ERROR ){
      newLocktype = SHARED_LOCK;
    }
    OSTRACE3( "LOCK %d acquire shared lock. res=%d\n", pFile->h, res );
  }

  /* Acquire a RESERVED lock
  */
  if( locktype==RESERVED_LOCK && res == NO_ERROR ){
    assert( pFile->locktype==SHARED_LOCK );
    LockArea.lOffset = RESERVED_BYTE;
    LockArea.lRange = 1L;
    UnlockArea.lOffset = 0L;
    UnlockArea.lRange = 0L;
    res = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, LOCK_TIMEOUT, 0L );
    if( res == NO_ERROR ){
      newLocktype = RESERVED_LOCK;
    }
    OSTRACE3( "LOCK %d acquire reserved lock. res=%d\n", pFile->h, res );
  }

  /* Acquire a PENDING lock
  */
  if( locktype==EXCLUSIVE_LOCK && res == NO_ERROR ){
    newLocktype = PENDING_LOCK;
    gotPendingLock = 0;
    OSTRACE2( "LOCK %d acquire pending lock. pending lock boolean unset.\n", pFile->h );
  }

  /* Acquire an EXCLUSIVE lock
  */
  if( locktype==EXCLUSIVE_LOCK && res == NO_ERROR ){
    assert( pFile->locktype>=SHARED_LOCK );
    res = unlockReadLock(pFile);
    OSTRACE2( "unreadlock = %d\n", res );
    LockArea.lOffset = SHARED_FIRST;
    LockArea.lRange = SHARED_SIZE;
    UnlockArea.lOffset = 0L;
    UnlockArea.lRange = 0L;
    res = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, LOCK_TIMEOUT, 0L );
    if( res == NO_ERROR ){
      newLocktype = EXCLUSIVE_LOCK;
    }else{
      OSTRACE2( "OS/2 error-code = %d\n", res );
      getReadLock(pFile);
    }
    OSTRACE3( "LOCK %d acquire exclusive lock.  res=%d\n", pFile->h, res );
  }

  /* If we are holding a PENDING lock that ought to be released, then
  ** release it now.
  */
  if( gotPendingLock && locktype==SHARED_LOCK ){
    int r;
    LockArea.lOffset = 0L;
    LockArea.lRange = 0L;
    UnlockArea.lOffset = PENDING_BYTE;
    UnlockArea.lRange = 1L;
    r = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, LOCK_TIMEOUT, 0L );
    OSTRACE3( "LOCK %d unlocking pending/is shared. r=%d\n", pFile->h, r );
  }

  /* Update the state of the lock has held in the file descriptor then
  ** return the appropriate result code.
  */
  if( res == NO_ERROR ){
    rc = SQLITE_OK;
  }else{
    OSTRACE4( "LOCK FAILED %d trying for %d but got %d\n", pFile->h,
              locktype, newLocktype );
    rc = SQLITE_BUSY;
  }
  pFile->locktype = newLocktype;
  OSTRACE3( "LOCK %d now %d\n", pFile->h, pFile->locktype );
  return rc;
}

/*
** This routine checks if there is a RESERVED lock held on the specified
** file by this or any other process. If such a lock is held, return
** non-zero, otherwise zero.
*/
int os2CheckReservedLock( sqlite3_file *id ){
  int r = 0;
  os2File *pFile = (os2File*)id;
  assert( pFile!=0 );
  if( pFile->locktype>=RESERVED_LOCK ){
    r = 1;
    OSTRACE3( "TEST WR-LOCK %d %d (local)\n", pFile->h, r );
  }else{
    FILELOCK  LockArea,
              UnlockArea;
    APIRET rc = NO_ERROR;
    memset(&LockArea, 0, sizeof(LockArea));
    memset(&UnlockArea, 0, sizeof(UnlockArea));
    LockArea.lOffset = RESERVED_BYTE;
    LockArea.lRange = 1L;
    UnlockArea.lOffset = 0L;
    UnlockArea.lRange = 0L;
    rc = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, LOCK_TIMEOUT, 0L );
    OSTRACE3( "TEST WR-LOCK %d lock reserved byte rc=%d\n", pFile->h, rc );
    if( rc == NO_ERROR ){
      APIRET rcu = NO_ERROR; /* return code for unlocking */
      LockArea.lOffset = 0L;
      LockArea.lRange = 0L;
      UnlockArea.lOffset = RESERVED_BYTE;
      UnlockArea.lRange = 1L;
      rcu = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, LOCK_TIMEOUT, 0L );
      OSTRACE3( "TEST WR-LOCK %d unlock reserved byte r=%d\n", pFile->h, rcu );
    }
    r = !(rc == NO_ERROR);
    OSTRACE3( "TEST WR-LOCK %d %d (remote)\n", pFile->h, r );
  }
  return r;
}

/*
** Lower the locking level on file descriptor id to locktype.  locktype
** must be either NO_LOCK or SHARED_LOCK.
**
** If the locking level of the file descriptor is already at or below
** the requested locking level, this routine is a no-op.
**
** It is not possible for this routine to fail if the second argument
** is NO_LOCK.  If the second argument is SHARED_LOCK then this routine
** might return SQLITE_IOERR;
*/
int os2Unlock( sqlite3_file *id, int locktype ){
  int type;
  os2File *pFile = (os2File*)id;
  APIRET rc = SQLITE_OK;
  APIRET res = NO_ERROR;
  FILELOCK  LockArea,
            UnlockArea;
  memset(&LockArea, 0, sizeof(LockArea));
  memset(&UnlockArea, 0, sizeof(UnlockArea));
  assert( pFile!=0 );
  assert( locktype<=SHARED_LOCK );
  OSTRACE4( "UNLOCK %d to %d was %d\n", pFile->h, locktype, pFile->locktype );
  type = pFile->locktype;
  if( type>=EXCLUSIVE_LOCK ){
    LockArea.lOffset = 0L;
    LockArea.lRange = 0L;
    UnlockArea.lOffset = SHARED_FIRST;
    UnlockArea.lRange = SHARED_SIZE;
    res = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, LOCK_TIMEOUT, 0L );
    OSTRACE3( "UNLOCK %d exclusive lock res=%d\n", pFile->h, res );
    if( locktype==SHARED_LOCK && getReadLock(pFile) != NO_ERROR ){
      /* This should never happen.  We should always be able to
      ** reacquire the read lock */
      OSTRACE3( "UNLOCK %d to %d getReadLock() failed\n", pFile->h, locktype );
      rc = SQLITE_IOERR_UNLOCK;
    }
  }
  if( type>=RESERVED_LOCK ){
    LockArea.lOffset = 0L;
    LockArea.lRange = 0L;
    UnlockArea.lOffset = RESERVED_BYTE;
    UnlockArea.lRange = 1L;
    res = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, LOCK_TIMEOUT, 0L );
    OSTRACE3( "UNLOCK %d reserved res=%d\n", pFile->h, res );
  }
  if( locktype==NO_LOCK && type>=SHARED_LOCK ){
    res = unlockReadLock(pFile);
    OSTRACE5( "UNLOCK %d is %d want %d res=%d\n", pFile->h, type, locktype, res );
  }
  if( type>=PENDING_LOCK ){
    LockArea.lOffset = 0L;
    LockArea.lRange = 0L;
    UnlockArea.lOffset = PENDING_BYTE;
    UnlockArea.lRange = 1L;
    res = DosSetFileLocks( pFile->h, &UnlockArea, &LockArea, LOCK_TIMEOUT, 0L );
    OSTRACE3( "UNLOCK %d pending res=%d\n", pFile->h, res );
  }
  pFile->locktype = locktype;
  OSTRACE3( "UNLOCK %d now %d\n", pFile->h, pFile->locktype );
  return rc;
}

/*
** Control and query of the open file handle.
*/
static int os2FileControl(sqlite3_file *id, int op, void *pArg){
  switch( op ){
    case SQLITE_FCNTL_LOCKSTATE: {
      *(int*)pArg = ((os2File*)id)->locktype;
      OSTRACE3( "FCNTL_LOCKSTATE %d lock=%d\n", ((os2File*)id)->h, ((os2File*)id)->locktype );
      return SQLITE_OK;
    }
  }
  return SQLITE_ERROR;
}

/*
** Return the sector size in bytes of the underlying block device for
** the specified file. This is almost always 512 bytes, but may be
** larger for some devices.
**
** SQLite code assumes this function cannot fail. It also assumes that
** if two files are created in the same file-system directory (i.e.
** a database and its journal file) that the sector size will be the
** same for both.
*/
static int os2SectorSize(sqlite3_file *id){
  return SQLITE_DEFAULT_SECTOR_SIZE;
}

/*
** Return a vector of device characteristics.
*/
static int os2DeviceCharacteristics(sqlite3_file *id){
  return 0;
}

/*
** Helper function to convert UTF-8 filenames to local OS/2 codepage.
** The two-step process: first convert the incoming UTF-8 string
** into UCS-2 and then from UCS-2 to the current codepage.
** The returned char pointer has to be freed.
*/
char *convertUtf8PathToCp(const char *in)
{
  UconvObject uconv;
  UniChar ucsUtf8Cp[12],
          tempPath[CCHMAXPATH];
  char *out;
  int rc = 0;

  out = (char *)calloc(CCHMAXPATH, 1);

  /* determine string for the conversion of UTF-8 which is CP1208 */
  rc = UniMapCpToUcsCp(1208, ucsUtf8Cp, 12);
  rc = UniCreateUconvObject(ucsUtf8Cp, &uconv);
  rc = UniStrToUcs(uconv, tempPath, (char *)in, CCHMAXPATH);
  rc = UniFreeUconvObject(uconv);

  /* conversion for current codepage which can be used for paths */
  rc = UniCreateUconvObject((UniChar *)L"@path=yes", &uconv);
  rc = UniStrFromUcs(uconv, out, tempPath, CCHMAXPATH);
  rc = UniFreeUconvObject(uconv);

  return out;
}

/*
** Helper function to convert filenames from local codepage to UTF-8.
** The two-step process: first convert the incoming codepage-specific
** string into UCS-2 and then from UCS-2 to the codepage of UTF-8.
** The returned char pointer has to be freed.
*/
char *convertCpPathToUtf8(const char *in)
{
  UconvObject uconv;
  UniChar ucsUtf8Cp[12],
          tempPath[CCHMAXPATH];
  char *out;
  int rc = 0;

  out = (char *)calloc(CCHMAXPATH, 1);

  /* conversion for current codepage which can be used for paths */
  rc = UniCreateUconvObject((UniChar *)L"@path=yes", &uconv);
  rc = UniStrToUcs(uconv, tempPath, (char *)in, CCHMAXPATH);
  rc = UniFreeUconvObject(uconv);

  /* determine string for the conversion of UTF-8 which is CP1208 */
  rc = UniMapCpToUcsCp(1208, ucsUtf8Cp, 12);
  rc = UniCreateUconvObject(ucsUtf8Cp, &uconv);
  rc = UniStrFromUcs(uconv, out, tempPath, CCHMAXPATH);
  rc = UniFreeUconvObject(uconv);

  return out;
}

/*
** This vector defines all the methods that can operate on an
** sqlite3_file for os2.
*/
static const sqlite3_io_methods os2IoMethod = {
  1,                        /* iVersion */
  os2Close,
  os2Read,
  os2Write,
  os2Truncate,
  os2Sync,
  os2FileSize,
  os2Lock,
  os2Unlock,
  os2CheckReservedLock,
  os2FileControl,
  os2SectorSize,
  os2DeviceCharacteristics
};

/***************************************************************************
** Here ends the I/O methods that form the sqlite3_io_methods object.
**
** The next block of code implements the VFS methods.
****************************************************************************/

/*
** Open a file.
*/
static int os2Open(
  sqlite3_vfs *pVfs,            /* Not used */
  const char *zName,            /* Name of the file */
  sqlite3_file *id,             /* Write the SQLite file handle here */
  int flags,                    /* Open mode flags */
  int *pOutFlags                /* Status return flags */
){
  HFILE h;
  ULONG ulFileAttribute = 0;
  ULONG ulOpenFlags = 0;
  ULONG ulOpenMode = 0;
  os2File *pFile = (os2File*)id;
  APIRET rc = NO_ERROR;
  ULONG ulAction;

  memset( pFile, 0, sizeof(*pFile) );

  OSTRACE2( "OPEN want %d\n", flags );

  //ulOpenMode = flags & SQLITE_OPEN_READWRITE ? OPEN_ACCESS_READWRITE : OPEN_ACCESS_READONLY;
  if( flags & SQLITE_OPEN_READWRITE ){
    ulOpenMode |= OPEN_ACCESS_READWRITE;
    OSTRACE1( "OPEN read/write\n" );
  }else{
    ulOpenMode |= OPEN_ACCESS_READONLY;
    OSTRACE1( "OPEN read only\n" );
  }

  //ulOpenFlags = flags & SQLITE_OPEN_CREATE ? OPEN_ACTION_CREATE_IF_NEW : OPEN_ACTION_FAIL_IF_NEW;
  if( flags & SQLITE_OPEN_CREATE ){
    ulOpenFlags |= OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW;
    OSTRACE1( "OPEN open new/create\n" );
  }else{
    ulOpenFlags |= OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW;
    OSTRACE1( "OPEN open existing\n" );
  }

  //ulOpenMode |= flags & SQLITE_OPEN_MAIN_DB ? OPEN_SHARE_DENYNONE : OPEN_SHARE_DENYWRITE;
  if( flags & SQLITE_OPEN_MAIN_DB ){
    ulOpenMode |= OPEN_SHARE_DENYNONE;
    OSTRACE1( "OPEN share read/write\n" );
  }else{
    ulOpenMode |= OPEN_SHARE_DENYWRITE;
    OSTRACE1( "OPEN share read only\n" );
  }

  if( flags & (SQLITE_OPEN_TEMP_DB | SQLITE_OPEN_TEMP_JOURNAL
               | SQLITE_OPEN_SUBJOURNAL) ){
    char pathUtf8[CCHMAXPATH];
    //ulFileAttribute = FILE_HIDDEN;  //for debugging, we want to make sure it is deleted
    ulFileAttribute = FILE_NORMAL;
    sqlite3OsFullPathname( pVfs, zName, CCHMAXPATH, pathUtf8 );
    pFile->pathToDel = convertUtf8PathToCp( pathUtf8 );
    OSTRACE1( "OPEN hidden/delete on close file attributes\n" );
  }else{
    ulFileAttribute = FILE_ARCHIVED | FILE_NORMAL;
    pFile->pathToDel = NULL;
    OSTRACE1( "OPEN normal file attribute\n" );
  }

  /* always open in random access mode for possibly better speed */
  ulOpenMode |= OPEN_FLAGS_RANDOM;
  ulOpenMode |= OPEN_FLAGS_FAIL_ON_ERROR;
  ulOpenMode |= OPEN_FLAGS_NOINHERIT;

  char *zNameCp = convertUtf8PathToCp( zName );
  rc = DosOpen( (PSZ)zNameCp,
                &h,
                &ulAction,
                0L,
                ulFileAttribute,
                ulOpenFlags,
                ulOpenMode,
                (PEAOP2)NULL );
  free( zNameCp );
  if( rc != NO_ERROR ){
    OSTRACE7( "OPEN Invalid handle rc=%d: zName=%s, ulAction=%#lx, ulAttr=%#lx, ulFlags=%#lx, ulMode=%#lx\n",
              rc, zName, ulAction, ulFileAttribute, ulOpenFlags, ulOpenMode );
    free( pFile->pathToDel );
    pFile->pathToDel = NULL;
    if( flags & SQLITE_OPEN_READWRITE ){
      OSTRACE2( "OPEN %d Invalid handle\n", ((flags | SQLITE_OPEN_READONLY) & ~SQLITE_OPEN_READWRITE) );
      return os2Open( 0, zName, id,
                      ((flags | SQLITE_OPEN_READONLY) & ~SQLITE_OPEN_READWRITE),
                      pOutFlags );
    }else{
      return SQLITE_CANTOPEN;
    }
  }

  if( pOutFlags ){
    *pOutFlags = flags & SQLITE_OPEN_READWRITE ? SQLITE_OPEN_READWRITE : SQLITE_OPEN_READONLY;
  }

  pFile->pMethod = &os2IoMethod;
  pFile->h = h;
  OpenCounter(+1);
  OSTRACE3( "OPEN %d pOutFlags=%d\n", pFile->h, pOutFlags );
  return SQLITE_OK;
}

/*
** Delete the named file.
*/
int os2Delete(
  sqlite3_vfs *pVfs,                     /* Not used on os2 */
  const char *zFilename,                 /* Name of file to delete */
  int syncDir                            /* Not used on os2 */
){
  APIRET rc = NO_ERROR;
  SimulateIOError(return SQLITE_IOERR_DELETE);
  char *zFilenameCp = convertUtf8PathToCp( zFilename );
  rc = DosDelete( (PSZ)zFilenameCp );
  free( zFilenameCp );
  OSTRACE2( "DELETE \"%s\"\n", zFilename );
  return rc == NO_ERROR ? SQLITE_OK : SQLITE_IOERR;
}

/*
** Check the existance and status of a file.
*/
static int os2Access(
  sqlite3_vfs *pVfs,        /* Not used on os2 */
  const char *zFilename,    /* Name of file to check */
  int flags                 /* Type of test to make on this file */
){
  FILESTATUS3 fsts3ConfigInfo;
  APIRET rc = NO_ERROR;

  memset( &fsts3ConfigInfo, 0, sizeof(fsts3ConfigInfo) );
  char *zFilenameCp = convertUtf8PathToCp( zFilename );
  rc = DosQueryPathInfo( (PSZ)zFilenameCp, FIL_STANDARD,
                         &fsts3ConfigInfo, sizeof(FILESTATUS3) );
  free( zFilenameCp );
  OSTRACE4( "ACCESS fsts3ConfigInfo.attrFile=%d flags=%d rc=%d\n",
            fsts3ConfigInfo.attrFile, flags, rc );
  switch( flags ){
    case SQLITE_ACCESS_READ:
    case SQLITE_ACCESS_EXISTS:
      rc = (rc == NO_ERROR);
      OSTRACE3( "ACCESS %s access of read and exists  rc=%d\n", zFilename, rc );
      break;
    case SQLITE_ACCESS_READWRITE:
      rc = (fsts3ConfigInfo.attrFile & FILE_READONLY) == 0;
      OSTRACE3( "ACCESS %s access of read/write  rc=%d\n", zFilename, rc );
      break;
    default:
      assert( !"Invalid flags argument" );
  }
  return rc;
}


/*
** Create a temporary file name in zBuf.  zBuf must be big enough to
** hold at pVfs->mxPathname characters.
*/
static int os2GetTempname( sqlite3_vfs *pVfs, int nBuf, char *zBuf ){
  static const unsigned char zChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  int i, j;
  char zTempPathBuf[3];
  PSZ zTempPath = (PSZ)&zTempPathBuf;
  char *zTempPathUTF;
  if( DosScanEnv( (PSZ)"TEMP", &zTempPath ) ){
    if( DosScanEnv( (PSZ)"TMP", &zTempPath ) ){
      if( DosScanEnv( (PSZ)"TMPDIR", &zTempPath ) ){
           ULONG ulDriveNum = 0, ulDriveMap = 0;
           DosQueryCurrentDisk( &ulDriveNum, &ulDriveMap );
           sprintf( (char*)zTempPath, "%c:", (char)( 'A' + ulDriveNum - 1 ) );
      }
    }
  }
  /* strip off a trailing slashes or backslashes, otherwise we would get *
   * multiple (back)slashes which causes DosOpen() to fail               */
  j = strlen(zTempPath);
  while( j > 0 && ( zTempPath[j-1] == '\\' || zTempPath[j-1] == '/' ) ){
    j--;
  }
  zTempPath[j] = '\0';
  zTempPathUTF = convertCpPathToUtf8( zTempPath );
  sqlite3_snprintf( nBuf-30, zBuf,
                    "%s\\"SQLITE_TEMP_FILE_PREFIX, zTempPathUTF );
  free( zTempPathUTF );
  j = strlen( zBuf );
  sqlite3_randomness( 20, &zBuf[j] );
  for( i = 0; i < 20; i++, j++ ){
    zBuf[j] = (char)zChars[ ((unsigned char)zBuf[j])%(sizeof(zChars)-1) ];
  }
  zBuf[j] = 0;
  OSTRACE2( "TEMP FILENAME: %s\n", zBuf );
  return SQLITE_OK;
}


/*
** Turn a relative pathname into a full pathname.  Write the full
** pathname into zFull[].  zFull[] will be at least pVfs->mxPathname
** bytes in size.
*/
static int os2FullPathname(
  sqlite3_vfs *pVfs,          /* Pointer to vfs object */
  const char *zRelative,      /* Possibly relative input path */
  int nFull,                  /* Size of output buffer in bytes */
  char *zFull                 /* Output buffer */
){
  char *zRelativeCp = convertUtf8PathToCp( zRelative );
  char zFullCp[CCHMAXPATH];
  char *zFullUTF;
  APIRET rc = DosQueryPathInfo( zRelativeCp, FIL_QUERYFULLNAME, zFullCp,
                                CCHMAXPATH );
  free( zRelativeCp );
  zFullUTF = convertCpPathToUtf8( zFullCp );
  sqlite3_snprintf( nFull, zFull, zFullUTF );
  free( zFullUTF );
  return rc == NO_ERROR ? SQLITE_OK : SQLITE_IOERR;
}

#ifndef SQLITE_OMIT_LOAD_EXTENSION
/*
** Interfaces for opening a shared library, finding entry points
** within the shared library, and closing the shared library.
*/
/*
** Interfaces for opening a shared library, finding entry points
** within the shared library, and closing the shared library.
*/
static void *os2DlOpen(sqlite3_vfs *pVfs, const char *zFilename){
  UCHAR loadErr[256];
  HMODULE hmod;
  APIRET rc;
  char *zFilenameCp = convertUtf8PathToCp(zFilename);
  rc = DosLoadModule((PSZ)loadErr, sizeof(loadErr), zFilenameCp, &hmod);
  free(zFilenameCp);
  return rc != NO_ERROR ? 0 : (void*)hmod;
}
/*
** A no-op since the error code is returned on the DosLoadModule call.
** os2Dlopen returns zero if DosLoadModule is not successful.
*/
static void os2DlError(sqlite3_vfs *pVfs, int nBuf, char *zBufOut){
/* no-op */
}
void *os2DlSym(sqlite3_vfs *pVfs, void *pHandle, const char *zSymbol){
  PFN pfn;
  APIRET rc;
  rc = DosQueryProcAddr((HMODULE)pHandle, 0L, zSymbol, &pfn);
  if( rc != NO_ERROR ){
    /* if the symbol itself was not found, search again for the same
     * symbol with an extra underscore, that might be needed depending
     * on the calling convention */
    char _zSymbol[256] = "_";
    strncat(_zSymbol, zSymbol, 255);
    rc = DosQueryProcAddr((HMODULE)pHandle, 0L, _zSymbol, &pfn);
  }
  return rc != NO_ERROR ? 0 : (void*)pfn;
}
void os2DlClose(sqlite3_vfs *pVfs, void *pHandle){
  DosFreeModule((HMODULE)pHandle);
}
#else /* if SQLITE_OMIT_LOAD_EXTENSION is defined: */
  #define os2DlOpen 0
  #define os2DlError 0
  #define os2DlSym 0
  #define os2DlClose 0
#endif


/*
** Write up to nBuf bytes of randomness into zBuf.
*/
static int os2Randomness(sqlite3_vfs *pVfs, int nBuf, char *zBuf ){
  ULONG sizeofULong = sizeof(ULONG);
  int n = 0;
  if( sizeof(DATETIME) <= nBuf - n ){
    DATETIME x;
    DosGetDateTime(&x);
    memcpy(&zBuf[n], &x, sizeof(x));
    n += sizeof(x);
  }

  if( sizeofULong <= nBuf - n ){
    PPIB ppib;
    DosGetInfoBlocks(NULL, &ppib);
    memcpy(&zBuf[n], &ppib->pib_ulpid, sizeofULong);
    n += sizeofULong;
  }

  if( sizeofULong <= nBuf - n ){
    PTIB ptib;
    DosGetInfoBlocks(&ptib, NULL);
    memcpy(&zBuf[n], &ptib->tib_ptib2->tib2_ultid, sizeofULong);
    n += sizeofULong;
  }

  /* if we still haven't filled the buffer yet the following will */
  /* grab everything once instead of making several calls for a single item */
  if( sizeofULong <= nBuf - n ){
    ULONG ulSysInfo[QSV_MAX];
    DosQuerySysInfo(1L, QSV_MAX, ulSysInfo, sizeofULong * QSV_MAX);

    memcpy(&zBuf[n], &ulSysInfo[QSV_MS_COUNT - 1], sizeofULong);
    n += sizeofULong;

    if( sizeofULong <= nBuf - n ){
      memcpy(&zBuf[n], &ulSysInfo[QSV_TIMER_INTERVAL - 1], sizeofULong);
      n += sizeofULong;
    }
    if( sizeofULong <= nBuf - n ){
      memcpy(&zBuf[n], &ulSysInfo[QSV_TIME_LOW - 1], sizeofULong);
      n += sizeofULong;
    }
    if( sizeofULong <= nBuf - n ){
      memcpy(&zBuf[n], &ulSysInfo[QSV_TIME_HIGH - 1], sizeofULong);
      n += sizeofULong;
    }
    if( sizeofULong <= nBuf - n ){
      memcpy(&zBuf[n], &ulSysInfo[QSV_TOTAVAILMEM - 1], sizeofULong);
      n += sizeofULong;
    }
  }

  return n;
}

/*
** Sleep for a little while.  Return the amount of time slept.
** The argument is the number of microseconds we want to sleep.
** The return value is the number of microseconds of sleep actually
** requested from the underlying operating system, a number which
** might be greater than or equal to the argument, but not less
** than the argument.
*/
static int os2Sleep( sqlite3_vfs *pVfs, int microsec ){
  DosSleep( (microsec/1000) );
  return microsec;
}

/*
** The following variable, if set to a non-zero value, becomes the result
** returned from sqlite3OsCurrentTime().  This is used for testing.
*/
#ifdef SQLITE_TEST
int sqlite3_current_time = 0;
#endif

/*
** Find the current time (in Universal Coordinated Time).  Write the
** current time and date as a Julian Day number into *prNow and
** return 0.  Return 1 if the time and date cannot be found.
*/
int os2CurrentTime( sqlite3_vfs *pVfs, double *prNow ){
  double now;
  SHORT minute; /* needs to be able to cope with negative timezone offset */
  USHORT second, hour,
         day, month, year;
  DATETIME dt;
  DosGetDateTime( &dt );
  second = (USHORT)dt.seconds;
  minute = (SHORT)dt.minutes + dt.timezone;
  hour = (USHORT)dt.hours;
  day = (USHORT)dt.day;
  month = (USHORT)dt.month;
  year = (USHORT)dt.year;

  /* Calculations from http://www.astro.keele.ac.uk/~rno/Astronomy/hjd.html
     http://www.astro.keele.ac.uk/~rno/Astronomy/hjd-0.1.c */
  /* Calculate the Julian days */
  now = day - 32076 +
    1461*(year + 4800 + (month - 14)/12)/4 +
    367*(month - 2 - (month - 14)/12*12)/12 -
    3*((year + 4900 + (month - 14)/12)/100)/4;

  /* Add the fractional hours, mins and seconds */
  now += (hour + 12.0)/24.0;
  now += minute/1440.0;
  now += second/86400.0;
  *prNow = now;
#ifdef SQLITE_TEST
  if( sqlite3_current_time ){
    *prNow = sqlite3_current_time/86400.0 + 2440587.5;
  }
#endif
  return 0;
}

/*
** Return a pointer to the sqlite3DefaultVfs structure.   We use
** a function rather than give the structure global scope because
** some compilers (MSVC) do not allow forward declarations of
** initialized structures.
*/
sqlite3_vfs *sqlite3OsDefaultVfs(void){
  static sqlite3_vfs os2Vfs = {
    1,                 /* iVersion */
    sizeof(os2File),   /* szOsFile */
    CCHMAXPATH,        /* mxPathname */
    0,                 /* pNext */
    "os2",             /* zName */
    0,                 /* pAppData */

    os2Open,           /* xOpen */
    os2Delete,         /* xDelete */
    os2Access,         /* xAccess */
    os2GetTempname,    /* xGetTempname */
    os2FullPathname,   /* xFullPathname */
    os2DlOpen,         /* xDlOpen */
    os2DlError,        /* xDlError */
    os2DlSym,          /* xDlSym */
    os2DlClose,        /* xDlClose */
    os2Randomness,     /* xRandomness */
    os2Sleep,          /* xSleep */
    os2CurrentTime     /* xCurrentTime */
  };

  return &os2Vfs;
}

#endif /* OS_OS2 */
