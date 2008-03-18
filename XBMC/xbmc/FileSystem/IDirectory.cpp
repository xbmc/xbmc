
#include "stdafx.h"
#include "IDirectory.h"
#include "../Util.h"

using namespace DIRECTORY;

IDirectory::IDirectory(void)
{
  m_strFileMask = "";
  m_allowPrompting = false;
  m_cacheDirectory = false;
}

IDirectory::~IDirectory(void)
{}

/*!
 \brief Test a file for an extension specified with SetMask().
 \param strFile File to test
 \return Returns \e true, if file is allowed.
 */
bool IDirectory::IsAllowed(const CStdString& strFile)
{
  CStdString strExtension;
  if ( !m_strFileMask.size() ) return true;
  if ( !strFile.size() ) return true;

  CUtil::GetExtension(strFile, strExtension);
  if (!strExtension.size()) return false;
  strExtension.ToLower();
  bool bOkay = false;
  int i=-1;
  while (!bOkay)
  {
    i = m_strFileMask.Find(strExtension,i+1);
    if (i >= 0)
    {
      if (i+strExtension.size() == m_strFileMask.size())
        bOkay = true;
      else
      {
        char c = m_strFileMask[i+strExtension.size()];
        if (c == '|')
          bOkay = true;
        else
          bOkay = false;
      }
    }    
    else
      break;
  }
  if ( i >= 0 && bOkay)
  {
    return true;
  }
  return false;
}

/*!
 \brief Set a mask of extensions for the files in the directory.
 \param strMask Mask of file extensions that are allowed.
 
 The mask has to look like the following: \n
 \verbatim
 .m4a|.flac|.aac|
 \endverbatim
 So only *.m4a, *.flac, *.aac files will be retrieved with GetDirectory().
 */
void IDirectory::SetMask(const CStdString& strMask)
{
  m_strFileMask = strMask;
  m_strFileMask.ToLower();
}

/*!
 \brief Set whether the directory handlers can prompt the user.
 \param allowPrompting Set true to allow prompting to occur (default is false).
 
 Directory handlers should only prompt the user as a direct result of the
 users actions.
 */

void IDirectory::SetAllowPrompting(bool allowPrompting)
{
  m_allowPrompting = allowPrompting;
}

/*!
 \brief Set whether the directory should be cached by our directory cache.
 \param cacheDirectory Set true to enable caching (default is false).
 */

void IDirectory::SetCacheDirectory(bool cacheDirectory)
{
  m_cacheDirectory = cacheDirectory;
}

