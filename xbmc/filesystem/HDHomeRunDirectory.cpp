/*
 *      Copyright (C) 2011-2012 Team XBMC
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
 
#include "HDHomeRunDirectory.h"
#include "URL.h"
#include "FileItem.h"
#include "utils/URIUtils.h"
#include "DllHDHomeRun.h"

using namespace XFILE;
using namespace std;

// -------------------------------------------
// ---------------- Directory ----------------
// -------------------------------------------

CHomeRunDirectory::CHomeRunDirectory()
{
  m_pdll = new DllHdHomeRun;
  m_pdll->Load();
}

CHomeRunDirectory::~CHomeRunDirectory()
{
  m_pdll->Unload();
  delete m_pdll;
}

bool CHomeRunDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  if(!m_pdll->IsLoaded())
    return false;

  CURL url(strPath);

  if(url.GetHostName().IsEmpty())
  {
    // no hostname, list all available devices
    int target_ip = 0;
    struct hdhomerun_discover_device_t result_list[64];
    int count = m_pdll->discover_find_devices_custom(target_ip, HDHOMERUN_DEVICE_TYPE_TUNER, HDHOMERUN_DEVICE_ID_WILDCARD, result_list, 64);
    if (count < 0)
      return false;

    for(int i=0;i<count;i++)
    {
      CStdString device, ip;
      CFileItemPtr item;
      unsigned int ip_addr = result_list[i].ip_addr;

      device.Format("%x", result_list[i].device_id);
      ip.Format("%u.%u.%u.%u",
            (unsigned int)(ip_addr >> 24) & 0xFF, (unsigned int)(ip_addr >> 16) & 0xFF,
            (unsigned int)(ip_addr >> 8) & 0xFF, (unsigned int)(ip_addr >> 0) & 0xFF);

      item.reset(new CFileItem("hdhomerun://" + device + "/tuner0/", true));
      item->SetLabel(device + "-0 On " + ip);
      item->SetLabelPreformated(true);
      items.Add(item);

      item.reset(new CFileItem("hdhomerun://" + device + "/tuner1/", true));
      item->SetLabel(device + "-1 On " + ip);
      item->SetLabelPreformated(true);
      items.Add(item);
    }
    return true;
  }
  else
  {
    hdhomerun_device_t* device = m_pdll->device_create_from_str(url.GetHostName().c_str(), NULL);
    if(!device)
      return false;

    m_pdll->device_set_tuner_from_str(device, url.GetFileName().c_str());

    hdhomerun_tuner_status_t status;
    if(!m_pdll->device_get_tuner_status(device, NULL, &status))
    {
      m_pdll->device_destroy(device);
      return true;
    }

    CStdString label;
    if(status.signal_present)
      label.Format("Current Stream: N/A");
    else
      label.Format("Current Stream: Channel %s, SNR %d", status.channel, status.signal_to_noise_quality);

    CStdString path = "hdhomerun://" + url.GetHostName() + "/" + url.GetFileName();
    URIUtils::RemoveSlashAtEnd(path);
    CFileItemPtr item(new CFileItem(path, false));
    item->SetLabel(label);
    item->SetLabelPreformated(true);
    items.Add(item);

    m_pdll->device_destroy(device);
    return true;
  }

  return false;
}
