#pragma once
#include "IDirectory.h"
#include "Settings.h"

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
    void SetSources(VECSOURCES& vecSources);
    inline unsigned int GetNumberOfSources() { 
      if (m_vecSources)
        return m_vecSources->size(); 
      else
        return 0;
      }
    bool IsSource(const CStdString& strPath) const;
    bool IsInSource(const CStdString& strPath) const;

    inline const CMediaSource& operator [](const int index) const
    {
      return m_vecSources->at(index);
    }

    inline CMediaSource& operator[](const int index)
    {
      return m_vecSources->at(index);
    }

    void GetSources(VECSOURCES &sources) const;

    void AllowNonLocalSources(bool allow) { m_allowNonLocalSources = allow; };

  protected:
    void CacheThumbs(CFileItemList &items);

    VECSOURCES* m_vecSources;
    bool       m_allowNonLocalSources;
  };
}
