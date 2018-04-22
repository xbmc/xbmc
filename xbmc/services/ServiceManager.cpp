#include "ServiceManager.h"

namespace SERVICES
{

CServiceManager& CServiceManager::GetInstance()
{
  static CServiceManager manager;
  return manager;
}

bool CServiceManager::RegisterService(std::shared_ptr<IService> service, size_t level)
{
  if (!service->StartService())
    return false;

  CSingleLock lock(*this);
  if (m_services.size() < level+1)
    m_services.resize(level+1);
  m_services[level].emplace_back(std::move(service));

  return true;
}

bool CServiceManager::TearDown(size_t level)
{
  if (level >= m_services.size())
    return false;

  CSingleLock lock(*this);
  size_t ok = 0;
  for (auto it = m_services[level].rbegin(); it != m_services[level].rend(); ++it, ++ok)
  {
    if (!(*it)->StopService())
      break;
    it->reset();
  }

  if (ok == m_services[level].size())
  {
    m_services[level].clear();
    return true;
  }

  m_services.erase(m_services.begin()+m_services.size()-ok, m_services.end());
  return false;
}

}
