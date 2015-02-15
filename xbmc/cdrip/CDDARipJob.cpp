/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "CDDARipJob.h"
#include "Encoder.h"
#include "EncoderFFmpeg.h"
#include "FileItem.h"
#include "utils/log.h"
#include "utils/SystemInfo.h"
#include "Util.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "storage/MediaManager.h"
#include "addons/AddonManager.h"
#include "addons/AudioEncoder.h"

using namespace ADDON;
using namespace MUSIC_INFO;
using namespace XFILE;

CCDDARipJob::CCDDARipJob(const std::string& input,
                         const std::string& output,
                         const CMusicInfoTag& tag, 
                         int encoder,
                         bool eject,
                         unsigned int rate,
                         unsigned int channels, unsigned int bps) : 
  m_rate(rate), m_channels(channels), m_bps(bps), m_tag(tag),
  m_input(input), m_output(CUtil::MakeLegalPath(output)), m_eject(eject),
  m_encoder(encoder)
{
}

CCDDARipJob::~CCDDARipJob()
{
}

bool CCDDARipJob::DoWork()
{
  CLog::Log(LOGINFO, "Start ripping track %s to %s", m_input.c_str(),
                                                     m_output.c_str());

  // if we are ripping to a samba share, rip it to hd first and then copy it it the share
  CFileItem file(m_output, false);
  if (file.IsRemote())
    m_output = SetupTempFile();
  
  if (m_output.empty())
  {
    CLog::Log(LOGERROR, "CCDDARipper: Error opening file");
    return false;
  }

  // init ripper
  CFile reader;
  CEncoder* encoder;
  if (!reader.Open(m_input,READ_CACHED) || !(encoder=SetupEncoder(reader)))
  {
    CLog::Log(LOGERROR, "Error: CCDDARipper::Init failed");
    return false;
  }

  // setup the progress dialog
  CGUIDialogExtendedProgressBar* pDlgProgress = 
      (CGUIDialogExtendedProgressBar*)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
  CGUIDialogProgressBarHandle* handle = pDlgProgress->GetHandle(g_localizeStrings.Get(605));

  int iTrack = atoi(m_input.substr(13, m_input.size() - 13 - 5).c_str());
  std::string strLine0 = StringUtils::Format("%02i. %s - %s", iTrack,
                                            StringUtils::Join(m_tag.GetArtist(), g_advancedSettings.m_musicItemSeparator).c_str(),
                                            m_tag.GetTitle().c_str());
  handle->SetText(strLine0);

  // start ripping
  int percent=0;
  int oldpercent=0;
  bool cancelled(false);
  int result;
  while (!cancelled && (result=RipChunk(reader, encoder, percent)) == 0)
  {
    cancelled = ShouldCancel(percent,100);
    if (percent > oldpercent)
    {
      oldpercent = percent;
      handle->SetPercentage(static_cast<float>(percent));
    }
  }

  // close encoder ripper
  encoder->CloseEncode();
  delete encoder;
  reader.Close();

  if (file.IsRemote() && !cancelled && result == 2)
  {
    // copy the ripped track to the share
    if (!CFile::Copy(m_output, file.GetPath()))
    {
      CLog::Log(LOGERROR, "CDDARipper: Error copying file from %s to %s", 
                m_output.c_str(), file.GetPath().c_str());
      CFile::Delete(m_output);
      return false;
    }
    // delete cached file
    CFile::Delete(m_output);
  }

  if (cancelled)
  {
    CLog::Log(LOGWARNING, "User Cancelled CDDA Rip");
    CFile::Delete(m_output);
  }
  else if (result == 1)
    CLog::Log(LOGERROR, "CDDARipper: Error ripping %s", m_input.c_str());
  else if (result < 0)
    CLog::Log(LOGERROR, "CDDARipper: Error encoding %s", m_input.c_str());
  else
  {
    CLog::Log(LOGINFO, "Finished ripping %s", m_input.c_str());
    if (m_eject)
    {
      CLog::Log(LOGINFO, "Ejecting CD");
      g_mediaManager.EjectTray();
    }
  }

  handle->MarkFinished();

  return !cancelled && result == 2;
}

int CCDDARipJob::RipChunk(CFile& reader, CEncoder* encoder, int& percent)
{
  percent = 0;

  uint8_t stream[1024];

  // get data
  int result = reader.Read(stream, 1024);

  // return if rip is done or on some kind of error
  if (result <= 0)
    return 1;

  // encode data
  int encres=encoder->Encode(result, stream);

  // Get progress indication
  percent = static_cast<int>(reader.GetPosition()*100/reader.GetLength());

  if (reader.GetPosition() == reader.GetLength())
    return 2;

  return -(1-encres);
}

CEncoder* CCDDARipJob::SetupEncoder(CFile& reader)
{
  CEncoder* encoder = NULL;
  if (CSettings::Get().GetString("audiocds.encoder") == "audioencoder.xbmc.builtin.aac" ||
           CSettings::Get().GetString("audiocds.encoder") == "audioencoder.xbmc.builtin.wma")
  {
    std::shared_ptr<IEncoder> enc(new CEncoderFFmpeg());
    encoder = new CEncoder(enc);
  }
  else
  {
    AddonPtr addon;
    CAddonMgr::Get().GetAddon(CSettings::Get().GetString("audiocds.encoder"), addon);
    if (addon)
    {
      std::shared_ptr<CAudioEncoder> aud =  std::static_pointer_cast<CAudioEncoder>(addon);
      aud->Create();
      std::shared_ptr<IEncoder> enc =  std::static_pointer_cast<IEncoder>(aud);
      encoder = new CEncoder(enc);
    }
  }
  if (!encoder)
    return NULL;

  // we have to set the tags before we init the Encoder
  std::string strTrack = StringUtils::Format("%li", strtol(m_input.substr(13, m_input.size() - 13 - 5).c_str(),NULL,10));

  encoder->SetComment(std::string("Ripped with ") + CSysInfo::GetAppName());
  encoder->SetArtist(StringUtils::Join(m_tag.GetArtist(),
                                      g_advancedSettings.m_musicItemSeparator));
  encoder->SetTitle(m_tag.GetTitle());
  encoder->SetAlbum(m_tag.GetAlbum());
  encoder->SetAlbumArtist(StringUtils::Join(m_tag.GetAlbumArtist(),
                                      g_advancedSettings.m_musicItemSeparator));
  encoder->SetGenre(StringUtils::Join(m_tag.GetGenre(),
                                      g_advancedSettings.m_musicItemSeparator));
  encoder->SetTrack(strTrack);
  encoder->SetTrackLength(static_cast<int>(reader.GetLength()));
  encoder->SetYear(m_tag.GetYearString());

  // init encoder
  if (!encoder->Init(m_output.c_str(), m_channels, m_rate, m_bps))
    delete encoder, encoder = NULL;

  return encoder;
}

std::string CCDDARipJob::SetupTempFile()
{
  char tmp[MAX_PATH];
#ifndef TARGET_POSIX
  GetTempFileName(CSpecialProtocol::TranslatePath("special://temp/").c_str(), "riptrack", 0, tmp);
#else
  int fd;
  strncpy(tmp, CSpecialProtocol::TranslatePath("special://temp/riptrackXXXXXX").c_str(), MAX_PATH);
  if ((fd = mkstemp(tmp)) == -1)
   tmp[0] = '\0'; 
  if (fd != -1)
    close(fd);
#endif
  return tmp;
}

bool CCDDARipJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CCDDARipJob* rjob = dynamic_cast<const CCDDARipJob*>(job);
    if (rjob)
    {
      return m_input  == rjob->m_input &&
             m_output == rjob->m_output;
    }
  }
  return false;
}
