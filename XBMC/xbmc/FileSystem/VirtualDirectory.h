#pragma once
#include "../fileitem.h"
#include "idirectory.h"

namespace DIRECTORY
{

	/*!
		\ingroup windows 
		\brief Get access to shares and it's directories.
		*/
	class CVirtualDirectory : public IDirectory
  {
  public:
    CVirtualDirectory(void);
    virtual ~CVirtualDirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,CFileItemList &items);
    void          SetShares(VECSHARES& vecShares);
		bool					IsShare(const CStdString& strPath) const;
		CStdString		GetDVDDriveUrl();
  
  protected:
    void          CacheThumbs(CFileItemList &items);
    VECSHARES*    m_vecShares;
  };
};