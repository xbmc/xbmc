/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IDirectory.h"

#include "PasswordManager.h"
#include "URL.h"
#include "guilib/GUIKeyboardFactory.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace KODI::MESSAGING;
using namespace XFILE;

const CProfileManager *IDirectory::m_profileManager = nullptr;

void IDirectory::RegisterProfileManager(const CProfileManager &profileManager)
{
  m_profileManager = &profileManager;
}

void IDirectory::UnregisterProfileManager()
{
  m_profileManager = nullptr;
}

IDirectory::IDirectory()
{
  m_flags = DIR_FLAG_DEFAULTS;
}

IDirectory::~IDirectory(void) = default;

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
    std::string folder = URIUtils::GetDirectory(fileName);
    URIUtils::RemoveSlashAtEnd(folder);
    folder = URIUtils::GetFileName(folder);
    if (StringUtils::EqualsNoCase(folder, "vcd") ||
        StringUtils::EqualsNoCase(folder, "mpegav") ||
        StringUtils::EqualsNoCase(folder, "cdda"))
    {
      // Allow filenames of the form AVSEQ##(#).DAT, ITEM###(#).DAT
      // and MUSIC##(#).DAT
      return (fileName.length() == 11 || fileName.length() == 12) &&
             (StringUtils::StartsWithNoCase(fileName, "AVSEQ") ||
              StringUtils::StartsWithNoCase(fileName, "MUSIC") ||
              StringUtils::StartsWithNoCase(fileName, "ITEM"));
    }
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
    if (CGUIKeyboardFactory::ShowAndGetInput(input, m_requirements["heading"], false, m_requirements["hidden"].asBoolean()))
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
    HELPERS::ShowOKDialogLines(CVariant{m_requirements["heading"]}, CVariant{m_requirements["line1"]}, CVariant{m_requirements["line2"]}, CVariant{m_requirements["line3"]});
  }
  m_requirements.clear();
  return false;
}

bool IDirectory::GetKeyboardInput(const CVariant &heading, std::string &input, bool hiddenInput)
{
  if (!m_requirements["input"].asString().empty())
  {
    input = m_requirements["input"].asString();
    return true;
  }
  m_requirements.clear();
  m_requirements["type"] = "keyboard";
  m_requirements["heading"] = heading;
  m_requirements["hidden"] = hiddenInput;
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
