
#include "..\stdafx.h"
#include "XboxMemoryTrace.h"



CXboxMemoryTrace::CXboxMemoryTrace()
{
}

CXboxMemoryTrace::~CXboxMemoryTrace()
{
}



void CXboxMemoryTrace::Reset()
{
  m_parsed.clear();
}


bool CXboxMemoryTrace::ParseFromVectorData(std::vector<std::string>& result)
{

}

int CXboxMemoryTrace::ParseTraceElementFromVectorData(std::vector<std::string>& result, int offset)
{

}