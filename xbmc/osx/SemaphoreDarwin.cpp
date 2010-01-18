
#include "SemaphoreDarwin.h"
#include <sys/stat.h>
#include <fcntl.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <ctime>
#include <CoreVideo/CVHostTime.h>

#define SEM_NAME_LEN (NAME_MAX-4)

CSemaphoreDarwin::CSemaphoreDarwin(uint32_t initialCount)
  : ISemaphore()
{
  m_szName            = new char[SEM_NAME_LEN];     

  snprintf(m_szName, SEM_NAME_LEN, "/xbmc-sem-%d-%" PRIu64,
      getpid(), CVGetCurrentHostTime());

  m_pSem = sem_open(m_szName, O_CREAT, 0600, initialCount);
}

CSemaphoreDarwin::~CSemaphoreDarwin()
{
  sem_close(m_pSem);
  sem_unlink(m_szName);
  delete [] m_szName;
}

bool CSemaphoreDarwin::Wait()
{
  return (0 == sem_wait(m_pSem));
}

SEM_GRAB CSemaphoreDarwin::TimedWait(uint32_t millis)
{
  uint64_t end = CVGetCurrentHostTime() + (millis * 1000000LL);
  do {
    if (0 == sem_trywait(m_pSem))
      return SEM_GRAB_SUCCESS;
    if (errno != EAGAIN)
      return SEM_GRAB_FAILED;
    usleep(1000);
  } while (CVGetCurrentHostTime() < end);

  return SEM_GRAB_TIMEOUT;
}

bool CSemaphoreDarwin::TryWait()
{
  return (0 == sem_trywait(m_pSem));
}

bool CSemaphoreDarwin::Post()
{
  return (0 == sem_post(m_pSem));
}

int CSemaphoreDarwin::GetCount() const
{
  int val;
  if (0 == sem_getvalue(m_pSem, &val))
    return val;
  return -1;
}

