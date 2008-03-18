#pragma once
#include "IDirectory.h"

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
    bool IsShare(const CStdString& strPath) const;
    bool IsInShare(const CStdString& strPath) const;

    inline const CShare& operator [](const int index) const
    {
      return m_vecShares->at(index);
    }

    inline CShare& operator[](const int index)
    {
      return m_vecShares->at(index);
    }

    void GetShares(VECSHARES &shares) const;

    void AllowNonLocalShares(bool allow) { m_allowNonLocalShares = allow; };

  protected:
    void CacheThumbs(CFileItemList &items);

    VECSHARES* m_vecShares;
    bool       m_allowNonLocalShares;
  };
}
