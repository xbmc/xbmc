
#pragma once

class CPdbParser;

class CModule
{
public:
  CModule()
  {
    this->name = "";
    this->signature = 0;
    this->pParser = NULL;
    this->loadAddress = 0;
    this->size = 0;
  }
  
  std::string name;
  DWORD signature; // signature
  DWORD loadAddress; // start address of the loaded module in memory
  DWORD size; // size of the loaded module
  CPdbParser* pParser;
};