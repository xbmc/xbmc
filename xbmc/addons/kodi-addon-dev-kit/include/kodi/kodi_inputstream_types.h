#pragma once

/*
 *      Copyright (C) 2005-2016 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

#ifdef BUILD_KODI_ADDON
#include "DVDDemuxPacket.h"
#else
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxPacket.h"
#endif

/* current API version */
#define INPUTSTREAM_API_VERSION "1.0.6"

extern "C" {

  // this are properties given to the addon on create
  // at this time we have no parameters for the addon
  typedef struct INPUTSTREAM_PROPS
  {
    int dummy;
  } INPUTSTREAM_PROPS;

  /*!
   * @brief InputStream add-on capabilities. All capabilities are set to "false" as default.
   */
  typedef struct INPUTSTREAM_CAPABILITIES
  {
    enum MASKTYPE: uint32_t
    {
      /// supports interface IDemux
      SUPPORTSIDEMUX = (1 << 0),

      /// supports interface IPosTime
      SUPPORTSIPOSTIME = (1 << 1),

      /// supports interface IDisplayTime
      SUPPORTSIDISPLAYTIME = (1 << 2),

      /// supports seek
      SUPPORTSSEEK = (1 << 3),

      /// supports pause
      SUPPORTSPAUSE = (1 << 4)
    };

    /// set of supported capabilities
    MASKTYPE m_mask;
  } INPUTSTREAM_CAPABILITIES;

  /*!
   * @brief structure of key/value pairs passed to addon on Open()
   */
  typedef struct INPUTSTREAM
  {
    static const unsigned int MAX_INFO_COUNT = 8;

    const char *m_strURL;

    unsigned int m_nCountInfoValues;
    struct LISTITEMPROPERTY
    {
      const char *m_strKey;
      const char *m_strValue;
    } m_ListItemProperties[MAX_INFO_COUNT];

    const char *m_libFolder;
    const char *m_profileFolder;
  } INPUTSTREAM;

  /*!
   * @brief Array of stream IDs
   */
  typedef struct INPUTSTREAM_IDS
  {
    static const unsigned int MAX_STREAM_COUNT = 32;
    unsigned int m_streamCount;
    unsigned int m_streamIds[MAX_STREAM_COUNT];
  } INPUTSTREAM_IDS;

  /*!
   * @brief stream properties
   */
  typedef struct INPUTSTREAM_INFO
  {
    enum STREAM_TYPE
    {
      TYPE_NONE,
      TYPE_VIDEO,
      TYPE_AUDIO,
      TYPE_SUBTITLE,
      TYPE_TELETEXT
    } m_streamType;

    char m_codecName[32];                /*!< @brief (required) name of codec according to ffmpeg */
    char m_codecInternalName[32];        /*!< @brief (optional) internal name of codec (selectionstream info) */
    unsigned int m_pID;                  /*!< @brief (required) physical index */
    unsigned int m_Bandwidth;            /*!< @brief (optional) bandwidth of the stream (selectionstream info) */

    const uint8_t *m_ExtraData;
    unsigned int m_ExtraSize;

    char m_language[4];                  /*!< @brief ISO 639 3-letter language code (empty string if undefined) */

    unsigned int m_FpsScale;             /*!< @brief Scale of 1000 and a rate of 29970 will result in 29.97 fps */
    unsigned int m_FpsRate;
    unsigned int m_Height;               /*!< @brief height of the stream reported by the demuxer */
    unsigned int m_Width;                /*!< @brief width of the stream reported by the demuxer */
    float m_Aspect;                      /*!< @brief display aspect of stream */

    unsigned int m_Channels;             /*!< @brief (required) amount of channels */
    unsigned int m_SampleRate;           /*!< @brief (required) sample rate */
    unsigned int m_BitRate;              /*!< @brief (required) bit rate */
    unsigned int m_BitsPerSample;        /*!< @brief (required) bits per sample */
    unsigned int m_BlockAlign;

    enum CRYPTO_KEY_SYSTEM :uint16_t
    {
      CRYPTO_KEY_SYSTEM_NONE = 0,
      CRYPTO_KEY_SYSTEM_WIDEVINE,
      CRYPTO_KEY_SYSTEM_PLAYREADY,
      CRYPTO_KEY_SYSTEM_COUNT
    } m_CryptoKeySystem;                 /*!< @brief keysystem for encrypted media, KEY_SYSTEM_NONE for unencrypted media */
    char * m_CryptoSessionId;            /*!< @brief The crypto session key id */
    uint16_t m_CryptoSessionIdSize;      /*!< @brief The size of the crypto session key id */
  } INPUTSTREAM_INFO;

  /*!
   * @brief Structure to transfer the methods from xbmc_inputstream_dll.h to XBMC
   */
  typedef struct KodiToAddonFuncTable_InputStream
  {
    bool (__cdecl* Open)(INPUTSTREAM&);
    void (__cdecl* Close)(void);
    const char* (__cdecl* GetPathList)(void);
    struct INPUTSTREAM_CAPABILITIES (__cdecl* GetCapabilities)(void);
    const char* (__cdecl* GetApiVersion)(void);

    // IDemux
    struct INPUTSTREAM_IDS (__cdecl* GetStreamIds)();
    struct INPUTSTREAM_INFO (__cdecl* GetStream)(int);
    void (__cdecl* EnableStream)(int, bool);
    void (__cdecl* DemuxReset)(void);
    void (__cdecl* DemuxAbort)(void);
    void (__cdecl* DemuxFlush)(void);
    DemuxPacket* (__cdecl* DemuxRead)(void);
    bool (__cdecl* DemuxSeekTime)(double, bool, double*);
    void (__cdecl* DemuxSetSpeed)(int);
    void (__cdecl* SetVideoResolution)(int, int);

    // IDisplayTime
    int (__cdecl* GetTotalTime)(void);
    int (__cdecl* GetTime)(void);

    // IPosTime
    bool (__cdecl* PosTime)(int);

    // Seekable (mandatory)
    bool (__cdecl* CanPauseStream)(void);
    bool (__cdecl* CanSeekStream)(void);

    int (__cdecl* ReadStream)(uint8_t*, unsigned int);
    int64_t(__cdecl* SeekStream)(int64_t, int);
    int64_t (__cdecl* PositionStream)(void);
    int64_t (__cdecl* LengthStream)(void);
    void (__cdecl* PauseStream)(double);
    bool (__cdecl* IsRealTimeStream)(void);
  } KodiToAddonFuncTable_InputStream;
}


