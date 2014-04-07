//
//  PlexTranscoderClientRPi.cpp
//  RasPlex
//
//  Created by Lionel Chazallon on 2014-03-07.
//
//

#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>
#include <stdio.h>

#include "Client/PlexTranscoderClientRPi.h"
#include "plex/PlexUtils.h"
#include "log.h"
#include "settings/GUISettings.h"
#include "Client/PlexConnection.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "PlexMediaDecisionEngine.h"

///////////////////////////////////////////////////////////////////////////////
CPlexTranscoderClientRPi::CPlexTranscoderClientRPi()
{
  m_maxVideoBitrate = 0;
  m_maxAudioBitrate = 0;

  // Here is as list of audio / video codecs that we support natively on RPi
  m_knownVideoCodecs = boost::assign::list_of<std::string>  ("h264") ("mpeg4");
  m_knownAudioCodecs = boost::assign::list_of<std::string>  ("") ("aac") ("ac3") ("mp3") ("mp2") ("dca") ("flac");

  // check if optionnal codecs are here
  if ( CheckCodec("MPG2") )
    m_knownVideoCodecs.insert("mpeg2video");

  if ( CheckCodec("WVC1") )
    m_knownVideoCodecs.insert("vc1");
}

///////////////////////////////////////////////////////////////////////////////
#if defined(_LINUX)
bool CPlexTranscoderClientRPi::CheckCodec(std::string codec)
{
  FILE *fp;
  char output[100];
  std::string command,reply;

  // check codec
  command = "vcgencmd codec_enabled " + codec;
  reply = codec + "=enabled";

  fp = popen(command.c_str(), "r");
  if (fp)
  {
      if (fgets(output, sizeof(output)-1, fp))
      {
        if (!strncmp(output, reply.c_str(),reply.length()))
        {
          CLog::Log(LOGDEBUG, "CPlexTranscoderClientRPi :  Codec %s was found.",codec.c_str());
          return true;
        }
        else
          CLog::Log(LOGDEBUG, "CPlexTranscoderClientRPi :  Codec %s was not found.",codec.c_str());
      }
      else
        CLog::Log(LOGERROR, "CPlexTranscoderClientRPi : No reply in %s codec check",codec.c_str());

      pclose(fp);
  }
  else CLog::Log(LOGERROR, "CPlexTranscoderClientRPi : Unable to check %s codec", codec.c_str());

  return false;
}
#else
bool CPlexTranscoderClientRPi::CheckCodec(std::string codec) { return false; }
#endif

///////////////////////////////////////////////////////////////////////////////
bool CPlexTranscoderClientRPi::ShouldTranscode(CPlexServerPtr server, const CFileItem& item)
{
  if (!item.IsVideo())
    return false;

  if (!server || !server->GetActiveConnection())
    return false;

  CFileItemPtr selectedItem = CPlexMediaDecisionEngine::getSelectedMediaItem((item));

  bool bShouldTranscode = false;
  CStdString ReasonWhy;


  // grab some properties
  std::string container = selectedItem->GetProperty("container").asString(),
              videoCodec = selectedItem->GetProperty("mediaTag-videoCodec").asString(),
              audioCodec = selectedItem->GetProperty("mediaTag-audioCodec").asString();


  int videoResolution = selectedItem->GetProperty("mediaTag-videoResolution").asInteger(),
      videoBitRate = selectedItem->GetProperty("bitrate").asInteger(),
      videoWidth = selectedItem->GetProperty("width").asInteger(),
      videoHeight = selectedItem->GetProperty("height").asInteger(),
      audioChannels = selectedItem->GetProperty("mediaTag-audioChannels").asInteger();

  // default capping values
  m_maxVideoBitrate = 20000;
  m_maxAudioBitrate = 1000;
  int maxBitDepth = 8;

  // grab some other information in the audio / video streams
  int audioBitRate = 0;
  float videoFrameRate = 0;
  int bitDepth = 0;

  CFileItemPtr audioStream,videoStream;
  CFileItemPtr mediaPart = selectedItem->m_mediaParts.at(0);
  if (mediaPart)
  {
    if ((audioStream = PlexUtils::GetSelectedStreamOfType(mediaPart, PLEX_STREAM_AUDIO)))
      audioBitRate = audioStream->GetProperty("bitrate").asInteger();
    else
      CLog::Log(LOGERROR,"CPlexTranscoderClient::ShouldTranscodeRPi - AudioStream is empty");

    if ((videoStream = PlexUtils::GetSelectedStreamOfType(mediaPart, PLEX_STREAM_VIDEO)))
    {
      videoFrameRate = videoStream->GetProperty("frameRate").asFloat();
      bitDepth = videoStream->GetProperty("bitDepth").asInteger();
    }
    else
      CLog::Log(LOGERROR,"CPlexTranscoderClient::ShouldTranscodeRPi - VideoStream is empty");
  }
  else CLog::Log(LOGERROR,"CPlexTranscoderClient::ShouldTranscodeRPi - MediaPart is empty");

  // Dump The Video information
  CLog::Log(LOGDEBUG,"----------- Video information for '%s' -----------",selectedItem->GetPath().c_str());
  CLog::Log(LOGDEBUG,"-%16s : %s", "container",container.c_str());
  CLog::Log(LOGDEBUG,"-%16s : %s", "videoCodec",videoCodec.c_str());
  CLog::Log(LOGDEBUG,"-%16s : %d", "videoResolution",videoResolution);
  CLog::Log(LOGDEBUG,"-%16s : %3.3f", "videoFrameRate",videoFrameRate);
  CLog::Log(LOGDEBUG,"-%16s : %d", "bitDepth",bitDepth);
  CLog::Log(LOGDEBUG,"-%16s : %d", "bitrate",videoBitRate);
  CLog::Log(LOGDEBUG,"-%16s : %d", "width",videoWidth);
  CLog::Log(LOGDEBUG,"-%16s : %d", "height",videoHeight);
  CLog::Log(LOGDEBUG,"----------- Audio information -----------");
  CLog::Log(LOGDEBUG,"-%16s : %s", "audioCodec",audioCodec.c_str());
  CLog::Log(LOGDEBUG,"-%16s : %d", "audioChannels",audioChannels);
  CLog::Log(LOGDEBUG,"-%16s : %d", "audioBitRate",audioBitRate);

  // check if seetings are to transcoding for local media
  if ( (g_guiSettings.GetInt("plexmediaserver.localquality") != 0) && (server->GetActiveConnection()->IsLocal()) )
  {
    bShouldTranscode = true;
    m_maxVideoBitrate = g_guiSettings.GetInt("plexmediaserver.localquality");
    ReasonWhy.Format("Settings require local transcoding to %d kbps",g_guiSettings.GetInt("plexmediaserver.localquality"));
  }
  // check if seetings are to transcoding for remote media
  else if ( (g_guiSettings.GetInt("plexmediaserver.remotequality") != 0) && (!server->GetActiveConnection()->IsLocal()) )
  {
    bShouldTranscode = true;
    m_maxVideoBitrate = g_guiSettings.GetInt("plexmediaserver.remotequality");
    ReasonWhy.Format("Settings require remote transcoding to %d kbps",g_guiSettings.GetInt("plexmediaserver.remotequality"));
  }
  // check if Video Codec is natively supported
  else if (m_knownVideoCodecs.find(videoCodec) == m_knownVideoCodecs.end())
  {
    bShouldTranscode = true;
    ReasonWhy.Format("Unknown video codec : %s",videoCodec);
  }
  // check if Audio Codec is natively supported
  else if (m_knownAudioCodecs.find(audioCodec) == m_knownAudioCodecs.end())
  {
    bShouldTranscode = true;
    ReasonWhy.Format("Unknown audio codec : %s",audioCodec);
  }
  // Then we eventually cap the video bitrate
  else if (videoBitRate > m_maxVideoBitrate)
  {
    bShouldTranscode = true;
    ReasonWhy.Format("Video bitrate is too high : %d kbps, (max :%d kbps)",videoBitRate,m_maxVideoBitrate);
  }
  // Then we eventually cap the audio bitrate if total bandwidth exceeds the limit
  else if ((audioBitRate > m_maxAudioBitrate) && ((audioBitRate + videoBitRate) > (m_maxVideoBitrate + m_maxAudioBitrate)))
  {
    bShouldTranscode = true;
    ReasonWhy.Format("Audio bitrate is too high : %d kbps, (max :%d kbps)",audioBitRate,m_maxAudioBitrate);
  }
  else if (bitDepth > maxBitDepth)
  {
    bShouldTranscode = true;
    ReasonWhy.Format("Video bitDepth is too high : %d (max : %d)",bitDepth,maxBitDepth);
  }

  if (bShouldTranscode)
  {
    CLog::Log(LOGDEBUG,"RPi ShouldTranscode decided to transcode, Reason : %s",ReasonWhy.c_str());
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "Transcoding", ReasonWhy);
  }
  else
  {
    CLog::Log(LOGDEBUG,"RPi ShouldTranscode decided not to transcode");
  }

  return bShouldTranscode;
}

///////////////////////////////////////////////////////////////////////////////
std::string CPlexTranscoderClientRPi::GetCurrentBitrate(bool local)
{
  return boost::lexical_cast<std::string>(m_maxVideoBitrate);
}




