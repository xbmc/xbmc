/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#include "IDirectory.h"
#include "Util.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIKeyboardFactory.h"
#include "URL.h"
#include "PasswordManager.h"
#include "utils/URIUtils.h"

using namespace XFILE;

IDirectory::IDirectory(void)
{
  m_strFileMask = "";
  m_flags = DIR_FLAG_DEFAULTS;
}

IDirectory::~IDirectory(void)
{}

/*!
 \brief Test a file for an extension specified with SetMask().
 \param strFile File to test
 \return Returns \e true, if file is allowed.
 */
bool IDirectory::IsAllowed(const CStdString& strFile) const
{
  CStdString strExtension;
  if ( !m_strFileMask.size() ) return true;
  if ( !strFile.size() ) return true;

  URIUtils::GetExtension(strFile, strExtension);

  if (!strExtension.size()) return false;

  strExtension.ToLower();
  strExtension += '|'; // ensures that we have a | at the end of it
  if (m_strFileMask.Find(strExtension) != -1)
  { // it's allowed, but we should also ignore all non dvd related ifo files.
    if (strExtension.Equals(".ifo|"))
    {
      CStdString fileName = URIUtils::GetFileName(strFile);
      if (fileName.Equals("video_ts.ifo")) return true;
      if (fileName.length() == 12 && fileName.Left(4).Equals("vts_") && fileName.Right(6).Equals("_0.ifo")) return true;
      return false;
    }
    if (strExtension.Equals(".dat|"))
    {
      CStdString fileName = URIUtils::GetFileName(strFile);
      /* VCD filenames are of the form AVSEQ##(#).DAT, ITEM###(#).DAT, MUSIC##(#).DAT - i.e. all 11 or 12 characters long
         starting with AVSEQ, MUSIC or ITEM */
      if ((fileName.length() == 11 || fileName.length() == 12) &&
          (fileName.Left(5).Equals("AVSEQ") || fileName.Left(5).Equals("MUSIC") || fileName.Left(4).Equals("ITEM")))
        return true;
      return false;
    }
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
  // ensure it's completed with a | so that filtering is easy.
  m_strFileMask.ToLower();
  if (m_strFileMask.size() && m_strFileMask[m_strFileMask.size() - 1] != '|')
    m_strFileMask += '|';
}

/*!
 \brief Set the flags for this directory handler.
 \param flags - \sa XFILE::DIR_FLAG for a description.
 */
void IDirectory::SetFlags(int flags)
{
  m_flags = flags;
}

bool IDirectory::ProcessRequirements()
{
  CStdString type = m_requirements["type"].asString();
  if (type == "keyboard")
  {
    CStdString input;
    if (CGUIKeyboardFactory::ShowAndGetInput(input, m_requirements["heading"], false))
    {
      m_requirements["input"] = input;
      return true;
    }
  }
  else if (type == "authenticate")
  {
    CURL url(m_requirements["url"].asString());
    if (CPasswordManager::GetInstance().PromptToAuthenticateURL(url))
    {
      m_requirements.clear();
      return true;
    }
  }
  else if (type == "error")
  {
    CGUIDialogOK::ShowAndGetInput(m_requirements["heading"], m_requirements["line1"], m_requirements["line2"], m_requirements["line3"]);
  }
  m_requirements.clear();
  return false;
}

bool IDirectory::GetKeyboardInput(const CVariant &heading, CStdString &input)
{
  if (!CStdString(m_requirements["input"].asString()).IsEmpty())
  {
    input = m_requirements["input"].asString();
    return true;
  }
  m_requirements.clear();
  m_requirements["type"] = "keyboard";
  m_requirements["heading"] = heading;
  return false;
}

void IDirectory::SetErrorDialog(const CVariant &heading, const CVariant &line1, const CVariant &line2, const CVariant &line3)
{
  m_requirements.clear();
  m_requirements["type"] = "error";
  m_requirements["heading"] = heading;
  m_requirements["line1"] = line1;
  m_requirements["line2"] = line2;
  m_requirements["line3"] = line3;
}

void IDirectory::RequireAuthentication(const CStdString &url)
{
  m_requirements.clear();
  m_requirements["type"] = "authenticate";
  m_requirements["url"] = url;
}
