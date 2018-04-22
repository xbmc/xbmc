#pragma once

#include "IService.h"
#include "threads/SingleLock.h"

#include <memory>
#include <vector>

namespace SERVICES
{

/*!
 \brief Singleton class for managing global services.
 \details Services are grouped in levels. The assumption is that
          the services are a LIFO stack, both for levels and within
          a given level.
 */
class CServiceManager : public CCriticalSection
{
public:
  static CServiceManager& GetInstance(); //!< Singleton accessor
  //! \brief Register a service.
  //! \param service The service to register
  //! \param level Level to register service at
  bool RegisterService(std::shared_ptr<IService> service, size_t level = 0);

  //! \brief Obtain a registered service
  //! \param level Hint for level of service (optional)
  template<class T>
  std::shared_ptr<T> GetService(int level = -1)
  {
    CSingleLock lock(*this);
    for (size_t i = (level > -1 ? level : 0); i < m_services.size(); ++i)
    {
      for (auto& it : m_services[i])
        if (typeid(*it) == typeid(T))
          return std::static_pointer_cast<T>(it);
    }

    return nullptr;
  }

  //! \brief Tear down all services at a given level.
  bool TearDown(size_t level);

protected:
  std::vector<std::vector<std::shared_ptr<IService>>> m_services; //!< Registered services
};

}
