#pragma once
#include "../fileitem.h"

namespace DIRECTORY
{
  class CDirectory
  {
  public:
    CDirectory(void);
    virtual ~CDirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items)=0;
		
		bool	IsAllowed(const CStdString& strFile);
		void  SetMask(const CStdString& strMask);
	protected:
		CStdString	m_strFileMask;
  };

}