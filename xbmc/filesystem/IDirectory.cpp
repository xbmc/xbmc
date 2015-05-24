/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIKeyboardFactory.h"
#include "URL.h"
#include "PasswordManager.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"

using namespace XFILE;

IDirectory::IDirectory(void)
{
  m_flags = DIR_FLAG_DEFAULTS;
}

IDirectory::~IDirectory(void)
{}

/*!
 \brief Test if file have an allowed extension, as specified with SetMask()
 \param strFile File to test
 \return \e true if file is allowed
 \note If extension is ".ifo", filename format must be "vide_ts.ifo" or
       "vts_##_0.ifo". If extension is ".dat", filename format must be
       "AVSEQ##(#).DAT", "ITEM###(#).DAT" or "MUSIC##(#).DAT".
 */
bool IDirectory::IsAllowed(const CURL& url) const
{
  if (m_strFileMask.empty())
    return true;

  // Check if strFile have an allowed extension
  if (!URIUtils::HasExtension(url, m_strFileMask))
    return false;

  // We should ignore all non dvd/vcd related ifo and dat files.
  if (URIUtils::HasExtension(url, ".ifo"))
  {
    std::string fileName = URIUtils::GetFileName(url);

    // Allow filenames of the form video_ts.ifo or vts_##_0.ifo
    
    return StringUtils::EqualsNoCase(fileName, "video_ts.ifo") ||
          (fileName.length() == 12 &&
           StringUtils::StartsWithNoCase(fileName, "vts_") &&
           StringUtils::EndsWithNoCase(fileName, "_0.ifo"));
  }
  
  if (URIUtils::HasExtension(url, ".dat"))
  {
    std::string fileName = URIUtils::GetFileName(url);

    // Allow filenames of the form AVSEQ##(#).DAT, ITEM###(#).DAT
    // and MUSIC##(#).DAT
    return (fileName.length() == 11 || fileName.length() == 12) &&
           (StringUtils::StartsWithNoCase(fileName, "AVSEQ") ||
            StringUtils::StartsWithNoCase(fileName, "MUSIC") ||
            StringUtils::StartsWithNoCase(fileName, "ITEM"));
  }

  return true;
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
void IDirectory::SetMask(const std::string& strMask)
{
  m_strFileMask = strMask;
  // ensure it's completed with a | so that filtering is easy.
  StringUtils::ToLower(m_strFileMask);
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
  std::string type = m_requirements["type"].asString();
  if (type == "keyboard")
  {
    std::string input;
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

bool IDirectory::GetKeyboardInput(const CVariant &heading, std::string &input)
{
  if (!m_requirements["input"].asString().empty())
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

void IDirectory::RequireAuthentication(const CURL &url)
{
  m_requirements.clear();
  m_requirements["type"] = "authenticate";
  m_requirements["url"] = url.Get();
}
