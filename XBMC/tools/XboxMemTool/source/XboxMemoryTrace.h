
#pragma once

class CTraceElement
{
public:
  DWORD allocatedSize;
  DWORD addr;
  std::vector<DWORD> callStack;
};

class CXboxMemoryTrace
{
public:
  CXboxMemoryTrace();
  ~CXboxMemoryTrace();

  CXboxMemoryTrace& operator = (const CXboxMemoryTrace &ct);
  const CXboxMemoryTrace operator - (const CXboxMemoryTrace &ct);

  // actually data from xbmemdump
  bool ParseFromVectorData(std::vector<std::string>& result);
  int ParseTraceElementFromVectorData(std::vector<std::string>& result, int offset);
  
  void Reset();
  
public:

};
