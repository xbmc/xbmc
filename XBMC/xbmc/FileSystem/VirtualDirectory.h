#pragma once
#include "../fileitem.h"
#include "../settings.h"
#include "directory.h"

#include <string>
#include "stdstring.h"
#include <vector>
using namespace std;

namespace DIRECTORY
{

	class CVirtualDirectory : public CDirectory
  {
  public:
    CVirtualDirectory(void);
    virtual ~CVirtualDirectory(void);
    virtual bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items);
    void          SetShares(VECSHARES& vecShares);
		bool					IsShare(const CStdString& strPath) const;
		CStdString		GetDVDDriveUrl();
  
  protected:
    void          CacheThumbs(VECFILEITEMS &items);
    VECSHARES*     m_vecShares;
  };
};