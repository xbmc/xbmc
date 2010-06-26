/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Repository.h"
#include "tinyXML/tinyxml.h"
#include "FileSystem/File.h"
#include "XMLUtils.h"
#include "AddonDatabase.h"
#include "Application.h"
#include "Settings.h"
#include "FileItem.h"
#include "utils/JobManager.h"
#include "utils/FileOperationJob.h"
#include "GUIWindowManager.h"
#include "GUIWindowAddonBrowser.h"
#include "GUIDialogYesNo.h"

using namespace XFILE;
using namespace ADDON;

AddonPtr CRepository::Clone(const AddonPtr &self) const
{
  CRepository* result = new CRepository(*this, self);
  result->m_info = m_info;
  result->m_checksum = m_checksum;
  result->m_datadir = m_datadir;
  result->m_compressed = m_compressed;
  result->m_zipped = m_zipped;
  return AddonPtr(result);
}

CRepository::CRepository(const AddonProps& props) :
  CAddon(props)
{
  m_compressed = false;
  m_zipped = false;
}

CRepository::CRepository(const cp_extension_t *ext)
  : CAddon(ext)
{
  m_compressed = false;
  m_zipped = false;
  // read in the other props that we need
  if (ext)
  {
    m_checksum = CAddonMgr::Get().GetExtValue(ext->configuration, "checksum");
    m_compressed = CAddonMgr::Get().GetExtValue(ext->configuration, "info@compressed").Equals("true");
    m_info = CAddonMgr::Get().GetExtValue(ext->configuration, "info");
    m_datadir = CAddonMgr::Get().GetExtValue(ext->configuration, "datadir");
    m_zipped = CAddonMgr::Get().GetExtValue(ext->configuration, "datadir@zip").Equals("true");
  }
}

CRepository::CRepository(const CRepository &rhs, const AddonPtr &self)
  : CAddon(rhs, self)
{
}

CRepository::~CRepository()
{
}

CStdString CRepository::Checksum()
{
  CSingleLock lock(m_critSection);
  CFile file;
  file.Open(m_checksum);
  CStdString checksum;
  try
  {
    char* temp = new char[(size_t)file.GetLength()+1];
    file.Read(temp,file.GetLength());
    temp[file.GetLength()] = 0;
    checksum = temp;
    delete[] temp;
  }
  catch (...)
  {
  }
  return checksum;
}

VECADDONS CRepository::Parse()
{
  CSingleLock lock(m_critSection);

  VECADDONS result;
  TiXmlDocument doc;

  CStdString file = m_info;
  if (m_compressed)
  {
    CURL url(m_info);
    CStdString opts = url.GetProtocolOptions();
    if (!opts.IsEmpty())
      opts += "&";
    url.SetProtocolOptions(opts+"Encoding=gzip");
    file = url.Get();
  }

  doc.LoadFile(file);
  if (doc.RootElement())
  {
    CAddonMgr::Get().AddonsFromRepoXML(doc.RootElement(), result);
    for (IVECADDONS i = result.begin(); i != result.end(); ++i)
    {
      AddonPtr addon = *i;
      if (m_zipped)
      {
        addon->Props().path = CUtil::AddFileToFolder(m_datadir,addon->ID()+"/"+addon->ID()+"-"+addon->Version().str+".zip");
        addon->Props().icon = CUtil::AddFileToFolder(m_datadir,addon->ID()+"/icon.png");
        addon->Props().changelog = CUtil::AddFileToFolder(m_datadir,addon->ID()+"/changelog-"+addon->Version().str+".txt");
        addon->Props().fanart = CUtil::AddFileToFolder(m_datadir,addon->ID()+"/fanart.jpg");
      }
      else
      {
        addon->Props().path = CUtil::AddFileToFolder(m_datadir,addon->ID()+"/");
        addon->Props().changelog = CUtil::AddFileToFolder(m_datadir,addon->ID()+"/changelog.txt");
        addon->Props().fanart = CUtil::AddFileToFolder(m_datadir,addon->ID()+"/fanart.jpg");
      }
    }
  }

  return result;
}

CRepositoryUpdateJob::CRepositoryUpdateJob(RepositoryPtr& repo, bool check)
{
  m_repo = boost::dynamic_pointer_cast<CRepository>(repo->Clone(repo));
}

bool CRepositoryUpdateJob::DoWork()
{
  VECADDONS addons = GrabAddons(m_repo,true);
  if (addons.empty())
    return false;

  // check for updates
  for (unsigned int i=0;i<addons.size();++i)
  {
    AddonPtr addon;
    CAddonMgr::Get().GetAddon(addons[i]->ID(),addon);
    if (addon && addons[i]->Version() > addon->Version())
    {
      if (g_settings.m_bAddonAutoUpdate || addon->Type() >= ADDON_VIZ_LIBRARY)
      {
        CGUIWindowAddonBrowser::AddJob(addons[i]->Path());
      }
      else
      {
        g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Info,
                                                          g_localizeStrings.Get(24061),
                                                          addon->Name(),TOAST_DISPLAY_TIME,false);
      }
    }
    if (addon && !addons[i]->Props().broken.IsEmpty())
    {
      CAddonDatabase database;
      database.Open();
      if (database.IsAddonBroken(addon->ID()).IsEmpty())
      {
        if (CGUIDialogYesNo::ShowAndGetInput(addon->Name(),
                                             g_localizeStrings.Get(24096),
                                             g_localizeStrings.Get(24097),
                                             ""))
          database.DisableAddon(addon->ID());

        database.BreakAddon(addon->ID(),true,addons[i]->Props().broken);
      }
    }
  }

  return true;
}

VECADDONS CRepositoryUpdateJob::GrabAddons(RepositoryPtr& repo,
                                           bool check)
{
  CAddonDatabase database;
  database.Open();
  CStdString checksum;
  int idRepo = database.GetRepoChecksum(repo->ID(),checksum);
  CStdString reposum=checksum;
  if (check)
    reposum = repo->Checksum();
  VECADDONS addons;
  if (idRepo == -1 || !checksum.Equals(reposum))
  {
    addons = repo->Parse();
    database.AddRepository(repo->ID(),addons,reposum);
  }
  else
  {
    database.GetRepository(repo->ID(),addons);
    database.SetRepoTimestamp(repo->ID(),CDateTime::GetCurrentDateTime().GetAsDBDateTime());
  }

  return addons;
}

