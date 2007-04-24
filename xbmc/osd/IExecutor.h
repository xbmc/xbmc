#pragma once
namespace OSD
{
class IOSDOption;

class IExecutor
{
public:
  IExecutor(){};
  virtual ~IExecutor(void){};
  virtual void OnExecute(int iAction, const IOSDOption* option) = 0;
};
};
