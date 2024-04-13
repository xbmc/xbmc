/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayerAudioID3.h"

#include "DVDStreamInfo.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "Interface/DemuxPacket.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "music/tags/MusicInfoTag.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRRadioRDSInfoTag.h"
#include "utils/Digest.h"
#include "utils/log.h"

#include <taglib/attachedpictureframe.h>
#include <taglib/commentsframe.h>
#include <taglib/id3v1genres.h>
#include <taglib/id3v2framefactory.h>
#include <taglib/mpegfile.h>
#include <taglib/tbytevectorstream.h>
#include <taglib/textidentificationframe.h>

using namespace TagLib;
using namespace PVR;

using namespace std::chrono_literals;

CVideoPlayerAudioID3::CVideoPlayerAudioID3(CProcessInfo& processInfo)
  : IDVDStreamPlayer(processInfo), CThread("VideoPlayerAudioID3"), m_messageQueue("id3")
{
  CLog::Log(LOGDEBUG, "Audio ID3 tag processor - new {}", __FUNCTION__);

  if (!XFILE::CDirectory::Exists(m_cacheDir))
    XFILE::CDirectory::Create(m_cacheDir);
}

CVideoPlayerAudioID3::~CVideoPlayerAudioID3()
{
  CLog::Log(LOGDEBUG, "Audio ID3 tag processor - delete {}", __FUNCTION__);
  StopThread();
}

bool CVideoPlayerAudioID3::CheckStream(const CDVDStreamInfo& hints)
{
  return hints.type == STREAM_AUDIO_ID3;
}

void CVideoPlayerAudioID3::Flush()
{
  if (m_messageQueue.IsInited())
  {
    m_messageQueue.Flush();
    m_messageQueue.Put(std::make_shared<CDVDMsg>(CDVDMsg::GENERAL_FLUSH));
  }
}

void CVideoPlayerAudioID3::WaitForBuffers()
{
  m_messageQueue.WaitUntilEmpty();
}

bool CVideoPlayerAudioID3::OpenStream(CDVDStreamInfo hints)
{
  CloseStream(true);
  m_messageQueue.Init();

  if (hints.type == STREAM_AUDIO_ID3)
  {
    Flush();
    CLog::Log(LOGINFO, "Creating Audio ID3 tag processor data thread");
    Create();
    return true;
  }

  return false;
}

void CVideoPlayerAudioID3::CloseStream(bool bWaitForBuffers)
{
  m_messageQueue.Abort();

  CLog::Log(LOGINFO, "Audio ID3 tag processor - waiting for data thread to exit");
  StopThread();

  m_messageQueue.End();
  m_currentRadiotext.reset();
  if (m_currentPVRChannel)
    m_currentPVRChannel->SetRadioRDSInfoTag(m_currentRadiotext);
  m_currentPVRChannel.reset();
}

void CVideoPlayerAudioID3::SendMessage(std::shared_ptr<CDVDMsg> pMsg, int priority)
{
  if (m_messageQueue.IsInited())
    m_messageQueue.Put(pMsg, priority);
}

void CVideoPlayerAudioID3::FlushMessages()
{
  m_messageQueue.Flush();
}

bool CVideoPlayerAudioID3::IsInited() const
{
  return true;
}

bool CVideoPlayerAudioID3::AcceptsData() const
{
  return !m_messageQueue.IsFull();
}

bool CVideoPlayerAudioID3::IsStalled() const
{
  return true;
}

void CVideoPlayerAudioID3::OnExit()
{
  CLog::Log(LOGINFO, "Audio ID3 tag processor - thread end");
}

void CVideoPlayerAudioID3::Process()
{
  CLog::Log(LOGINFO, "Audio ID3 tag processor - running thread");

  while (!m_bStop)
  {
    std::shared_ptr<CDVDMsg> pMsg;
    int iPriority = (m_speed == DVD_PLAYSPEED_PAUSE) ? 1 : 0;
    MsgQueueReturnCode ret = m_messageQueue.Get(pMsg, 2s, iPriority);

    // Timeout for ID3 tag data is not a bad thing, so we continue without error
    if (ret == MSGQ_TIMEOUT)
      continue;

    if (MSGQ_IS_ERROR(ret))
    {
      if (!m_messageQueue.ReceivedAbortRequest())
        CLog::Log(LOGERROR, "MSGQ_IS_ERROR returned true ({})", ret);

      break;
    }

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      std::unique_lock<CCriticalSection> lock(m_critSection);
      DemuxPacket* pPacket = std::static_pointer_cast<CDVDMsgDemuxerPacket>(pMsg)->GetPacket();
      if (pPacket)
        ProcessID3(pPacket->pData, pPacket->iSize);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      m_speed = std::static_pointer_cast<CDVDMsgInt>(pMsg)->m_value;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH) || pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      ResetCache();
    }
  }
}

void CVideoPlayerAudioID3::ProcessID3(const unsigned char* data, unsigned int length) const
{
  if (data && length > 0)
  {
    ByteVectorStream tagStream(ByteVector(reinterpret_cast<const char*>(data), length));
    if (tagStream.isOpen())
    {
      MPEG::File tagFile = MPEG::File(&tagStream, ID3v2::FrameFactory::instance());
      if (tagFile.isOpen())
      {
        if (tagFile.hasID3v1Tag())
          ProcessID3v1(tagFile.ID3v1Tag(false));

        else if (tagFile.hasID3v2Tag())
          ProcessID3v2(tagFile.ID3v2Tag(false));
      }
    }
  }
}

void CVideoPlayerAudioID3::ProcessID3v1(const ID3v1::Tag* tag) const
{
  if (tag != nullptr && !tag->isEmpty())
  {
    MUSIC_INFO::CMusicInfoTag* currentMusic = g_application.CurrentFileItem().GetMusicInfoTag();
    if (currentMusic)
    {
      bool changed = false;

      const String title = tag->title();
      if (!title.isEmpty())
      {
        currentMusic->SetTitle(title.to8Bit(true));
        changed = true;
      }

      const String artist = tag->artist();
      if (!artist.isEmpty())
      {
        currentMusic->SetArtist(artist.to8Bit(true));
        changed = true;
      }

      const String album = tag->album();
      if (!album.isEmpty())
      {
        currentMusic->SetAlbum(album.to8Bit(true));
        changed = true;
      }

      const String comment = tag->comment();
      if (!comment.isEmpty())
      {
        currentMusic->SetComment(comment.to8Bit(true));
        changed = true;
      }

      const String genre = tag->genre();
      if (!genre.isEmpty())
      {
        currentMusic->SetGenre(genre.to8Bit(true));
        changed = true;
      }

      const unsigned int year = tag->year();
      if (year != 0)
      {
        currentMusic->SetYear(year);
        changed = true;
      }

      const unsigned int track = tag->track();
      if (track != 0)
      {
        currentMusic->SetTrackNumber(track);
        changed = true;
      }

      if (changed)
        CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(g_application.CurrentFileItem());
    }
  }
}

void CVideoPlayerAudioID3::ProcessID3v2(const ID3v2::Tag* tag) const
{
  if (tag != nullptr && !tag->isEmpty())
  {
    CFileItem& currentMusicFile = g_application.CurrentFileItem();
    MUSIC_INFO::CMusicInfoTag* currentMusic = currentMusicFile.GetMusicInfoTag();
    if (currentMusic)
    {
      if (m_currentRadiotext)
        m_currentRadiotext->SetPlayingRadioText(true);

      bool changed = false;

      const ID3v2::FrameListMap& frameListMap = tag->frameListMap();
      for (const auto& it : frameListMap)
      {
        if (it.first[0] == 'T')
        {
          std::string str = it.second.front()->toString().to8Bit(true);
          if (str == "!!!EMPTY!!!")
            str = "";

          if (it.first == "TKDL")
          {
            if (m_currentRadiotext)
              m_currentRadiotext->SetRadioText(str);
          }

          else if (it.first == "TKNO")
          {
            if (m_currentRadiotext)
              m_currentRadiotext->SetProgNow(str);
          }

          else if (it.first == "TKNE")
          {
            if (m_currentRadiotext)
              m_currentRadiotext->SetProgNext(str);
          }

          else if (it.first == "TIT2")
          {
            if (!str.empty())
            {
              currentMusic->SetTitle(str);
              if (m_currentRadiotext)
                m_currentRadiotext->SetTitle(str);
            }
            else
            {
              ClearPlayingTitle(currentMusic);
            }
          }

          else if (it.first == "TPE1")
          {
            currentMusic->SetArtist(GetID3v2StringList(it.second), true);
            if (m_currentRadiotext)
              m_currentRadiotext->SetArtist(str);
          }

          else if (it.first == "TALB")
          {
            currentMusic->SetAlbum(str);
            if (m_currentRadiotext)
              m_currentRadiotext->SetAlbum(str);
          }

          else if (it.first == "TCON")
          {
            currentMusic->SetGenre(GetID3v2StringList(it.second));
          }

          else if (it.first == "TYER")
          {
            currentMusic->SetYear(
                static_cast<int>(strtol(it.second.front()->toString().toCString(true), nullptr, 10)));
          }

          else if (it.first == "TRCK")
          {
            currentMusic->SetTrackNumber(
                static_cast<int>(strtol(it.second.front()->toString().toCString(true), nullptr, 10)));
          }

          changed = true;
        }

        else if (it.first == "COMM")
        {
          // Loop through and look for the main (no description) comment
          for (const auto& ct : it.second)
          {
            auto commentsFrame = dynamic_cast<const ID3v2::CommentsFrame*>(ct);
            if (commentsFrame && commentsFrame->description().isEmpty())
            {
              const std::string str = it.second.front()->toString().to8Bit(true);
              currentMusic->SetComment(str);

              if (m_currentRadiotext)
                m_currentRadiotext->SetComment(str);
              changed = true;
              break;
            }
          }
        }

        // Support for setting the cover art image via CMusicInfoTag does not currently exist,
        // the code sample below would check for an ID3v2 "APIC" tag of the proper type and
        // convert the information into an EmbeddedArt object instance
        //
        else if (it.first == "APIC")
        {
          using KODI::UTILITY::CDigest;

          // Loop through and look for the FrontCover picture frame
          for (const auto& pi : it.second)
          {
            auto pictureFrame = dynamic_cast<ID3v2::AttachedPictureFrame*>(pi);
            if (pictureFrame)
            {
              std::string ext;
              if (pictureFrame->mimeType().to8Bit(true) == "image/png")
                ext = "png";
              else if (pictureFrame->mimeType().to8Bit(true) == "image/jpeg")
                ext = "jpg";
              else
                continue;

              const ID3v2::AttachedPictureFrame::Type type = pictureFrame->type();

              int picTypeId;
              std::string picTypeName;

              if (type == ID3v2::AttachedPictureFrame::FrontCover)
              {
                picTypeId = 0;
                picTypeName = "fanart";
              }
              else if (type == ID3v2::AttachedPictureFrame::FileIcon)
              {
                picTypeId = 1;
                picTypeName = "thumb";
              }
              else
                continue;

              const uint8_t* data =
                  reinterpret_cast<const uint8_t*>(pictureFrame->picture().data());
              const size_t size = pictureFrame->size();
              const std::string md5 = CDigest::Calculate(CDigest::Type::MD5, data, size);

              if (m_ImageTypeList[picTypeId].lastMD5 == md5)
                continue;

              std::string cacheDir = StringUtils::Format("{}/{}-{}.{}", m_cacheDir, picTypeName,
                                                         m_ImageTypeList[picTypeId].toogleId, ext);

              XFILE::CFile file;
              if (file.OpenForWrite(cacheDir, true))
              {
                file.Write(reinterpret_cast<const uint8_t*>(pictureFrame->picture().data()),
                           pictureFrame->size());
                file.Close();

                currentMusicFile.SetArt(picTypeName, cacheDir);

                m_ImageTypeList[picTypeId].lastMD5 = md5;
                m_ImageTypeList[picTypeId].toogleId = m_ImageTypeList[picTypeId].toogleId ? 0 : 1;

                if (!m_ImageTypeList[picTypeId].lastExt.empty())
                {
                  cacheDir = StringUtils::Format("{}/{}-{}.{}", m_cacheDir, picTypeName,
                                                 m_ImageTypeList[picTypeId].toogleId,
                                                 m_ImageTypeList[picTypeId].lastExt);
                  XFILE::CFile::Delete(cacheDir);
                }

                m_ImageTypeList[picTypeId].lastExt = ext;

                changed = true;
              }
            }
          }
        }
      }

      if (changed)
        CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(g_application.CurrentFileItem());
    }
  }
}

std::vector<std::string> CVideoPlayerAudioID3::GetID3v2StringList(const ID3v2::FrameList& frameList)
{
  auto frame = dynamic_cast<const ID3v2::TextIdentificationFrame*>(frameList.front());
  if (frame)
    return StringListToVectorString(frame->fieldList());
  return {};
}

std::vector<std::string> CVideoPlayerAudioID3::StringListToVectorString(
    const StringList& stringList)
{
  std::vector<std::string> values;
  for (const auto& value : stringList)
    values.emplace_back(value.to8Bit(true));
  return values;
}

void CVideoPlayerAudioID3::ResetCache()
{
  m_currentPVRChannel = g_application.CurrentFileItem().GetPVRChannelInfoTag();
  if (m_currentPVRChannel)
  {
    m_currentRadiotext = std::make_shared<CPVRRadioRDSInfoTag>();
    m_currentPVRChannel->SetRadioRDSInfoTag(m_currentRadiotext);
  }

  // send a message to all windows to tell them to update the radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CVideoPlayerAudioID3::ClearPlayingTitle(MUSIC_INFO::CMusicInfoTag* currentMusic) const
{
  currentMusic->SetTitle("");
  currentMusic->SetArtist("");
  currentMusic->SetAlbum("");
  currentMusic->SetGenre("");
  currentMusic->SetYear(0);
  currentMusic->SetTrackNumber(0);

  if (m_currentRadiotext)
  {
    m_currentRadiotext->SetTitle("");
    m_currentRadiotext->SetArtist("");
    m_currentRadiotext->SetAlbum("");
  }
}
