
#include "Semaphore.h"
#ifdef __linux__
#include "SemaphorePOSIX.h"
#endif

CSemaphore::CSemaphore(uint32_t initialCount/*=1*/)
  : ISemaphore()
{
#ifdef _SEMAPHORE_H
  m_pSemaphore = new CSemaphorePOSIX(initialCount);
#else
#error No supported semaphore implementation available
#endif
}

CSemaphore::CSemaphore(const CSemaphore& sem)
  : ISemaphore()
{
#ifdef _SEMAPHORE_H
  m_pSemaphore = new CSemaphorePOSIX(sem.GetCount());
#else
#error No supported semaphore implementation available
#endif
}

CSemaphore::~CSemaphore()
{
  delete m_pSemaphore;
}

bool CSemaphore::Wait()
{
  return m_pSemaphore->Wait();
}

SEM_GRAB CSemaphore::TimedWait(uint32_t millis)
{
  return m_pSemaphore->TimedWait(millis);
}

bool CSemaphore::TryWait()
{
  return m_pSemaphore->TryWait();
}

bool CSemaphore::Post()
{
  return m_pSemaphore->Post();
}

int CSemaphore::GetCount() const
{
  return m_pSemaphore->GetCount();
}

