#pragma once

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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "DSUtil/DSUtil.h"
#include "DSUtil/DShowCommon.h"
#include "DSUtil/MediaTypeEx.h"
#include <initguid.h>
#include "moreuuids.h"
#include <dmodshow.h>
#include <D3d9.h>

#include "Filters/ffdshow_constants.h"
#include "DSGraph.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "utils/RegExp.h"
#include "Subtitles/subpic/ISubPic.h"
#include "Subtitles/DllLibSubs.h"
#include "Subtitles/ILogImpl.h"

#include "utils/StreamDetails.h"

class CDSStreamDetail
{
public:
  CDSStreamDetail();

  uint32_t                IAMStreamSelect_Index; ///< IAMStreamSelect index of the stream
  CStdString              displayname; ///< Stream displayname
  unsigned long           flags; ///< Stream flags. Set to AMSTREAMSELECTINFO_ENABLED if the stream if selected in the GUI, 0 otherwise
  Com::SmartPtr<IPin>     pObj; ///< Output pin of the splitter
  Com::SmartPtr<IPin>     pUnk; ///< Not used
  LCID                    lcid; ///< LCID of stream language
  DWORD                   group; ///< Currently not used
  bool                    connected; ///< Is the stream connected
};

//  CStdString codecname; ///< Stream codec name
//  CStdString codec; ///< Stream codec ID

/// Informations about an audio stream
class CDSStreamDetailAudio: public CDSStreamDetail, public CStreamDetailAudio
{
public:
  CDSStreamDetailAudio();

  CStdString              m_strCodecName;
  uint32_t                m_iBitRate; ///< Audio bitrate
  uint32_t                m_iSampleRate; ///< Audio samplerate
};

/// Informations about a video stream
struct CDSStreamDetailVideo: public CDSStreamDetail, public CStreamDetailVideo
{
public:
  CStdString              m_strCodecName;

  CDSStreamDetailVideo();
};

/// Informations about a internal subtitle
enum SubtitleType {
  INTERNAL,
  EXTERNAL
};
class CDSStreamDetailSubtitle: public CDSStreamDetail, public CStreamDetailSubtitle
{
public:
  CDSStreamDetailSubtitle(SubtitleType type = INTERNAL);

  CStdString            encoding; ///< Subtitle encoding
  uint32_t              offset; ///< Not used
  CStdString            isolang; ///< ISO Code of the subtitle language.
  CStdString            trackname; ///< 
  GUID                  subtype;
  const SubtitleType    m_subType;
};

/// Informations about an external subtitle
class CDSStreamDetailSubtitleExternal: public CDSStreamDetailSubtitle
{
public:

  CDSStreamDetailSubtitleExternal();

  CStdString            path; ///< Subtitle file path
  Com::SmartPtr<ISubStream> substream;
};

/** @brief DSPlayer Streams Manager.
 *
 * Singleton class handling audio and subtitle streams.
 */
class CStreamsManager
{
public:
  friend class CSubtitleManager;
  /// Retrieve singleton instance
  static CStreamsManager *Get();
  /// Destroy singleton instance
  static void Destroy();

  /// @return A std::vector of all audio streams found in the media file
  std::vector<CDSStreamDetailAudio *>& GetAudios();

  /// @return Audio streams count
  int  GetAudioStreamCount();
  /// @return The index to the current audio stream
  int  GetAudioStream();
  /** Get the displayname of an audio stream
   * @param[in] iStream Index of the audio stream to get displayname of
   * @param[out] strStreamName Name of the iStream audio stream
   */
  void GetAudioStreamName(int iStream, CStdString &strStreamName);
  /** Change the current audio stream
   * @param[in] iStream Index of the audio stream
   * @remarks If the IAMStreamSelect interface wasn't found, the graph must be stopped and restarted in order to change the audio stream
   */
  void SetAudioStream(int iStream);
  /** Wait until the graph is ready. If a stream is being changed, the
   * function waits. Otherwise, simply returns.
   */
  void WaitUntilReady();
  /// @return The number of audio channels of the current audio stream
  int GetChannels();
  /// @return The number of bits per sample of the current audio stream
  int GetBitsPerSample();
  /// @return The sample rate of the current audio stream
  int GetSampleRate();
  /// @return The ID of the audio codec used in the media file (ie FLAC, MP3, DTS ...)
  CStdString GetAudioCodecName();
  /// @return The displayname of the audio codec used in the media file (ie FLAC, MP3, DTS ...)
  CStdString GetAudioCodecDisplayName() { int i = GetAudioStream(); return (i == -1) ? "" : m_audioStreams[i]->m_strCodecName; }
  /// @return An instance to the IAMStreamSelect interface if the splitter expose it, NULL otherwise
  IAMStreamSelect *GetStreamSelector() { return m_pIAMStreamSelect; }
  
  /// Initialize streams from the current media file
  void LoadStreams();

  /// @return Current video width
  int GetPictureWidth();
  /// @return Current video height
  int GetPictureHeight();
  /// @return The ID of the video codec used in the media file (XviD, DivX, h264, ...)
  CStdString GetVideoCodecName();
  /// @return The displayname of the video codec used in the media file (XviD, DivX, h264, ...)
  CStdString GetVideoCodecDisplayName() { return m_videoStream.m_strCodecName; }
  
  /** Initialize the manager
   * @return True if the manager is initialized, false otherwise
   */
  bool InitManager();
  /** Extract stream information from AM_MEDIA_TYPE
   * @param[in] mt Media type informations
   * @param[out] s A filled SStreamInfos structure
   */
  static void MediaTypeToStreamDetail(AM_MEDIA_TYPE *mt, CStreamDetail& s);
  static void FormatStreamName(CStreamDetail& s);
  static void ExtractCodecDetail(CStreamDetail& s, CStdString& codecInfos);
  static CStdString ISOToLanguage(CStdString code);

  void LoadDVDStreams();
  void UpdateDVDStream();

  CDSStreamDetailVideo* GetVideoStreamDetail(unsigned int iIndex = 0);
  CDSStreamDetailAudio* GetAudioStreamDetail(unsigned int iIndex = 0);

  boost::shared_ptr<CSubtitleManager> SubtitleManager;

private:
  CStreamsManager(void);
  ~CStreamsManager(void);
  static CStreamsManager *m_pSingleton;

  void LoadStreamsInternal();
  void LoadIAMStreamSelectStreamsInternal();

  std::vector<CDSStreamDetailAudio *> m_audioStreams;
  Com::SmartPtr<IAMStreamSelect> m_pIAMStreamSelect;
  Com::SmartPtr<IFilterGraph2> m_pGraphBuilder;
  Com::SmartPtr<IBaseFilter> m_pSplitter;

  bool m_init;
  CDSStreamDetailVideo m_videoStream;
  CCriticalSection m_lock;
  bool m_dvdStreamLoaded;

  CEvent m_readyEvent;
};

class CSubtitleManager
{
public:
  CSubtitleManager(CStreamsManager* pStreamManager);
  ~CSubtitleManager();
  
  void Initialize();
  void Unload();
  bool Ready();

  /** @remark renderRect represent the rendering surface size */
  HRESULT GetTexture(Com::SmartPtr<IDirect3DTexture9>& pTexture, Com::SmartRect& pSrc, Com::SmartRect& pDest, Com::SmartRect& renderRect);

  void StopThread();
  void StartThread();

  /// @return A std::vector of all subtitle streams (internal or external) found in the media file
  std::vector<CDSStreamDetailSubtitle *>& GetSubtitles();
  /// @return Subtitles count
  int  GetSubtitleCount();
  /// @return The index of the current subtitle
  int  GetSubtitle();
  /** Get the displayname of a subtitle
   * @param[in] iStream Index of the subtitle to get displayname of
   * @param[out] strStreamName Name of the iStream subtitle
   */
  void GetSubtitleName(int iStream, CStdString &strStreamName);
  /** Change the current subtitle
   * @param[in] iStream Index of the subtitle
   * @remarks If the IAMStreamSelect interface wasn't found, the graph must be stopped and restarted in order to change the subtitle
   */
  void SetSubtitle(int iStream);
  /// @return True if subtitles are visible, false otherwise
  bool GetSubtitleVisible();
  /** Set subtitle visibility
   * @param[in] bVisible Subtitle visibility
   */
  void SetSubtitleVisible( bool bVisible );
  /** Set subtitle delay
   * @param in fValue Subtitle delay (in ms)
   */
  void SetSubtitleDelay(float fValue = 0.0f);
  /// @return Current subtitle delay
  float GetSubtitleDelay(void);
  /** Add a subtitle to the manager
   * @param[in] subFilePath Path of the subtitle
   * @return -1 if the function fails. Otherwise, returns the index of the added subtitle
   * @remarks The subtitle will be automatically flagged as external
  */
  int AddSubtitle(const CStdString& subFilePath);

  void SetTime(REFERENCE_TIME rtNow);
  void SetSizes(Com::SmartRect window, Com::SmartRect video);

  CDSStreamDetailSubtitle* GetSubtitleStreamDetail(unsigned int iIndex = 0);
  CDSStreamDetailSubtitleExternal* GetExternalSubtitleStreamDetail(unsigned int iIndex = 0);

  void SetTimePerFrame(REFERENCE_TIME iTimePerFrame);

  void SelectBestSubtitle();
private:
  void DisconnectCurrentSubtitlePins(void);
  IPin *GetFirstSubtitlePin(void);
  static void DeleteSubtitleManager(ISubManager* pManager, DllLibSubs dll);

  std::vector<CDSStreamDetailSubtitle *> m_subtitleStreams;
  DllLibSubs m_dll;
  boost::shared_ptr<ISubManager> m_pManager;
  boost::shared_ptr<ILogImpl> m_Log;
  CStreamsManager* m_pStreamManager;
  bool m_bSubtitlesUnconnected;
  bool m_bSubtitlesVisible;
  AM_MEDIA_TYPE m_subtitleMediaType;
  REFERENCE_TIME m_rtSubtitleDelay;
};