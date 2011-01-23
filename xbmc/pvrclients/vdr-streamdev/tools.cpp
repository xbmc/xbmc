/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/*
 * Most of this code is taken from tools.c in the Video Disk Recorder ('VDR')
 */

#include "tools.h"

ssize_t safe_read(int filedes, void *buffer, size_t size)
{
  for (;;) {
      ssize_t p = __read(filedes, buffer, size);
      if (p < 0 && errno == EINTR) {
         XBMC->Log(LOG_DEBUG, "EINTR while reading from file handle %d - retrying", filedes);
         continue;
         }
      return p;
      }
}

ssize_t safe_write(int filedes, const void *buffer, size_t size)
{
  ssize_t p = 0;
  ssize_t written = size;
  const unsigned char *ptr = (const unsigned char *)buffer;
  while (size > 0) {
        p = __write(filedes, ptr, size);
        if (p < 0) {
           if (errno == EINTR) {
              XBMC->Log(LOG_DEBUG, "EINTR while writing to file handle %d - retrying", filedes);
              continue;
              }
           break;
           }
        ptr  += p;
        size -= p;
        }
  return p < 0 ? p : written;
}

void writechar(int filedes, char c)
{
  safe_write(filedes, &c, sizeof(c));
}

int WriteAllOrNothing(int fd, const unsigned char *Data, int Length, int TimeoutMs, int RetryMs)
{
  int written = 0;
  while (Length > 0) {
        int w = write(fd, Data + written, Length);
        if (w > 0) {
           Length -= w;
           written += w;
           }
        else if (written > 0 && !FATALERRNO) {
           // we've started writing, so we must finish it!
           cTimeMs t;
           cPoller Poller(fd, true);
           Poller.Poll(RetryMs);
           if (TimeoutMs > 0 && (TimeoutMs -= t.Elapsed()) <= 0)
              break;
           }
        else
           // nothing written yet (or fatal error), so we can just return the error code:
           return w;
        }
  return written;
}

char *strcpyrealloc(char *dest, const char *src)
{
  if (src) {
     int l = max(dest ? strlen(dest) : 0, strlen(src)) + 1; // don't let the block get smaller!
     dest = (char *)realloc(dest, l);
     if (dest)
        strcpy(dest, src);
     else
        XBMC->Log(LOG_ERROR, "ERROR: out of memory");
     }
  else {
     free(dest);
     dest = NULL;
     }
  return dest;
}

char *strn0cpy(char *dest, const char *src, size_t n)
{
  char *s = dest;
  for ( ; --n && (*dest = *src) != 0; dest++, src++) ;
  *dest = 0;
  return s;
}

char *strreplace(char *s, char c1, char c2)
{
  if (s) {
     char *p = s;
     while (*p) {
           if (*p == c1)
              *p = c2;
           p++;
           }
     }
  return s;
}

char *strreplace(char *s, const char *s1, const char *s2)
{
  char *p = strstr(s, s1);
  if (p) {
     int of = p - s;
     int l  = strlen(s);
     int l1 = strlen(s1);
     int l2 = strlen(s2);
     if (l2 > l1)
        s = (char *)realloc(s, l + l2 - l1 + 1);
     char *sof = s + of;
     if (l2 != l1)
        memmove(sof + l2, sof + l1, l - of - l1 + 1);
     strncpy(sof, s2, l2);
     }
  return s;
}

char *stripspace(char *s)
{
  if (s && *s) {
     for (char *p = s + strlen(s) - 1; p >= s; p--) {
         if (!isspace(*p))
            break;
         *p = 0;
         }
     }
  return s;
}

char *compactspace(char *s)
{
  if (s && *s) {
     char *t = stripspace(skipspace(s));
     char *p = t;
     while (p && *p) {
           char *q = skipspace(p);
           if (q - p > 1)
              memmove(p + 1, q, strlen(q) + 1);
           p++;
           }
     if (t != s)
        memmove(s, t, strlen(t) + 1);
     }
  return s;
}

bool startswith(const char *s, const char *p)
{
  while (*p) {
        if (*p++ != *s++)
           return false;
        }
  return true;
}

bool endswith(const char *s, const char *p)
{
  const char *se = s + strlen(s) - 1;
  const char *pe = p + strlen(p) - 1;
  while (pe >= p) {
        if (*pe-- != *se-- || (se < s && pe >= p))
           return false;
        }
  return true;
}

bool isempty(const char *s)
{
  return !(s && *skipspace(s));
}

int numdigits(int n)
{
  int res = 1;
  while (n >= 10) {
        n /= 10;
        res++;
        }
  return res;
}

bool IsNumber(const char *s)
{
  if (!*s)
     return false;
  do {
     if (!isdigit(*s))
        return false;
     } while (*++s);
  return true;
}

CStdString AddDirectory(const char *DirName, const char *FileName)
{
  CStdString ret;
  ret.Format("%s/%s", DirName && *DirName ? DirName : ".", FileName);
  return ret;
}

char *ReadLink(const char *FileName)
{
#if defined(__WINDOWS__)
  return NULL;
#else
  if (!FileName)
    return NULL;
  char *TargetName = NULL;
  char *res = realpath(FileName, TargetName);
  if (!res)
  {
    if (errno == ENOENT) // file doesn't exist
      TargetName = strdup(FileName);
    else // some other error occurred
      XBMC->Log(LOG_ERROR, "ERROR (%s,%d,s): %m", __FILE__, __LINE__, FileName);
  }
  return TargetName;
#endif
}


// --- cTimeMs ---------------------------------------------------------------

cTimeMs::cTimeMs(int Ms)
{
  Set(Ms);
}

uint64_t cTimeMs::Now(void)
{
#if _POSIX_TIMERS > 0 && defined(_POSIX_MONOTONIC_CLOCK)
#define MIN_RESOLUTION 5 // ms
  static bool initialized = false;
  static bool monotonic = false;
  struct timespec tp;
  if (!initialized) {
     // check if monotonic timer is available and provides enough accurate resolution:
     if (clock_getres(CLOCK_MONOTONIC, &tp) == 0) {
        long Resolution = tp.tv_nsec;
        // require a minimum resolution:
        if (tp.tv_sec == 0 && tp.tv_nsec <= MIN_RESOLUTION * 1000000) {
           if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0) {
              XBMC->Log(LOG_DEBUG, "cTimeMs: using monotonic clock (resolution is %ld ns)", Resolution);
              monotonic = true;
              }
           else
              XBMC->Log(LOG_ERROR, "cTimeMs: clock_gettime(CLOCK_MONOTONIC) failed");
           }
        else
           XBMC->Log(LOG_DEBUG, "cTimeMs: not using monotonic clock - resolution is too bad (%ld s %ld ns)", tp.tv_sec, tp.tv_nsec);
        }
     else
        XBMC->Log(LOG_ERROR, "cTimeMs: clock_getres(CLOCK_MONOTONIC) failed");
     initialized = true;
     }
  if (monotonic) {
     if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
        return (uint64_t(tp.tv_sec)) * 1000 + tp.tv_nsec / 1000000;
     XBMC->Log(LOG_ERROR, "cTimeMs: clock_gettime(CLOCK_MONOTONIC) failed");
     monotonic = false;
     // fall back to gettimeofday()
     }
#else
#if !defined(__WINDOWS__)
#  warning Posix monotonic clock not available
#endif
#endif
  struct timeval t;
  if (gettimeofday(&t, NULL) == 0)
     return (uint64_t(t.tv_sec)) * 1000 + t.tv_usec / 1000;
  return 0;
}

void cTimeMs::Set(int Ms)
{
  begin = Now() + Ms;
}

bool cTimeMs::TimedOut(void)
{
  return Now() >= begin;
}

uint64_t cTimeMs::Elapsed(void)
{
  return Now() - begin;
}

// --- cPoller ---------------------------------------------------------------

cPoller::cPoller(int FileHandle, bool Out)
{
  numFileHandles = 0;
  Add(FileHandle, Out);
}

bool cPoller::Add(int FileHandle, bool Out)
{
  if (FileHandle >= 0) {
     for (int i = 0; i < numFileHandles; i++) {
         if (pfd[i].fd == FileHandle && pfd[i].events == (Out ? POLLOUT : POLLIN))
            return true;
         }
     if (numFileHandles < MaxPollFiles) {
        pfd[numFileHandles].fd = FileHandle;
        pfd[numFileHandles].events = Out ? POLLOUT : POLLIN;
        pfd[numFileHandles].revents = 0;
        numFileHandles++;
        return true;
        }
     XBMC->Log(LOG_ERROR, "ERROR: too many file handles in cPoller");
     }
  return false;
}

bool cPoller::Poll(int TimeoutMs)
{
  if (numFileHandles) {
     if (__poll(pfd, numFileHandles, TimeoutMs) != 0)
        return true; // returns true even in case of an error, to let the caller
                     // access the file and thus see the error code
     }
  return false;
}

// --- cFile -----------------------------------------------------------------

bool cFile::files[FD_SETSIZE] = { false };
int cFile::maxFiles = 0;

cFile::cFile(void)
{
  f = -1;
}

cFile::~cFile()
{
  Close();
}

bool cFile::Open(const char *FileName, int Flags, mode_t Mode)
{
  if (!IsOpen())
#ifdef __WINDOWS__
    return Open(open(FileName, Flags, 0));
#else
    return Open(open(FileName, Flags, Mode));
#endif
  XBMC->Log(LOG_ERROR, "ERROR: attempt to re-open %s", FileName);
  return false;
}

bool cFile::Open(int FileDes)
{
  if (FileDes >= 0) {
     if (!IsOpen()) {
        f = FileDes;
        if (f >= 0) {
           if (f < FD_SETSIZE) {
              if (f >= maxFiles)
                 maxFiles = f + 1;
              if (!files[f])
                 files[f] = true;
              else
                 XBMC->Log(LOG_ERROR, "ERROR: file descriptor %d already in files[]", f);
              return true;
              }
           else
              XBMC->Log(LOG_ERROR, "ERROR: file descriptor %d is larger than FD_SETSIZE (%d)", f, FD_SETSIZE);
           }
        }
     else
        XBMC->Log(LOG_ERROR, "ERROR: attempt to re-open file descriptor %d", FileDes);
     }
  return false;
}

void cFile::Close(void)
{
  if (f >= 0) {
     close(f);
     files[f] = false;
     f = -1;
     }
}

bool cFile::Ready(bool Wait)
{
  return f >= 0 && AnyFileReady(f, Wait ? 1000 : 0);
}

bool cFile::AnyFileReady(int FileDes, int TimeoutMs)
{
  fd_set set;
  FD_ZERO(&set);
  for (int i = 0; i < maxFiles; i++) {
      if (files[i])
         FD_SET(i, &set);
      }
  if (0 <= FileDes && FileDes < FD_SETSIZE && !files[FileDes])
     FD_SET(FileDes, &set); // in case we come in with an arbitrary descriptor
  if (TimeoutMs == 0)
     TimeoutMs = 10; // load gets too heavy with 0
  struct timeval timeout;
  timeout.tv_sec  = TimeoutMs / 1000;
  timeout.tv_usec = (TimeoutMs % 1000) * 1000;
  return select(FD_SETSIZE, &set, NULL, NULL, &timeout) > 0 && (FileDes < 0 || FD_ISSET(FileDes, &set));
}

bool cFile::FileReady(int FileDes, int TimeoutMs)
{
  fd_set set;
  struct timeval timeout;
  FD_ZERO(&set);
  FD_SET(FileDes, &set);
  if (TimeoutMs >= 0) {
     if (TimeoutMs < 100)
        TimeoutMs = 100;
     timeout.tv_sec  = TimeoutMs / 1000;
     timeout.tv_usec = (TimeoutMs % 1000) * 1000;
     }
  return select(FD_SETSIZE, &set, NULL, NULL, (TimeoutMs >= 0) ? &timeout : NULL) > 0 && FD_ISSET(FileDes, &set);
}

bool cFile::FileReadyForWriting(int FileDes, int TimeoutMs)
{
  fd_set set;
  struct timeval timeout;
  FD_ZERO(&set);
  FD_SET(FileDes, &set);
  if (TimeoutMs < 100)
     TimeoutMs = 100;
  timeout.tv_sec  = 0;
  timeout.tv_usec = TimeoutMs * 1000;
  return select(FD_SETSIZE, NULL, &set, NULL, &timeout) > 0 && FD_ISSET(FileDes, &set);
}


// --- cUnbufferedFile -------------------------------------------------------

#if !defined(__WINDOWS__)
#if !defined(__APPLE__)
#define USE_FADVISE
#endif
#endif

#define WRITE_BUFFER KILOBYTE(800)

cUnbufferedFile::cUnbufferedFile(void)
{
  fd = -1;
}

cUnbufferedFile::~cUnbufferedFile()
{
  Close();
}

int cUnbufferedFile::Open(const char *FileName, int Flags, mode_t Mode)
{
  Close();
  fd = open(FileName, Flags, Mode);
  curpos = 0;
#ifdef USE_FADVISE
  begin = lastpos = ahead = 0;
  cachedstart = 0;
  cachedend = 0;
  readahead = KILOBYTE(128);
  written = 0;
  totwritten = 0;
  if (fd >= 0)
     posix_fadvise(fd, 0, 0, POSIX_FADV_RANDOM); // we could use POSIX_FADV_SEQUENTIAL, but we do our own readahead, disabling the kernel one.
#endif
  return fd;
}

int cUnbufferedFile::Close(void)
{
  if (fd >= 0) {
#ifdef USE_FADVISE
     if (totwritten)    // if we wrote anything make sure the data has hit the disk before
        fdatasync(fd);  // calling fadvise, as this is our last chance to un-cache it.
     posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
#endif
     int OldFd = fd;
     fd = -1;
     return close(OldFd);
     }
  errno = EBADF;
  return -1;
}

// When replaying and going e.g. FF->PLAY the position jumps back 2..8M
// hence we do not want to drop recently accessed data at once.
// We try to handle the common cases such as PLAY->FF->PLAY, small
// jumps, moving editing marks etc.

#define FADVGRAN   KILOBYTE(4) // AKA fadvise-chunk-size; PAGE_SIZE or getpagesize(2) would also work.
#define READCHUNK  MEGABYTE(8)

void cUnbufferedFile::SetReadAhead(size_t ra)
{
  readahead = ra;
}

int cUnbufferedFile::FadviseDrop(off_t Offset, off_t Len)
{
#ifdef USE_FADVISE
  // rounding up the window to make sure that not PAGE_SIZE-aligned data gets freed.
  return posix_fadvise(fd, Offset - (FADVGRAN - 1), Len + (FADVGRAN - 1) * 2, POSIX_FADV_DONTNEED);
#else
  return 0;
#endif
}

off_t cUnbufferedFile::Seek(off_t Offset, int Whence)
{
  if (Whence == SEEK_SET && Offset == curpos)
     return curpos;
  curpos = lseek(fd, Offset, Whence);
  return curpos;
}

ssize_t cUnbufferedFile::Read(void *Data, size_t Size)
{
  if (fd >= 0) {
#ifdef USE_FADVISE
     off_t jumped = curpos-lastpos; // nonzero means we're not at the last offset
     if ((cachedstart < cachedend) && (curpos < cachedstart || curpos > cachedend)) {
        // current position is outside the cached window -- invalidate it.
        FadviseDrop(cachedstart, cachedend-cachedstart);
        cachedstart = curpos;
        cachedend = curpos;
        }
     cachedstart = min(cachedstart, curpos);
#endif
     ssize_t bytesRead = safe_read(fd, Data, Size);
#ifdef USE_FADVISE
     if (bytesRead > 0) {
        curpos += bytesRead;
        cachedend = max(cachedend, curpos);

        // Read ahead:
        // no jump? (allow small forward jump still inside readahead window).
        if (jumped >= 0 && jumped <= (off_t)readahead) {
           // Trigger the readahead IO, but only if we've used at least
           // 1/2 of the previously requested area. This avoids calling
           // fadvise() after every read() call.
           if (ahead - curpos < (off_t)(readahead / 2)) {
              posix_fadvise(fd, curpos, readahead, POSIX_FADV_WILLNEED);
              ahead = curpos + readahead;
              cachedend = max(cachedend, ahead);
              }
           if (readahead < Size * 32) { // automagically tune readahead size.
              readahead = Size * 32;
              }
           }
        else
           ahead = curpos; // jumped -> we really don't want any readahead, otherwise e.g. fast-rewind gets in trouble.
        }

     if (cachedstart < cachedend) {
        if (curpos - cachedstart > READCHUNK * 2) {
           // current position has moved forward enough, shrink tail window.
           FadviseDrop(cachedstart, curpos - READCHUNK - cachedstart);
           cachedstart = curpos - READCHUNK;
           }
        else if (cachedend > ahead && cachedend - curpos > READCHUNK * 2) {
           // current position has moved back enough, shrink head window.
           FadviseDrop(curpos + READCHUNK, cachedend - (curpos + READCHUNK));
           cachedend = curpos + READCHUNK;
           }
        }
     lastpos = curpos;
#endif
     return bytesRead;
     }
  return -1;
}

ssize_t cUnbufferedFile::Write(const void *Data, size_t Size)
{
  if (fd >=0) {
     ssize_t bytesWritten = safe_write(fd, Data, Size);
#ifdef USE_FADVISE
     if (bytesWritten > 0) {
        begin = min(begin, curpos);
        curpos += bytesWritten;
        written += bytesWritten;
        lastpos = max(lastpos, curpos);
        if (written > WRITE_BUFFER) {
           if (lastpos > begin) {
              // Now do three things:
              // 1) Start writeback of begin..lastpos range
              // 2) Drop the already written range (by the previous fadvise call)
              // 3) Handle nonpagealigned data.
              //    This is why we double the WRITE_BUFFER; the first time around the
              //    last (partial) page might be skipped, writeback will start only after
              //    second call; the third call will still include this page and finally
              //    drop it from cache.
              off_t headdrop = min(begin, off_t(WRITE_BUFFER * 2));
              posix_fadvise(fd, begin - headdrop, lastpos - begin + headdrop, POSIX_FADV_DONTNEED);
              }
           begin = lastpos = curpos;
           totwritten += written;
           written = 0;
           // The above fadvise() works when writing slowly (recording), but could
           // leave cached data around when writing at a high rate, e.g. when cutting,
           // because by the time we try to flush the cached pages (above) the data
           // can still be dirty - we are faster than the disk I/O.
           // So we do another round of flushing, just like above, but at larger
           // intervals -- this should catch any pages that couldn't be released
           // earlier.
           if (totwritten > MEGABYTE(32)) {
              // It seems in some setups, fadvise() does not trigger any I/O and
              // a fdatasync() call would be required do all the work (reiserfs with some
              // kind of write gathering enabled), but the syncs cause (io) load..
              // Uncomment the next line if you think you need them.
              //fdatasync(fd);
              off_t headdrop = min(off_t(curpos - totwritten), off_t(totwritten * 2));
              posix_fadvise(fd, curpos - totwritten - headdrop, totwritten + headdrop, POSIX_FADV_DONTNEED);
              totwritten = 0;
              }
           }
        }
#endif
     return bytesWritten;
     }
  return -1;
}

cUnbufferedFile *cUnbufferedFile::Create(const char *FileName, int Flags, mode_t Mode)
{
  cUnbufferedFile *File = new cUnbufferedFile;
  if (File->Open(FileName, Flags, Mode) < 0) {
     delete File;
     File = NULL;
     }
  return File;
}

// --- cReadDir --------------------------------------------------------------

cReadDir::cReadDir(const char *Directory)
{
  directory = opendir(Directory);
}

cReadDir::~cReadDir()
{
  if (directory)
    closedir(directory);
}

struct dirent *cReadDir::Next(void)
{
  return directory && readdir_r(directory, &u.d, &result) == 0 ? result : NULL;
}

// --- cReadLine -------------------------------------------------------------

cReadLine::cReadLine(void)
{
  size = 0;
  buffer = NULL;
}

cReadLine::~cReadLine()
{
  free(buffer);
}

char *cReadLine::Read(FILE *f)
{
  int n = getline(&buffer, &size, f);
  if (n > 0) {
     n--;
     if (buffer[n] == '\n') {
        buffer[n] = 0;
        if (n > 0) {
           n--;
           if (buffer[n] == '\r')
              buffer[n] = 0;
           }
        }
     return buffer;
     }
  return NULL;
}
