#pragma once
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
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items, bool bUseFileDirectories); 
    void SetShares(VECSHARES& vecShares);
    inline unsigned int GetNumberOfShares() { 
      if (m_vecShares)
        return m_vecShares->size(); 
      else
        return 0;
      }
    void AddShare(const CShare& share);
    bool RemoveShare(const CStdString& strPath);
    bool RemoveShareName(const CStdString& strName);
    bool IsShare(const CStdString& strPath) const;
    CStdString GetDVDDriveUrl();

    inline const CShare& operator [](const int index) const
    {
      return m_vecShares->at(index);
    }

    inline CShare& operator[](const int index)
    {
      return m_vecShares->at(index);
    }

  protected:
    void CacheThumbs(CFileItemList &items);
    VECSHARES* m_vecShares;
  };
};
