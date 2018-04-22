#pragma once

namespace SERVICES
{

//! \brief Base class for services.
class IService
{
public:
  virtual bool StartService() { return true; }
  virtual bool StopService() { return true; }
};

}
