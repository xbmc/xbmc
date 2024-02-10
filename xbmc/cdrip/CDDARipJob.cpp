/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CDDARipJob.h"

#include "Encoder.h"
#include "EncoderAddon.h"
#include "EncoderFFmpeg.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "network/NetworkFileItemClassify.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"

#if defined(TARGET_WINDOWS)
#include "platform/win32/CharsetConverter.h"
#endif

using namespace ADDON;
using namespace MUSIC_INFO;
using namespace XFILE;
using namespace KODI;
using namespace KODI::CDRIP;

CCDDARipJob::CCDDARipJob(const std::string& input,
                         const std::string& output,
                         const CMusicInfoTag& tag,
                         int encoder,
                         bool eject,
                         unsigned int rate,
                         unsigned int channels,
                         unsigned int bps)
  : m_rate(rate),
    m_channels(channels),
    m_bps(bps),
    m_tag(tag),
    m_input(input),
    m_output(CUtil::MakeLegalPath(output)),
    m_eject(eject),
    m_encoder(encoder)
{
}

CCDDARipJob::~CCDDARipJob() = default;

bool CCDDARipJob::DoWork()
{
  CLog::Log(LOGINFO, "CCDDARipJob::{} - Start ripping track {} to {}", __func__, m_input, m_output);

  // if we are ripping to a samba share, rip it to hd first and then copy it to the share
  CFileItem file(m_output, false);
  if (NETWORK::IsRemote(file))
    m_output = SetupTempFile();

  if (m_output.empty())
  {
    CLog::Log(LOGERROR, "CCDDARipJob::{} - Error opening file", __func__);
    return false;
  }

  // init ripper
  CFile reader;
  std::unique_ptr<CEncoder> encoder{};
  if (!reader.Open(m_input, READ_CACHED) || !(encoder = SetupEncoder(reader)))
  {
    CLog::Log(LOGERROR, "CCDDARipJob::{} - Opening failed", __func__);
    return false;
  }

  // setup the progress dialog
  CGUIDialogExtendedProgressBar* pDlgProgress =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(
          WINDOW_DIALOG_EXT_PROGRESS);
  CGUIDialogProgressBarHandle* handle = pDlgProgress->GetHandle(g_localizeStrings.Get(605));

  int iTrack = atoi(m_input.substr(13, m_input.size() - 13 - 5).c_str());
  std::string strLine0 =
      StringUtils::Format("{:02}. {} - {}", iTrack, m_tag.GetArtistString(), m_tag.GetTitle());
  handle->SetText(strLine0);

  // start ripping
  int percent = 0;
  int oldpercent = 0;
  bool cancelled{false};
  int result{-1};
  while (!cancelled && (result = RipChunk(reader, encoder, percent)) == 0)
  {
    cancelled = ShouldCancel(percent, 100);
    if (percent > oldpercent)
    {
      oldpercent = percent;
      handle->SetPercentage(static_cast<float>(percent));
    }
  }

  // close encoder ripper
  encoder->EncoderClose();
  encoder.reset();
  reader.Close();

  if (NETWORK::IsRemote(file) && !cancelled && result == 2)
  {
    // copy the ripped track to the share
    if (!CFile::Copy(m_output, file.GetPath()))
    {
      CLog::Log(LOGERROR, "CCDDARipJob::{} - Error copying file from {} to {}", __func__, m_output,
                file.GetPath());
      CFile::Delete(m_output);
      return false;
    }
    // delete cached file
    CFile::Delete(m_output);
  }

  if (cancelled)
  {
    CLog::Log(LOGWARNING, "CCDDARipJob::{} - User Cancelled CDDA Rip", __func__);
    CFile::Delete(m_output);
  }
  else if (result == 1)
    CLog::Log(LOGERROR, "CCDDARipJob::{} - Error ripping {}", __func__, m_input);
  else if (result < 0)
    CLog::Log(LOGERROR, "CCDDARipJob::{} - Error encoding {}", __func__, m_input);
  else
  {
    CLog::Log(LOGINFO, "CCDDARipJob::{} - Finished ripping {}", __func__, m_input);
    if (m_eject)
    {
      CLog::Log(LOGINFO, "CCDDARipJob::{} - Ejecting CD", __func__);
      CServiceBroker::GetMediaManager().EjectTray();
    }
  }

  handle->MarkFinished();

  return !cancelled && result == 2;
}

int CCDDARipJob::RipChunk(CFile& reader, const std::unique_ptr<CEncoder>& encoder, int& percent)
{
  percent = 0;

  uint8_t stream[1024];

  // get data
  ssize_t result = reader.Read(stream, 1024);

  // return if rip is done or on some kind of error
  if (result <= 0)
    return 1;

  // encode data
  ssize_t encres = encoder->EncoderEncode(stream, result);

  // Get progress indication
  percent = static_cast<int>(reader.GetPosition() * 100 / reader.GetLength());

  if (reader.GetPosition() == reader.GetLength())
    return 2;

  return -(1 - encres);
}

std::unique_ptr<CEncoder> CCDDARipJob::SetupEncoder(CFile& reader)
{
  std::unique_ptr<CEncoder> encoder;
  const std::string audioEncoder = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CSettings::SETTING_AUDIOCDS_ENCODER);
  if (audioEncoder == "audioencoder.kodi.builtin.aac" ||
      audioEncoder == "audioencoder.kodi.builtin.wma")
  {
    encoder = std::make_unique<CEncoderFFmpeg>();
  }
  else
  {
    const AddonInfoPtr addonInfo =
        CServiceBroker::GetAddonMgr().GetAddonInfo(audioEncoder, AddonType::AUDIOENCODER);
    if (addonInfo)
    {
      encoder = std::make_unique<CEncoderAddon>(addonInfo);
    }
  }
  if (!encoder)
    return std::unique_ptr<CEncoder>{};

  // we have to set the tags before we init the Encoder
  const std::string strTrack = StringUtils::Format(
      "{}", std::stol(m_input.substr(13, m_input.size() - 13 - 5), nullptr, 10));

  const std::string itemSeparator =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;

  encoder->SetComment(std::string("Ripped with ") + CSysInfo::GetAppName());
  encoder->SetArtist(StringUtils::Join(m_tag.GetArtist(), itemSeparator));
  encoder->SetTitle(m_tag.GetTitle());
  encoder->SetAlbum(m_tag.GetAlbum());
  encoder->SetAlbumArtist(StringUtils::Join(m_tag.GetAlbumArtist(), itemSeparator));
  encoder->SetGenre(StringUtils::Join(m_tag.GetGenre(), itemSeparator));
  encoder->SetTrack(strTrack);
  encoder->SetTrackLength(static_cast<int>(reader.GetLength()));
  encoder->SetYear(m_tag.GetYearString());

  // init encoder
  if (!encoder->EncoderInit(m_output, m_channels, m_rate, m_bps))
    encoder.reset();

  return encoder;
}

std::string CCDDARipJob::SetupTempFile()
{
  char tmp[MAX_PATH + 1];
#if defined(TARGET_WINDOWS)
  using namespace KODI::PLATFORM::WINDOWS;
  wchar_t tmpW[MAX_PATH];
  GetTempFileName(ToW(CSpecialProtocol::TranslatePath("special://temp/")).c_str(), L"riptrack", 0,
                  tmpW);
  auto tmpString = FromW(tmpW);
  strncpy_s(tmp, tmpString.length(), tmpString.c_str(), MAX_PATH);
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
  if (strcmp(job->GetType(), GetType()) == 0)
  {
    const CCDDARipJob* rjob = dynamic_cast<const CCDDARipJob*>(job);
    if (rjob)
    {
      return m_input == rjob->m_input && m_output == rjob->m_output;
    }
  }
  return false;
}
