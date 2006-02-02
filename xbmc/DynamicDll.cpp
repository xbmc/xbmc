#include "stdafx.h"
#include "DynamicDll.h"

DllDynamic::DllDynamic()
{
  m_dll=NULL;
  m_DelayUnload=true;
}

DllDynamic::DllDynamic(const CStdString& strDllName)
{
  m_strDllName=strDllName;
  m_dll=NULL;
  m_DelayUnload=true;
}

DllDynamic::~DllDynamic()
{
  Unload();
}

bool DllDynamic::Load()
{
  if (m_dll)
    return true;

  if (!(m_dll=CSectionLoader::LoadDLL(m_strDllName, m_DelayUnload, LoadSymbols())))
    return false;

  if (!ResolveExports())
  {
    CLog::Log(LOGERROR, "Unable to resolve exports from dll %s", m_strDllName.c_str());
    return false;
  }

  return true;
}

void DllDynamic::Unload()
{
  if(m_dll)
    CSectionLoader::UnloadDLL(m_strDllName);
  m_dll=NULL;
}

bool DllDynamic::CanLoad()
{
  return CFile::Exists(m_strDllName);
}

bool DllDynamic::EnableDelayedUnload(bool bOnOff)
{
  if (m_dll)
    return false;

  m_DelayUnload=bOnOff;

  return true;
}

bool DllDynamic::SetFile(const CStdString& strDllName)
{
  if (m_dll)
    return false;

  m_strDllName=strDllName;
  return true;
}
