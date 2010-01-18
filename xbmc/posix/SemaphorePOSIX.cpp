
#include "SemaphorePOSIX.h"
#include <sys/stat.h>
#include <fcntl.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <ctime>

#define SEM_NAME_LEN (NAME_MAX-4)

CSemaphorePOSIX::CSemaphorePOSIX(uint32_t initialCount)
  : ISemaphore()
{
  struct timespec now = {};
  m_szName            = new char[SEM_NAME_LEN];

  clock_gettime(CLOCK_MONOTONIC, &now);

  snprintf(m_szName, SEM_NAME_LEN, "/xbmc-sem-%d-%" PRIu64 "-%" PRIu64,
      getpid(), (uint64_t)now.tv_sec, (uint64_t)now.tv_nsec);

  m_pSem = sem_open(m_szName, O_CREAT, 0600, initialCount);
}

CSemaphorePOSIX::~CSemaphorePOSIX()
{
  sem_close(m_pSem);
  sem_unlink(m_szName);
  delete [] m_szName;
}

bool CSemaphorePOSIX::Wait()
{
  return (0 == sem_wait(m_pSem));
}

SEM_GRAB CSemaphorePOSIX::TimedWait(uint32_t millis)
{
  struct timespec to =
  {
    millis / 1000,
    (millis % 1000) * 1000000
  };

  if (0 == sem_timedwait(m_pSem, &to))
    return SEM_GRAB_SUCCESS;

  if (errno == ETIMEDOUT)
    return SEM_GRAB_TIMEOUT;

  return SEM_GRAB_FAILED;
}

bool CSemaphorePOSIX::TryWait()
{
  return (0 == sem_trywait(m_pSem));
}

bool CSemaphorePOSIX::Post()
{
  return (0 == sem_post(m_pSem));
}

int CSemaphorePOSIX::GetCount() const
{
  int val;
  if (0 == sem_getvalue(m_pSem, &val))
    return val;
  return -1;
}

