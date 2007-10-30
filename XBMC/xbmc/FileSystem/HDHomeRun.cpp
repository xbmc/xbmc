#include "stdafx.h"
#include "../Util.h"

#include "HDHomeRun.h"

using namespace XFILE;
using namespace DIRECTORY;

class CUrlOptions 
  : public std::map<CStdString, CStdString>
{  
public:
  CUrlOptions(const CStdString& data)
  {    
    std::vector<CStdString> options;    
    CUtil::Tokenize(data, options, "&");
    for(std::vector<CStdString>::iterator it = options.begin();it != options.end(); it++)
    {
      CStdString name, value;
      int pos = it->find_first_of('=');
      if(pos != CStdString::npos)
      {
        name = it->substr(0, pos);
        value = it->substr(pos+1);
      }
      else
      {
        name = *it;
        value = "";
      }

      CUtil::UrlDecode(name);
      CUtil::UrlDecode(value);
      insert(value_type(name, value));
    }
  }  
};


// -------------------------------------------
// ---------------- Directory ----------------
// -------------------------------------------

CDirectoryHomeRun::CDirectoryHomeRun()
{
  m_dll.Load();
}

CDirectoryHomeRun::~CDirectoryHomeRun()
{
  m_dll.Unload();
}

bool CDirectoryHomeRun::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  if(!m_dll.IsLoaded())
    return false;

  CURL url(strPath);

  if(url.GetHostName().IsEmpty())
  {
    // no hostname, list all available devices
	  struct hdhomerun_discover_device_t result_list[64];
    int count = m_dll.discover_find_devices(HDHOMERUN_DEVICE_TYPE_TUNER, result_list, 64);
	  if (count < 0)
      return false;

    for(int i=0;i<count;i++)
    {
      CStdString device, ip;
      CFileItem *item;
      unsigned int ip_addr = result_list[i].ip_addr;

      device.Format("%x", result_list[i].device_id);
      ip.Format("%u.%u.%u.%u",
		    (unsigned int)(ip_addr >> 24) & 0xFF, (unsigned int)(ip_addr >> 16) & 0xFF,
		    (unsigned int)(ip_addr >> 8) & 0xFF, (unsigned int)(ip_addr >> 0) & 0xFF);

      item = new CFileItem("hdhomerun://" + device + "/tuner0/", true);
      item->SetLabel(device + "-0 On " + ip);
      item->SetLabelPreformated(true);
      items.Add(item);

      item = new CFileItem("hdhomerun://" + device + "/tuner1/", true);
      item->SetLabel(device + "-1 On " + ip);
      item->SetLabelPreformated(true);
      items.Add(item);
    }
    return true;
  }
  else
  {    
    hdhomerun_device_t* device = m_dll.device_create_from_str(url.GetHostName().c_str());
    if(!device)
      return false;

    m_dll.device_set_tuner_from_str(device, url.GetFileName().c_str());

    hdhomerun_tuner_status_t status;
    if(!m_dll.device_get_tuner_status(device, &status))
    {
      m_dll.device_destroy(device);
      return true;
    }

    CStdString label;
    if(status.signal_present)
      label.Format("Current Stream: N/A");
    else
      label.Format("Current Stream: Channel %s, SNR %d", status.channel, status.signal_to_noise_quality);

    CFileItem* item = new CFileItem("hdhomerun://" + url.GetHostName() + "/" + url.GetFileName(), false);
    CUtil::RemoveSlashAtEnd(item->m_strPath);
    item->SetLabel(label);
    item->SetLabelPreformated(true);
    items.Add(item);

    m_dll.device_destroy(device);
    return true;
  }

  return false;
}


// -------------------------------------------
// ------------------ File -------------------
// -------------------------------------------
CFileHomeRun::CFileHomeRun() 
{
  m_device = NULL;
  m_dll.Load();
}

CFileHomeRun::~CFileHomeRun()
{
  Close();
}

bool CFileHomeRun::Open(const CURL &url, bool bBinary)
{
  if(!bBinary)
    return false;
  
  if(!m_dll.IsLoaded())
    return false;
  
  m_device = m_dll.device_create_from_str(url.GetHostName().c_str());
  if(!m_device)
    return false;

  m_dll.device_set_tuner_from_str(m_device, url.GetFileName().c_str());

  CUrlOptions options(url.GetOptions().Mid(1));
  CUrlOptions::iterator it;

  if( (it = options.find("channel")) != options.end() )
    m_dll.device_set_tuner_channel(m_device, it->second.c_str());

  if( (it = options.find("program")) != options.end() )
    m_dll.device_set_tuner_program(m_device, it->second.c_str());

  // start streaming from selected device and tuner
  if( m_dll.device_stream_start(m_device) <= 0 )
    return false;

  return true;
}

unsigned int CFileHomeRun::Read(void* lpBuf, __int64 uiBufSize)
{
  unsigned int datasize;
  // for now, let it it time out after 5 seconds,
  // neither of the players can be forced to 
  // continue even if read return 0 as can happen
  // on live streams.
  DWORD timestamp = GetTickCount() + 5000;
  while(1) 
  {
    datasize = (unsigned int)min((unsigned int) uiBufSize,UINT_MAX);
    uint8_t* ptr = m_dll.device_stream_recv(m_device, datasize, &datasize);
    if(ptr)
    {
      memcpy(lpBuf, ptr, datasize);
      return datasize;
    }

    if(GetTickCount() > timestamp)
      return 0;

    Sleep(64);
  }
  return datasize;
}

void CFileHomeRun::Close()
{  
  if(m_device)
  {
    m_dll.device_stream_stop(m_device);
    m_dll.device_destroy(m_device);
    m_device = NULL;
  }
}