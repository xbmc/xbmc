#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

//                 _______    _______                              _______    _
//  ||        ||  ||     ||  ||     ||  ||     ||  ||  ||     ||  ||     ||  | |
//  ||        ||  ||     ||  ||     ||  ||\    ||  ||  ||\    ||  ||     ||  | |
//  ||   /\   ||  ||     ||  ||     ||  ||\\   ||  ||  ||\\   ||  ||         | |
//  ||  //\\  ||  ||=====||  ||=====    || \\  ||  ||  || \\  ||  ||  ____   | |
//  || //  \\ ||  ||     ||  ||  \\     ||  \\ ||  ||  ||  \\ ||  ||     ||  |_|
//  ||//    \\||  ||     ||  ||   \\    ||   \\||  ||  ||   \\||  ||     ||   _
//  ||/      \||  ||     ||  ||    \\   ||    \||  ||  ||    \||  ||_____||  |_|
//
//  ____________________________________________________________________________
// |____________________________________________________________________________|
//
// Note this in order to keep compatibility between the API levels!
//
// - Do not use enum's as values, use integer instead and define values as
//   "typedef enum".
// - Do not use CPP style to pass structures, define always as pointer with "*"!
// - Prevent use of structure definition in structure!
// - Do not include dev-kit headers on Kodi's side where function becomes done,
//   predefine them, see e.g. xbmc/addons/binary/interfaces/api3/Addon/Addon_File.h.
//   This is needed to prevent conflicts if Level 2 functions are needed in Level3!
// - Do not use "#defines" for interface values, use typedef!
// - Do not use for new parts headers from Kodi, create a translation system to
//   change add.on side to Kodi's!
//
//  ____________________________________________________________________________
// |____________________________________________________________________________|
//

#include "../definitions.hpp"

#ifdef BUILD_KODI_ADDON
  #include "kodi/AEChannelData.h"
#else
  #include "cores/AudioEngine/Utils/AEChannelData.h"
#endif

#include "../../kodi_audioengine_types.h"
#include "../../xbmc_epg_types.h"
#include "../../xbmc_pvr_types.h"
#include "../../kodi_adsp_types.h"

#include <map>

#ifdef _WIN32                   // windows
#ifndef _SSIZE_T_DEFINED
typedef intptr_t      ssize_t;
#define _SSIZE_T_DEFINED
#endif // !_SSIZE_T_DEFINED
#endif

/*
 * This file includes not for add-on developer used parts, but required for the
 * interface to Kodi over the library.
 */

extern "C"
{

struct DemuxPacket;

API_NAMESPACE

namespace KodiAPI
{

  typedef struct KODI_API_ErrorTranslator
  {
    uint32_t errorCode;
    const char* errorName;
  } KODI_API_ErrorTranslator;

  extern const KODI_API_ErrorTranslator errorTranslator[];

  typedef char* _get_addon_info(void* hdl, const char* id);
  typedef bool _get_setting(void* hdl, const char* settingName, void *settingValue, bool global);
  typedef void _open_settings_dialog(void* hdl);
  typedef void _queue_notification(void* hdl, const int type, const char *msg);
  typedef void _queue_notification_from_type(void* hdl, const int type, const char* aCaption, const char* aDescription, unsigned int displayTime, bool withSound, unsigned int messageTime);
  typedef void _queue_notification_with_image(void* hdl, const char* aImageFile, const char* aCaption, const char* aDescription, unsigned int displayTime, bool withSound, unsigned int messageTime);
  typedef void _get_md5(const char* text, char& md5);
  typedef char* _unknown_to_utf8(void* hdl, const char* str, bool &ret, bool failOnBadChar);
  typedef char* _get_localized_string(void* hdl, long dwCode);
  typedef void _get_language(void* hdl, char& language, unsigned int& iMaxStringSize, int format, bool region);
  typedef void _get_dvd_menu_language(void* hdl, char &language, unsigned int &iMaxStringSize);
  typedef bool _start_server(void* hdl, int typ, bool start, bool wait);
  typedef void _audio_suspend(void* hdl);
  typedef void _audio_resume(void* hdl);
  typedef float _get_volume(void* hdl, bool percentage);
  typedef void _set_volume(void* hdl, float value, bool isPercentage);
  typedef bool _is_muted(void* hdl);
  typedef void _toggle_mute(void* hdl);
  typedef long _get_optical_state(void* hdl);
  typedef bool _eject_optical_drive(void* hdl);
  typedef void _kodi_version(void* hdl, char*& compile_name, int& major, int& minor, char*& revision, char*& tag, char*& tagversion);
  typedef void _kodi_quit(void* hdl);
  typedef void _htpc_shutdown(void* hdl);
  typedef void _htpc_restart(void* hdl);
  typedef void _execute_script(void* hdl, const char* script);
  typedef void _execute_builtin(void* hdl, const char* function, bool wait);
  typedef char* _execute_jsonrpc(void* hdl, const char* jsonrpccommand);
  typedef char* _get_region(void* hdl, const char* id);
  typedef long _get_free_mem(void* hdl);
  typedef int _get_global_idle_time(void* hdl);
  typedef char* _translate_path(void* hdl, const char* path);

  struct CB_Addon_General
  {
    _get_addon_info* get_addon_info;
    _get_setting* get_setting;
    _open_settings_dialog* open_settings_dialog;
    _queue_notification* queue_notification;
    _queue_notification_from_type* queue_notification_from_type;
    _queue_notification_with_image* queue_notification_with_image;
    _get_md5* get_md5;
    _unknown_to_utf8* unknown_to_utf8;
    _get_localized_string* get_localized_string;
    _get_language* get_language;
    _get_dvd_menu_language* get_dvd_menu_language;
    _start_server* start_server;
    _audio_suspend* audio_suspend;
    _audio_resume* audio_resume;
    _get_volume* get_volume;
    _set_volume* set_volume;
    _is_muted* is_muted;
    _toggle_mute* toggle_mute;
    _get_optical_state* get_optical_state;
    _eject_optical_drive* eject_optical_drive;
    _kodi_version* kodi_version;
    _kodi_quit* kodi_quit;
    _htpc_shutdown* htpc_shutdown;
    _htpc_restart* htpc_restart;
    _execute_script* execute_script;
    _execute_builtin* execute_builtin;
    _execute_jsonrpc* execute_jsonrpc;
    _get_region* get_region;
    _get_free_mem* get_free_mem;
    _get_global_idle_time* get_global_idle_time;
    _translate_path* translate_path;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void* _soundplay_get_handle(void* HANDLE, const char *filename);
  typedef void _soundplay_release_handle(void* HANDLE, void* sndHandle);
  typedef void _soundplay_play(void* HANDLE, void* sndHandle);
  typedef void _soundplay_stop(void* HANDLE, void* sndHandle);
  typedef void _soundplay_set_channel(void* HANDLE, void* sndHandle, int channel);
  typedef int _soundplay_get_channel(void* HANDLE, void* sndHandle);
  typedef void _soundplay_set_volume(void* HANDLE, void* sndHandle, float volume);
  typedef float _soundplay_get_volume(void* HANDLE, void* sndHandle);

  struct CB_Addon_Audio
  {
    _soundplay_get_handle* soundplay_get_handle;
    _soundplay_release_handle* soundplay_release_handle;
    _soundplay_play* soundplay_play;
    _soundplay_stop* soundplay_stop;
    _soundplay_set_channel* soundplay_set_channel;
    _soundplay_get_channel* soundplay_get_channel;
    _soundplay_set_volume* soundplay_set_volume;
    _soundplay_get_volume* soundplay_get_volume;
  };

  #define IMPL_ADDONSOUNDPLAY \
    private: \
      void* m_PlayHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void _get_codec_by_name(void* HANDLE, const char* strCodecName, kodi_codec* codec);
  typedef DemuxPacket* _allocate_demux_packet(void* HANDLE, int iDataSize);
  typedef void _free_demux_packet(void* HANDLE, DemuxPacket* pPacket);

  struct CB_InputStream
  {
    _get_codec_by_name* get_codec_by_name;
    _allocate_demux_packet* allocate_demux_packet;
    _free_demux_packet* free_demux_packet;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  typedef bool _can_open_directory(void* HANDLE, const char* strURL);
  typedef bool _create_directory(void* HANDLE, const char* strPath);
  typedef bool _directory_exists(void* HANDLE, const char* strPath);
  typedef bool _remove_directory(void* HANDLE, const char* strPath);

  struct CB_Addon_Directory
  {
    _can_open_directory* can_open_directory;
    _create_directory* create_directory;
    _directory_exists* directory_exists;
    _remove_directory* remove_directory;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void* _open_file(void* hdl, const char* strFileName, unsigned int flags);
  typedef void* _open_file_for_write(void* hdl, const char* strFileName, bool bOverWrite);
  typedef ssize_t _read_file(void* hdl, void* file, void* lpBuf, size_t uiBufSize);
  typedef bool _read_file_string(void* hdl, void* file, char *szLine, int iLineLength);
  typedef ssize_t _write_file(void* hdl, void* file, const void* lpBuf, size_t uiBufSize);
  typedef void _flush_file(void* hdl, void* file);
  typedef int64_t _seek_file(void* hdl, void* file, int64_t iFilePosition, int iWhence);
  typedef int _truncate_file(void* hdl, void* file, int64_t iSize);
  typedef int64_t _get_file_position(void* hdl, void* file);
  typedef int64_t _get_file_length(void* hdl, void* file);
  typedef double  _get_file_download_speed(void* hdl, void* file);
  typedef void _close_file(void* hdl, void* file);
  typedef int _get_file_chunk_size(void* hdl, void* file);
  typedef bool _file_exists(void* hdl, const char *strFileName, bool bUseCache);
  typedef int _stat_file(void* hdl, const char *strFileName, struct __stat64* buffer);
  typedef bool _delete_file(void* hdl, const char *strFileName);
  typedef char* _get_file_md5(void* hdl, const char* strFileName);
  typedef char* _get_cache_thumb_name(void* hdl, const char* strFileName);
  typedef char* _make_legal_filename(void* hdl, const char* strFileName);
  typedef char* _make_legal_path(void* hdl, const char* strPath);
  typedef void* _curl_create(void* hdl, const char* url);
  typedef bool _curl_add_option(void* hdl, void* file, int type, const char* name, const char* value);
  typedef bool _curl_open(void* hdl, void* file, unsigned int flags);

  struct CB_Addon_File
  {
    _open_file* open_file;
    _open_file_for_write* open_file_for_write;
    _read_file* read_file;
    _read_file_string* read_file_string;
    _write_file* write_file;
    _flush_file* flush_file;
    _seek_file* seek_file;
    _truncate_file* truncate_file;
    _get_file_position* get_file_position;
    _get_file_length* get_file_length;
    _get_file_download_speed* get_file_download_speed;
    _close_file* close_file;
    _get_file_chunk_size* get_file_chunk_size;
    _file_exists* file_exists;
    _stat_file* stat_file;
    _delete_file* delete_file;
    _get_file_md5* get_file_md5;
    _get_cache_thumb_name* get_cache_thumb_name;
    _make_legal_filename* make_legal_filename;
    _make_legal_path* make_legal_path;
    _curl_create* curl_create;
    _curl_add_option* curl_add_option;
    _curl_open* curl_open;
  };

 #define IMPL_FILE \
   private: \
     void* m_pFile;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef bool _wake_on_lan(void* HANDLE, const char *mac);
  typedef void _get_ip_address(void* HANDLE, char &ip, unsigned int &iMaxStringSize);
  typedef char* _dns_lookup(void* HANDLE, const char* url, bool& ret);
  typedef char* _url_encode(void* HANDLE, const char* url);

  struct CB_Addon_Network
  {
    _wake_on_lan* wake_on_lan;
    _get_ip_address* get_ip_address;
    _dns_lookup* dns_lookup;
    _url_encode* url_encode;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  struct VFSDirEntry
  {
    char* label;      //!< item label
    char* path;       //!< item path
    bool folder;      //!< Item is a folder
    uint64_t size;    //!< Size of file represented by item
  };

  typedef bool _create_directory(void* HANDLE, const char* strPath);
  typedef bool _directory_exists(void* HANDLE, const char* strPath);
  typedef bool _remove_directory(void* HANDLE, const char* strPath);
  typedef bool _get_directory(void* HANDLE, const char* strPath, const char* mask, VFSDirEntry** items, unsigned int* num_items);
  typedef void _free_directory(void* HANDLE, VFSDirEntry* items, unsigned int num_items);

  struct CB_Addon_VFS
  {
    _create_directory* create_directory;
    _directory_exists* directory_exists;
    _remove_directory* remove_directory;
    _get_directory* get_directory;
    _free_directory* free_directory;
  };

  #define IMPL_VFS_DIR_ENTRY \
    private: \
      std::string m_label; \
      std::string m_path; \
      bool m_bFolder; \
      int64_t m_size;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void _add_dsp_menu_hook(void *hdl, AE_DSP_MENUHOOK *hook);
  typedef void _remove_dsp_menu_hook(void *hdl, AE_DSP_MENUHOOK *hook);
  typedef void _register_dsp_mode(void *hdl, void *mode);
  typedef void _unregister_dsp_Mode(void *hdl, void *mode);
  typedef bool _get_current_sink_format(void* hdl, AudioEngineFormat* sinkFormat);
  typedef void* _make_stream(void* hdl, AudioEngineFormat Format, unsigned int Options);
  typedef void _free_stream(void* hdl, void* StreamHandle);

  struct CB_AudioEngine
  {
    _add_dsp_menu_hook* add_dsp_menu_hook;
    _remove_dsp_menu_hook* remove_dsp_menu_hook;

    _register_dsp_mode* register_dsp_mode;
    _unregister_dsp_Mode* unregister_dsp_Mode;

    _get_current_sink_format* get_current_sink_format;

    _make_stream* make_stream;
    _free_stream* free_stream;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  typedef unsigned int _Stream_GetSpace(void *addonData, AEStreamHandle *handle);
  typedef unsigned int _Stream_AddData(void *addonData, AEStreamHandle *handle, uint8_t* const *Data, unsigned int Offset, unsigned int Frames);
  typedef double _Stream_GetDelay(void *addonData, AEStreamHandle *handle);
  typedef bool _Stream_IsBuffering(void *addonData, AEStreamHandle *handle);
  typedef double _Stream_GetCacheTime(void *addonData, AEStreamHandle *handle);
  typedef double _Stream_GetCacheTotal(void *addonData, AEStreamHandle *handle);
  typedef void _Stream_Pause(void *addonData, AEStreamHandle *handle);
  typedef void _Stream_Resume(void *addonData, AEStreamHandle *handle);
  typedef void _Stream_Drain(void *addonData, AEStreamHandle *handle, bool Wait);
  typedef bool _Stream_IsDraining(void *addonData, AEStreamHandle *handle);
  typedef bool _Stream_IsDrained(void *addonData, AEStreamHandle *handle);
  typedef void _Stream_Flush(void *addonData, AEStreamHandle *handle);
  typedef float _Stream_GetVolume(void *addonData, AEStreamHandle *handle);
  typedef void _Stream_SetVolume(void *addonData, AEStreamHandle *handle, float Volume);
  typedef float _Stream_GetAmplification(void *addonData, AEStreamHandle *handle);
  typedef void _Stream_SetAmplification(void *addonData, AEStreamHandle *handle, float Amplify);
  typedef const unsigned int _Stream_GetFrameSize(void *addonData, AEStreamHandle *handle);
  typedef const unsigned int _Stream_GetChannelCount(void *addonData, AEStreamHandle *handle);
  typedef const unsigned int _Stream_GetSampleRate(void *addonData, AEStreamHandle *handle);
  typedef const int _Stream_GetDataFormat(void *addonData, AEStreamHandle *handle);
  typedef double _Stream_GetResampleRatio(void *addonData, AEStreamHandle *handle);
  typedef void _Stream_SetResampleRatio(void *addonData, AEStreamHandle *handle, double Ratio);

  struct CB_AudioEngine_Stream
  {
    // AudioEngine stream callbacks
    _Stream_GetSpace* AEStream_GetSpace;
    _Stream_AddData* AEStream_AddData;
    _Stream_GetDelay* AEStream_GetDelay;
    _Stream_IsBuffering* AEStream_IsBuffering;
    _Stream_GetCacheTime* AEStream_GetCacheTime;
    _Stream_GetCacheTotal* AEStream_GetCacheTotal;
    _Stream_Pause* AEStream_Pause;
    _Stream_Resume* AEStream_Resume;
    _Stream_Drain* AEStream_Drain;
    _Stream_IsDraining* AEStream_IsDraining;
    _Stream_IsDrained* AEStream_IsDrained;
    _Stream_Flush* AEStream_Flush;
    _Stream_GetVolume* AEStream_GetVolume;
    _Stream_SetVolume* AEStream_SetVolume;
    _Stream_GetAmplification* AEStream_GetAmplification;
    _Stream_SetAmplification* AEStream_SetAmplification;
    _Stream_GetFrameSize* AEStream_GetFrameSize;
    _Stream_GetChannelCount* AEStream_GetChannelCount;
    _Stream_GetSampleRate* AEStream_GetSampleRate;
    _Stream_GetDataFormat* AEStream_GetDataFormat;
    _Stream_GetResampleRatio* AEStream_GetResampleRatio;
    _Stream_SetResampleRatio* AEStream_SetResampleRatio;
  };

#define IMPLEMENT_ADDON_AE_STREAM \
  private: \
    void* m_StreamHandle; \
    unsigned int m_planes;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void _add_menu_hook(void* addonData, PVR_MENUHOOK* hook);
  typedef void _recording(void* addonData, const char* strName, const char* strFileName, bool bOnOff);
  typedef void _connection_state_change(void* addonData, const char* strConnectionString, int newState, const char *strMessage);
  typedef void _epg_event_state_change(void* addonData, EPG_TAG* tag, unsigned int iUniqueChannelId, int newState);

  typedef void _transfer_epg_entry(void* addonData, const ADDON_HANDLE_STRUCT* handle, const EPG_TAG *epgentry);
  typedef void _transfer_channel_entry(void* addonData, const ADDON_HANDLE_STRUCT* handle, const PVR_CHANNEL *chan);
  typedef void _transfer_channel_group(void* addonData, const ADDON_HANDLE_STRUCT* handle, const PVR_CHANNEL_GROUP*);
  typedef void _transfer_channel_group_member(void* addonData, const ADDON_HANDLE_STRUCT* handle, const PVR_CHANNEL_GROUP_MEMBER*);
  typedef void _transfer_timer_entry(void* addonData, const ADDON_HANDLE_STRUCT* handle, const PVR_TIMER *timer);
  typedef void _transfer_recording_entry(void* addonData, const ADDON_HANDLE_STRUCT* handle, const PVR_RECORDING *recording);

  typedef void _trigger_channel_update(void* HANDLE);
  typedef void _trigger_channel_groups_update(void* HANDLE);
  typedef void _trigger_timer_update(void* HANDLE);
  typedef void _trigger_recording_update(void* HANDLE);
  typedef void _trigger_epg_update(void* HANDLE, unsigned int iChannelUid);

  struct CB_PVR
  {
    _add_menu_hook* add_menu_hook;
    _recording* recording;
    _connection_state_change* connection_state_change;
    _epg_event_state_change* epg_event_state_change;

    _transfer_epg_entry* transfer_epg_entry;
    _transfer_channel_entry* transfer_channel_entry;
    _transfer_channel_group* transfer_channel_group;
    _transfer_channel_group_member* transfer_channel_group_member;
    _transfer_timer_entry* transfer_timer_entry;
    _transfer_recording_entry* transfer_recording_entry;

    _trigger_channel_update* trigger_channel_update;
    _trigger_channel_groups_update* trigger_channel_groups_update;
    _trigger_timer_update* trigger_timer_update;
    _trigger_recording_update* trigger_recording_update;
    _trigger_epg_update* trigger_epg_update;
  };

  #define IMPL_STREAM_PROPS \
    private: \
      stream_vector       *m_streamVector; \
      std::map<unsigned int, int> m_streamIndex; \
      void UpdateIndex();
  //----------------------------------------------------------------------------


  //============================================================================
  typedef struct AddonInfoTagMusic
  {
    char* m_url;
    char* m_title;
    char* m_artist;
    char* m_album;
    char* m_albumArtist;
    char* m_genre;
    int m_duration;
    int m_tracks;
    int m_disc;
    char* m_releaseDate;
    int m_listener;
    int m_playCount;
    char* m_lastPlayed;
    char* m_comment;
    char* m_lyrics;
  } AddonInfoTagMusic;

  typedef bool AddonInfoTagMusic_GetFromPlayer(void* hdl, void* player, AddonInfoTagMusic* tag);
  typedef void AddonInfoTagMusic_Release(void* hdl, AddonInfoTagMusic* tag);

  struct CB_Player_AddonInfoTagMusic
  {
    AddonInfoTagMusic_GetFromPlayer* GetFromPlayer;
    AddonInfoTagMusic_Release* Release;
  };

  #define IMPL_ADDON_INFO_TAG_MUSIC \
    private: \
      inline void TransferInfoTag(AddonInfoTagMusic& infoTag); \
 \
      std::string m_url; \
      std::string m_title; \
      std::string m_artist; \
      std::string m_album; \
      std::string m_albumArtist; \
      std::string m_genre; \
      int m_duration; \
      int m_tracks; \
      int m_disc; \
      std::string m_releaseDate; \
      int m_listener; \
      int m_playCount; \
      std::string m_lastPlayed; \
      std::string m_comment; \
      std::string m_lyrics;
  //----------------------------------------------------------------------------


  //============================================================================
  class CPlayerLib_InfoTagVideo;

  typedef struct AddonInfoTagVideo
  {
    char* m_director;
    char* m_writingCredits;
    char* m_genre;
    char* m_country;
    char* m_tagLine;
    char* m_plotOutline;
    char* m_plot;
    char* m_trailer;
    char* m_pictureURL;
    char* m_title;
    char* m_type;
    char* m_votes;
    char* m_cast;
    char* m_file;
    char* m_path;
    char* m_IMDBNumber;
    char* m_MPAARating;
    int m_year;
    double m_rating;
    int m_playCount;
    char* m_lastPlayed;
    char* m_originalTitle;
    char* m_premiered;
    char* m_firstAired;
    char* m_showTitle;
    int m_season;
    int m_episode;
    int m_dbId;
    unsigned int m_duration;
  } AddonInfoTagVideo;

  typedef bool AddonInfoTagVideo_GetFromPlayer(void* hdl, void* player, AddonInfoTagVideo* tag);
  typedef void AddonInfoTagVideo_Release(void* hdl, AddonInfoTagVideo* tag);

  struct CB_Player_AddonInfoTagVideo
  {
    AddonInfoTagVideo_GetFromPlayer* GetFromPlayer;
    AddonInfoTagVideo_Release* Release;
  };

  #define IMPL_ADDON_INFO_TAG_VIDEO \
    private: \
      inline void TransferInfoTag(AddonInfoTagVideo& infoTag); \
 \
      std::string m_director; \
      std::string m_writingCredits; \
      std::string m_genre; \
      std::string m_country; \
      std::string m_tagLine; \
      std::string m_plotOutline; \
      std::string m_plot; \
      std::string m_trailer; \
      std::string m_pictureURL; \
      std::string m_title; \
      std::string m_votes; \
      std::string m_cast; \
      std::string m_file; \
      std::string m_path; \
      std::string m_IMDBNumber; \
      std::string m_MPAARating; \
      int m_year; \
      double m_rating; \
      int m_playCount; \
      std::string m_lastPlayed; \
      std::string m_originalTitle; \
      std::string m_premiered; \
      std::string m_firstAired; \
      unsigned int m_duration;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef char* AddonPlayer_GetSupportedMedia(void* hdl, int mediaType);
  typedef void* AddonPlayer_New(void* hdl);
  typedef void AddonPlayer_Delete(void* hdl, void* handle);
  typedef bool AddonPlayer_PlayFile(void* hdl, void* handle, const char* item, bool windowed);
  typedef bool AddonPlayer_PlayFileItem(void* hdl, void* handle, const void* listitem, bool windowed);
  typedef bool AddonPlayer_PlayList(void* hdl, void* handle, const void* list, int playList, bool windowed, int startpos);
  typedef void AddonPlayer_Stop(void* hdl, void* handle);
  typedef void AddonPlayer_Pause(void* hdl, void* handle);
  typedef void AddonPlayer_PlayNext(void* hdl, void* handle);
  typedef void AddonPlayer_PlayPrevious(void* hdl, void* handle);
  typedef void AddonPlayer_PlaySelected(void* hdl, void* handle, int selected);
  typedef bool AddonPlayer_IsPlaying(void* hdl, void* handle);
  typedef bool AddonPlayer_IsPlayingAudio(void* hdl, void* handle);
  typedef bool AddonPlayer_IsPlayingVideo(void* hdl, void* handle);
  typedef bool AddonPlayer_IsPlayingRDS(void* hdl, void* handle);
  typedef bool AddonPlayer_GetPlayingFile(void* hdl, void* handle, char& file, unsigned int& iMaxStringSize);
  typedef double AddonPlayer_GetTotalTime(void* hdl, void* handle);
  typedef double AddonPlayer_GetTime(void* hdl, void* handle);
  typedef void AddonPlayer_SeekTime(void* hdl, void* handle, double seekTime);
  typedef bool AddonPlayer_GetAvailableVideoStreams(void* hdl, void* handle, char**& streams, unsigned int& entries);
  typedef void AddonPlayer_SetVideoStream(void* hdl, void* handle, int iStream);
  typedef bool AddonPlayer_GetAvailableAudioStreams(void* hdl, void* handle, char**& streams, unsigned int& entries);
  typedef void AddonPlayer_SetAudioStream(void* hdl, void* handle, int iStream);
  typedef bool AddonPlayer_GetAvailableSubtitleStreams(void* hdl, void* handle, char**& streams, unsigned int& entries);
  typedef void AddonPlayer_SetSubtitleStream(void* hdl, void* handle, int iStream);
  typedef void AddonPlayer_ShowSubtitles(void* hdl, void* handle, bool bVisible);
  typedef bool AddonPlayer_GetCurrentSubtitleName(void* hdl, void* handle, char& name, unsigned int& iMaxStringSize);
  typedef void AddonPlayer_AddSubtitle(void* hdl, void* handle, const char* strSubPath);
  typedef void AddonPlayer_ClearList(char**& path, unsigned int entries);

  struct CB_Player_AddonPlayer
  {
    AddonPlayer_GetSupportedMedia* GetSupportedMedia;

    AddonPlayer_New* New;
    AddonPlayer_Delete* Delete;

    AddonPlayer_PlayFile* PlayFile;
    AddonPlayer_PlayFileItem* PlayFileItem;
    AddonPlayer_PlayList* PlayList;
    AddonPlayer_Stop* Stop;
    AddonPlayer_Pause* Pause;
    AddonPlayer_PlayNext* PlayNext;
    AddonPlayer_PlayPrevious* PlayPrevious;
    AddonPlayer_PlaySelected* PlaySelected;
    AddonPlayer_IsPlaying* IsPlaying;
    AddonPlayer_IsPlayingAudio* IsPlayingAudio;
    AddonPlayer_IsPlayingVideo* IsPlayingVideo;
    AddonPlayer_IsPlayingRDS* IsPlayingRDS;

    AddonPlayer_GetPlayingFile* GetPlayingFile;
    AddonPlayer_GetTotalTime* GetTotalTime;
    AddonPlayer_GetTime* GetTime;
    AddonPlayer_SeekTime* SeekTime;

    AddonPlayer_GetAvailableVideoStreams* GetAvailableVideoStreams;
    AddonPlayer_SetVideoStream* SetVideoStream;
    AddonPlayer_GetAvailableAudioStreams* GetAvailableAudioStreams;
    AddonPlayer_SetAudioStream* SetAudioStream;

    AddonPlayer_GetAvailableSubtitleStreams* GetAvailableSubtitleStreams;
    AddonPlayer_SetSubtitleStream* SetSubtitleStream;
    AddonPlayer_ShowSubtitles* ShowSubtitles;
    AddonPlayer_GetCurrentSubtitleName* GetCurrentSubtitleName;
    AddonPlayer_AddSubtitle* AddSubtitle;

    AddonPlayer_ClearList* ClearList;
  };

  #define IMPL_ADDON_PLAYER \
    private: \
      friend class CInfoTagMusic; \
      friend class CInfoTagVideo; \
      void* m_ControlHandle; \
      static void CBOnPlayBackStarted(void* cbhdl); \
      static void CBOnPlayBackEnded(void* cbhdl); \
      static void CBOnPlayBackStopped(void* cbhdl); \
      static void CBOnPlayBackPaused(void* cbhdl); \
      static void CBOnPlayBackResumed(void* cbhdl); \
      static void CBOnQueueNextItem(void* cbhdl); \
      static void CBOnPlayBackSpeedChanged(void* cbhdl, int iSpeed); \
      static void CBOnPlayBackSeek(void* cbhdl, int iTime, int seekOffset);\
      static void CBOnPlayBackSeekChapter(void* cbhdl, int iChapter);
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void* AddonPlayList_New(void* hdl, int playlist);
  typedef void AddonPlayList_Delete(void* hdl, void* handle);
  typedef bool AddonPlayList_LoadPlaylist(void* hdl, void* handle, const char* filename, int playList);
  typedef void AddonPlayList_AddItemURL(void* hdl, void* handle, const char* url, int index);
  typedef void AddonPlayList_AddItemList(void* hdl, void* handle, const void* listitem, int index);
  typedef void AddonPlayList_RemoveItem(void* hdl, void* handle, const char* url);
  typedef void AddonPlayList_ClearList(void* hdl, void* handle);
  typedef int AddonPlayList_GetListSize(void* hdl, void* handle);
  typedef int AddonPlayList_GetListPosition(void* hdl, void* handle);
  typedef void AddonPlayList_Shuffle(void* hdl, void* handle,  bool shuffle);
  typedef void* AddonPlayList_GetItem(void* hdl, void* handle, long position);

  struct CB_Player_AddonPlayList
  {
    AddonPlayList_New* New;
    AddonPlayList_Delete* Delete;
    AddonPlayList_LoadPlaylist* LoadPlaylist;
    AddonPlayList_AddItemURL* AddItemURL;
    AddonPlayList_AddItemList* AddItemList;
    AddonPlayList_RemoveItem* RemoveItem;
    AddonPlayList_ClearList* ClearList;
    AddonPlayList_GetListSize* GetListSize;
    AddonPlayList_GetListPosition* GetListPosition;
    AddonPlayList_Shuffle* Shuffle;
    AddonPlayList_GetItem* GetItem;
  };

  #define IMPL_ADDON_PLAYLIST \
    public: \
      const void* GetListHandle() const \
      { \
        return m_ControlHandle; \
      } \
      const AddonPlayListType GetListType() const \
      { \
        return m_playlist; \
      } \
    private: \
      void* m_ControlHandle; \
      AddonPlayListType m_playlist;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Button_SetVisible(void* hdl, void* handle, bool visible);
  typedef void GUIControl_Button_SetEnabled(void* hdl, void* handle, bool enabled);
  typedef void GUIControl_Button_SetLabel(void* hdl, void* handle, const char *label);
  typedef void GUIControl_Button_GetLabel(void* hdl, void* handle, char &label, unsigned int &iMaxStringSize);
  typedef void GUIControl_Button_SetLabel2(void* hdl, void* handle, const char *label);
  typedef void GUIControl_Button_GetLabel2(void* hdl, void* handle, char &label, unsigned int &iMaxStringSize);

  struct CB_GUI_Control_Button
  {
    GUIControl_Button_SetVisible* SetVisible;
    GUIControl_Button_SetEnabled* SetEnabled;

    GUIControl_Button_SetLabel* SetLabel;
    GUIControl_Button_GetLabel* GetLabel;

    GUIControl_Button_SetLabel2* SetLabel2;
    GUIControl_Button_GetLabel2* GetLabel2;
  };

  #define IMPL_GUI_BUTTON_CONTROL \
    private: \
      CWindow* m_Window; \
      int m_ControlId; \
      void* m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Edit_SetVisible(void* hdl, void* handle, bool visible);
  typedef void GUIControl_Edit_SetEnabled(void* hdl, void* handle, bool enabled);
  typedef void GUIControl_Edit_SetLabel(void* hdl, void* handle, const char *label);
  typedef void GUIControl_Edit_GetLabel(void* hdl, void* handle, char &label, unsigned int &iMaxStringSize);
  typedef void GUIControl_Edit_SetText(void* hdl, void* handle, const char *text);
  typedef void GUIControl_Edit_GetText(void* hdl, void* handle, char &text, unsigned int &iMaxStringSize);
  typedef void GUIControl_Edit_SetCursorPosition(void* hdl, void* handle, unsigned int iPosition);
  typedef unsigned int GUIControl_Edit_GetCursorPosition(void* hdl, void* handle);
  typedef void GUIControl_Edit_SetInputType(void* hdl, void* handle, int type, const char *heading);

  struct CB_GUI_Control_Edit
  {
    GUIControl_Edit_SetVisible* SetVisible;
    GUIControl_Edit_SetEnabled* SetEnabled;

    GUIControl_Edit_SetLabel* SetLabel;
    GUIControl_Edit_GetLabel* GetLabel;
    GUIControl_Edit_SetText* SetText;
    GUIControl_Edit_GetText* GetText;
    GUIControl_Edit_SetCursorPosition* SetCursorPosition;
    GUIControl_Edit_GetCursorPosition* GetCursorPosition;
    GUIControl_Edit_SetInputType* SetInputType;
  };

  #define IMPL_GUI_EDIT_CONTROL \
    private: \
      CWindow* m_Window; \
      int m_ControlId; \
      void* m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_FadeLabel_SetVisible(void* hdl, void* handle, bool visible);
  typedef void GUIControl_FadeLabel_AddLabel(void* hdl, void* handle, const char *text);
  typedef void GUIControl_FadeLabel_GetLabel(void* hdl, void* handle, char &text, unsigned int &iMaxStringSize);
  typedef void GUIControl_FadeLabel_SetScrolling(void* hdl, void* handle, bool scroll);
  typedef void GUIControl_FadeLabel_Reset(void* hdl, void* handle);

  struct CB_GUI_Control_FadeLabel
  {
    GUIControl_FadeLabel_SetVisible* SetVisible;
    GUIControl_FadeLabel_AddLabel* AddLabel;
    GUIControl_FadeLabel_GetLabel* GetLabel;
    GUIControl_FadeLabel_SetScrolling* SetScrolling;
    GUIControl_FadeLabel_Reset* Reset;
  };

  #define IMPL_GUI_FADELABEL_CONTROL \
    private: \
      CWindow* m_Window; \
      int m_ControlId; \
      void* m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Image_SetVisible(void* hdl, void* handle, bool visible);
  typedef void GUIControl_Image_SetFileName(void* hdl, void* handle, const char* strFileName, const bool useCache);
  typedef void GUIControl_Image_SetColorDiffuse(void* hdl, void* handle, uint32_t colorDiffuse);

  struct CB_GUI_Control_Image
  {
    GUIControl_Image_SetVisible* SetVisible;
    GUIControl_Image_SetFileName* SetFileName;
    GUIControl_Image_SetColorDiffuse* SetColorDiffuse;
  };

  #define IMPL_GUI_IMAGE_CONTROL \
    private: \
      CWindow* m_Window; \
      int m_ControlId; \
      void* m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Label_SetVisible(void* hdl, void* handle, bool visible);
  typedef void GUIControl_Label_SetLabel(void* hdl, void* handle, const char *text);
  typedef void GUIControl_Label_GetLabel(void* hdl, void* handle, char &text, unsigned int &iMaxStringSize);

  struct CB_GUI_Control_Label
  {
    GUIControl_Label_SetVisible* SetVisible;
    GUIControl_Label_SetLabel* SetLabel;
    GUIControl_Label_GetLabel* GetLabel;
  };

  #define IMPL_GUI_LABEL_CONTROL \
    private: \
      CWindow* m_Window; \
      int m_ControlId; \
      void* m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Progress_SetVisible(void* hdl, void* handle, bool visible);
  typedef void GUIControl_Progress_SetPercentage(void* hdl, void* handle, float fPercent);
  typedef float GUIControl_Progress_GetPercentage(void* hdl, void* handle);

  struct CB_GUI_Control_Progress
  {
    GUIControl_Progress_SetVisible* SetVisible;
    GUIControl_Progress_SetPercentage* SetPercentage;
    GUIControl_Progress_GetPercentage* GetPercentage;
  };

  #define IMPL_GUI_PROGRESS_CONTROL \
    private: \
      CWindow* m_Window; \
      int m_ControlId; \
      void* m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_RadioButton_SetVisible(void* hdl, void* handle, bool yesNo);
  typedef void GUIControl_RadioButton_SetEnabled(void* hdl, void* handle, bool enabled);
  typedef void GUIControl_RadioButton_SetLabel(void* hdl, void* handle, const char *text);
  typedef void GUIControl_RadioButton_GetLabel(void* hdl, void* handle, char &text, unsigned int &iMaxStringSize);
  typedef void GUIControl_RadioButton_SetSelected(void* hdl, void* handle, bool yesNo);
  typedef bool GUIControl_RadioButton_IsSelected(void* hdl, void* handle);

  struct CB_GUI_Control_RadioButton
  {
    GUIControl_RadioButton_SetVisible* SetVisible;
    GUIControl_RadioButton_SetEnabled* SetEnabled;
    GUIControl_RadioButton_SetLabel* SetLabel;
    GUIControl_RadioButton_GetLabel* GetLabel;
    GUIControl_RadioButton_SetSelected* SetSelected;
    GUIControl_RadioButton_IsSelected* IsSelected;
  };

  #define IMPL_GUI_RADIO_BUTTON_CONTROL \
    private: \
      CWindow* m_Window; \
      int m_ControlId; \
      void* m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIRenderAddon_SetCallbacks(void* addonData, void* handle, void* clienthandle,
      bool    (*createCB)(void*,int,int,int,int,void*),
      void    (*renderCB)(void*),
      void    (*stopCB)(void*),
      bool    (*dirtyCB)(void*));
  typedef void GUIRenderAddon_Delete(void *addonData, void* handle);

  struct CB_GUI_Control_Rendering
  {
    GUIRenderAddon_SetCallbacks* SetCallbacks;
    GUIRenderAddon_Delete* Delete;
  };

  #define IMPL_GUI_RENDERING_CONTROL \
    private: \
      CWindow* m_Window; \
      int m_ControlId; \
      void* m_ControlHandle; \
      static bool OnCreateCB(void* cbhdl, int x, int y, int w, int h, void* device); \
      static void OnRenderCB(void* cbhdl); \
      static void OnStopCB(void* cbhdl); \
      static bool OnDirtyCB(void* cbhdl);
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_SettingsSlider_SetVisible(void* hdl, void* handle, bool yesNo);
  typedef void GUIControl_SettingsSlider_SetEnabled(void* hdl, void* handle, bool yesNo);
  typedef void GUIControl_SettingsSlider_SetText(void* hdl, void* handle, const char *label);
  typedef void GUIControl_SettingsSlider_Reset(void* hdl, void* handle);
  typedef void GUIControl_SettingsSlider_SetIntRange(void* hdl, void* handle, int iStart, int iEnd);
  typedef void GUIControl_SettingsSlider_SetIntValue(void* hdl, void* handle, int iValue);
  typedef int GUIControl_SettingsSlider_GetIntValue(void* hdl, void* handle);
  typedef void GUIControl_SettingsSlider_SetIntInterval(void* hdl, void* handle, int iInterval);
  typedef void GUIControl_SettingsSlider_SetPercentage(void* hdl, void* handle, float fPercent);
  typedef float GUIControl_SettingsSlider_GetPercentage(void* hdl, void* handle);
  typedef void GUIControl_SettingsSlider_SetFloatRange(void* hdl, void* handle, float fStart, float fEnd);
  typedef void GUIControl_SettingsSlider_SetFloatValue(void* hdl, void* handle, float fValue);
  typedef float GUIControl_SettingsSlider_GetFloatValue(void* hdl, void* handle);
  typedef void GUIControl_SettingsSlider_SetFloatInterval(void* hdl, void* handle, float fInterval);

  struct CB_GUI_Control_SettingsSlider
  {
    GUIControl_SettingsSlider_SetVisible* SetVisible;
    GUIControl_SettingsSlider_SetEnabled* SetEnabled;

    GUIControl_SettingsSlider_SetText* SetText;
    GUIControl_SettingsSlider_Reset* Reset;

    GUIControl_SettingsSlider_SetIntRange* SetIntRange;
    GUIControl_SettingsSlider_SetIntValue* SetIntValue;
    GUIControl_SettingsSlider_GetIntValue* GetIntValue;
    GUIControl_SettingsSlider_SetIntInterval* SetIntInterval;

    GUIControl_SettingsSlider_SetPercentage* SetPercentage;
    GUIControl_SettingsSlider_GetPercentage* GetPercentage;

    GUIControl_SettingsSlider_SetFloatRange* SetFloatRange;
    GUIControl_SettingsSlider_SetFloatValue* SetFloatValue;
    GUIControl_SettingsSlider_GetFloatValue* GetFloatValue;
    GUIControl_SettingsSlider_SetFloatInterval* SetFloatInterval;
  };

  #define IMPL_GUI_SETTINGS_SLIDER_CONTROL \
    private: \
      CWindow* m_Window; \
      int m_ControlId; \
      void* m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Slider_SetVisible(void* hdl, void* handle, bool yesNo);
  typedef void GUIControl_Slider_SetEnabled(void* hdl, void* handle, bool yesNo);
  typedef void GUIControl_Slider_Reset(void* hdl, void* handle);
  typedef void GUIControl_Slider_GetDescription(void* hdl, void* handle, char &text, unsigned int &iMaxStringSize);
  typedef void GUIControl_Slider_SetIntRange(void* hdl, void* handle, int start, int end);
  typedef void GUIControl_Slider_SetIntValue(void* hdl, void* handle, int value);
  typedef int GUIControl_Slider_GetIntValue(void* hdl, void* handle);
  typedef void GUIControl_Slider_SetIntInterval(void* hdl, void* handle, int interval);
  typedef void GUIControl_Slider_SetPercentage(void* hdl, void* handle, float percent);
  typedef float GUIControl_Slider_GetPercentage(void* hdl, void* handle);
  typedef void GUIControl_Slider_SetFloatRange(void* hdl, void* handle, float start, float end);
  typedef void GUIControl_Slider_SetFloatValue(void* hdl, void* handle, float value);
  typedef float GUIControl_Slider_GetFloatValue(void* hdl, void* handle);
  typedef void GUIControl_Slider_SetFloatInterval(void* hdl, void* handle, float interval);

  struct CB_GUI_Control_Slider
  {
    GUIControl_Slider_SetVisible* SetVisible;
    GUIControl_Slider_SetEnabled* SetEnabled;

    GUIControl_Slider_Reset* Reset;
    GUIControl_Slider_GetDescription* GetDescription;

    GUIControl_Slider_SetIntRange* SetIntRange;
    GUIControl_Slider_SetIntValue* SetIntValue;
    GUIControl_Slider_GetIntValue* GetIntValue;
    GUIControl_Slider_SetIntInterval* SetIntInterval;

    GUIControl_Slider_SetPercentage* SetPercentage;
    GUIControl_Slider_GetPercentage* GetPercentage;

    GUIControl_Slider_SetFloatRange* SetFloatRange;
    GUIControl_Slider_SetFloatValue* SetFloatValue;
    GUIControl_Slider_GetFloatValue* GetFloatValue;
    GUIControl_Slider_SetFloatInterval* SetFloatInterval;
  };

  #define IMPL_GUI_SLIDER_CONTROL \
    private: \
      CWindow* m_Window; \
      int m_ControlId; \
      void* m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Spin_SetVisible(void* hdl, void* spinhandle, bool visible);
  typedef void GUIControl_Spin_SetEnabled(void* hdl, void* spinhandle, bool enabled);
  typedef void GUIControl_Spin_SetText(void* hdl, void* spinhandle, const char* text);
  typedef void GUIControl_Spin_Reset(void* hdl, void* spinhandle);
  typedef void GUIControl_Spin_SetType(void* hdl, void* handle, int type);
  typedef void GUIControl_Spin_AddStringLabel(void* hdl, void* handle, const char* label, const char* value);
  typedef void GUIControl_Spin_SetStringValue(void* hdl, void* handle, const char* value);
  typedef void GUIControl_Spin_GetStringValue(void* hdl, void* handle, char& value, unsigned int& maxStringSize);
  typedef void GUIControl_Spin_AddIntLabel(void* hdl, void* handle, const char* label, int value);
  typedef void GUIControl_Spin_SetIntRange(void* hdl, void* handle, int start, int end);
  typedef void GUIControl_Spin_SetIntValue(void* hdl, void* handle, int value);
  typedef int GUIControl_Spin_GetIntValue(void* hdl, void* handle);
  typedef void GUIControl_Spin_SetFloatRange(void* hdl, void* handle, float start, float end);
  typedef void GUIControl_Spin_SetFloatValue(void* hdl, void* handle, float value);
  typedef float GUIControl_Spin_GetFloatValue(void* hdl, void* handle);
  typedef void GUIControl_Spin_SetFloatInterval(void* hdl, void* handle, float interval);

  struct CB_GUI_Control_Spin
  {
    GUIControl_Spin_SetVisible* SetVisible;
    GUIControl_Spin_SetEnabled* SetEnabled;

    GUIControl_Spin_SetText* SetText;
    GUIControl_Spin_Reset* Reset;
    GUIControl_Spin_SetType* SetType;

    GUIControl_Spin_AddStringLabel* AddStringLabel;
    GUIControl_Spin_SetStringValue* SetStringValue;
    GUIControl_Spin_GetStringValue* GetStringValue;

    GUIControl_Spin_AddIntLabel* AddIntLabel;
    GUIControl_Spin_SetIntRange* SetIntRange;
    GUIControl_Spin_SetIntValue* SetIntValue;
    GUIControl_Spin_GetIntValue* GetIntValue;

    GUIControl_Spin_SetFloatRange* SetFloatRange;
    GUIControl_Spin_SetFloatValue* SetFloatValue;
    GUIControl_Spin_GetFloatValue* GetFloatValue;
    GUIControl_Spin_SetFloatInterval* SetFloatInterval;
  };

  #define IMPL_GUI_SPIN_CONTROL \
    private: \
      CWindow* m_Window; \
      int m_ControlId; \
      void* m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_TextBox_SetVisible(void* hdl, void* spinhandle, bool visible);
  typedef void GUIControl_TextBox_Reset(void* hdl, void* handle);
  typedef void GUIControl_TextBox_SetText(void* hdl, void* handle, const char* text);
  typedef void GUIControl_TextBox_GetText(void* hdl, void* handle, char &text, unsigned int &iMaxStringSize);
  typedef void GUIControl_TextBox_Scroll(void* hdl, void* handle, unsigned int scroll);
  typedef void GUIControl_TextBox_SetAutoScrolling(void* hdl, void* handle, int delay, int time, int repeat);

  struct CB_GUI_Control_TextBox
  {
    GUIControl_TextBox_SetVisible* SetVisible;
    GUIControl_TextBox_Reset* Reset;
    GUIControl_TextBox_SetText* SetText;
    GUIControl_TextBox_GetText* GetText;
    GUIControl_TextBox_Scroll* Scroll;
    GUIControl_TextBox_SetAutoScrolling* SetAutoScrolling;
  };

  #define IMPL_GUI_TEXTBOX_CONTROL \
    private: \
      CWindow* m_Window; \
      int m_ControlId; \
      void* m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void* GUIDialog_ExtendedProgress_New(void* hdl, const char *title);
  typedef void GUIDialog_ExtendedProgress_Delete(void* hdl, void* handle);
  typedef void GUIDialog_ExtendedProgress_Title(void* hdl, void* handle, char &title, unsigned int &iMaxStringSize);
  typedef void GUIDialog_ExtendedProgress_SetTitle(void* hdl, void* handle, const char *title);
  typedef void GUIDialog_ExtendedProgress_Text(void* hdl, void* handle, char &text, unsigned int &iMaxStringSize);
  typedef void GUIDialog_ExtendedProgress_SetText(void* hdl, void* handle, const char *text);
  typedef bool GUIDialog_ExtendedProgress_IsFinished(void* hdl, void* handle);
  typedef void GUIDialog_ExtendedProgress_MarkFinished(void* hdl, void* handle);
  typedef float GUIDialog_ExtendedProgress_Percentage(void* hdl, void* handle);
  typedef void GUIDialog_ExtendedProgress_SetPercentage(void* hdl, void* handle, float fPercentage);
  typedef void GUIDialog_ExtendedProgress_SetProgress(void* hdl, void* handle, int currentItem, int itemCount);

  struct CB_GUI_Dialog_ExtendedProgress
  {
    GUIDialog_ExtendedProgress_New* New;
    GUIDialog_ExtendedProgress_Delete* Delete;
    GUIDialog_ExtendedProgress_Title* Title;
    GUIDialog_ExtendedProgress_SetTitle* SetTitle;
    GUIDialog_ExtendedProgress_Text* Text;
    GUIDialog_ExtendedProgress_SetText* SetText;
    GUIDialog_ExtendedProgress_IsFinished* IsFinished;
    GUIDialog_ExtendedProgress_MarkFinished* MarkFinished;
    GUIDialog_ExtendedProgress_Percentage* Percentage;
    GUIDialog_ExtendedProgress_SetPercentage* SetPercentage;
    GUIDialog_ExtendedProgress_SetProgress* SetProgress;
  };

  #define IMPL_GUI_EXTENDED_PROGRESS_DIALOG \
    private: \
      void* m_DialogHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef bool GUIDialog_FileBrowser_ShowAndGetDirectory(const char* shares, const char* heading, char& path, unsigned int& iMaxStringSize, bool bWriteOnly);
  typedef bool GUIDialog_FileBrowser_ShowAndGetFile(const char* shares, const char* mask, const char* heading, char& file, unsigned int& iMaxStringSize, bool useThumbs, bool useFileDirectories);
  typedef bool GUIDialog_FileBrowser_ShowAndGetFileFromDir(const char* directory, const char* mask, const char* heading, char& file, unsigned int& iMaxStringSize, bool useThumbs, bool useFileDirectories, bool singleList);
  typedef bool GUIDialog_FileBrowser_ShowAndGetFileList(const char* shares, const char* mask, const char* heading, char**& path, unsigned int& entries, bool useThumbs, bool useFileDirectories);
  typedef bool GUIDialog_FileBrowser_ShowAndGetSource(char& path, unsigned int& iMaxStringSize, bool allowNetworkShares, const char* additionalShare, const char* strType);
  typedef bool GUIDialog_FileBrowser_ShowAndGetImage(const char* shares, const char* heading, char& path, unsigned int& iMaxStringSize);
  typedef bool GUIDialog_FileBrowser_ShowAndGetImageList( const char* shares, const char* heading, char**& path, unsigned int& entries);
  typedef void GUIDialog_FileBrowser_ClearList(char**& path, unsigned int entries);

  struct CB_GUI_Dialog_FileBrowser
  {
    GUIDialog_FileBrowser_ShowAndGetDirectory* ShowAndGetDirectory;
    GUIDialog_FileBrowser_ShowAndGetFile* ShowAndGetFile;
    GUIDialog_FileBrowser_ShowAndGetFileFromDir* ShowAndGetFileFromDir;
    GUIDialog_FileBrowser_ShowAndGetFileList* ShowAndGetFileList;
    GUIDialog_FileBrowser_ShowAndGetSource* ShowAndGetSource;
    GUIDialog_FileBrowser_ShowAndGetImage* ShowAndGetImage;
    GUIDialog_FileBrowser_ShowAndGetImageList* ShowAndGetImageList;
    GUIDialog_FileBrowser_ClearList* ClearList;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  typedef bool GUIDialog_Keyboard_ShowAndGetInputWithHead(char &strTextString, unsigned int &iMaxStringSize, const char *heading, bool allowEmptyResult, bool hiddenInput, unsigned int autoCloseMs);
  typedef bool GUIDialog_Keyboard_ShowAndGetInput(char &strTextString, unsigned int &iMaxStringSize, bool allowEmptyResult, unsigned int autoCloseMs);
  typedef bool GUIDialog_Keyboard_ShowAndGetNewPasswordWithHead(char &newPassword, unsigned int &iMaxStringSize, const char *strHeading, bool allowEmptyResult, unsigned int autoCloseMs);
  typedef bool GUIDialog_Keyboard_ShowAndGetNewPassword(char &strNewPassword, unsigned int &iMaxStringSize, unsigned int autoCloseMs);
  typedef bool GUIDialog_Keyboard_ShowAndVerifyNewPasswordWithHead(char &strNewPassword, unsigned int &iMaxStringSize, const char *strHeading, bool allowEmpty, unsigned int autoCloseMs);
  typedef bool GUIDialog_Keyboard_ShowAndVerifyNewPassword(char &strNewPassword, unsigned int &iMaxStringSize, unsigned int autoCloseMs);
  typedef int GUIDialog_Keyboard_ShowAndVerifyPassword(char &strPassword, unsigned int &iMaxStringSize, const char *strHeading, int iRetries, unsigned int autoCloseMs);
  typedef bool GUIDialog_Keyboard_ShowAndGetFilter(char &aTextString, unsigned int &iMaxStringSize, bool searching, unsigned int autoCloseMs);
  typedef bool GUIDialog_Keyboard_SendTextToActiveKeyboard(const char *aTextString, bool closeKeyboard);
  typedef bool GUIDialog_Keyboard_isKeyboardActivated();

  struct CB_GUI_Dialog_Keyboard
  {
    GUIDialog_Keyboard_ShowAndGetInputWithHead* ShowAndGetInputWithHead;
    GUIDialog_Keyboard_ShowAndGetInput* ShowAndGetInput;
    GUIDialog_Keyboard_ShowAndGetNewPasswordWithHead* ShowAndGetNewPasswordWithHead;
    GUIDialog_Keyboard_ShowAndGetNewPassword* ShowAndGetNewPassword;
    GUIDialog_Keyboard_ShowAndVerifyNewPasswordWithHead* ShowAndVerifyNewPasswordWithHead;
    GUIDialog_Keyboard_ShowAndVerifyNewPassword* ShowAndVerifyNewPassword;
    GUIDialog_Keyboard_ShowAndVerifyPassword* ShowAndVerifyPassword;
    GUIDialog_Keyboard_ShowAndGetFilter* ShowAndGetFilter;
    GUIDialog_Keyboard_SendTextToActiveKeyboard* SendTextToActiveKeyboard;
    GUIDialog_Keyboard_isKeyboardActivated* isKeyboardActivated;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  typedef bool GUIDialog_Numeric_ShowAndVerifyNewPassword(char &strNewPassword, unsigned int &iMaxStringSize);
  typedef int GUIDialog_Numeric_ShowAndVerifyPassword(char &strPassword, unsigned int &iMaxStringSize, const char *strHeading, int iRetries);
  typedef bool GUIDialog_Numeric_ShowAndVerifyInput(char &strPassword, unsigned int &iMaxStringSize, const char *strHeading, bool bVerifyInput);
  typedef bool GUIDialog_Numeric_ShowAndGetTime(tm &time, const char *strHeading);
  typedef bool GUIDialog_Numeric_ShowAndGetDate(tm &date, const char *strHeading);
  typedef bool GUIDialog_Numeric_ShowAndGetIPAddress(char &strIPAddress, unsigned int &iMaxStringSize, const char *strHeading);
  typedef bool GUIDialog_Numeric_ShowAndGetNumber(char &strInput, unsigned int &iMaxStringSize, const char *strHeading, unsigned int iAutoCloseTimeoutMs);
  typedef bool GUIDialog_Numeric_ShowAndGetSeconds(char &timeString, unsigned int &iMaxStringSize, const char *strHeading);

  struct CB_GUI_Dialog_Numeric
  {
    GUIDialog_Numeric_ShowAndVerifyNewPassword* ShowAndVerifyNewPassword;
    GUIDialog_Numeric_ShowAndVerifyPassword* ShowAndVerifyPassword;
    GUIDialog_Numeric_ShowAndVerifyInput* ShowAndVerifyInput;
    GUIDialog_Numeric_ShowAndGetTime* ShowAndGetTime;
    GUIDialog_Numeric_ShowAndGetDate* ShowAndGetDate;
    GUIDialog_Numeric_ShowAndGetIPAddress* ShowAndGetIPAddress;
    GUIDialog_Numeric_ShowAndGetNumber* ShowAndGetNumber;
    GUIDialog_Numeric_ShowAndGetSeconds* ShowAndGetSeconds;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIDialog_OK_ShowAndGetInputSingleText(const char *heading, const char *text);
  typedef void GUIDialog_OK_ShowAndGetInputLineText(const char *heading, const char *line0, const char *line1, const char *line2);

  struct CB_GUI_Dialog_OK
  {
    GUIDialog_OK_ShowAndGetInputSingleText* ShowAndGetInputSingleText;
    GUIDialog_OK_ShowAndGetInputLineText* ShowAndGetInputLineText;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void* GUIDialog_Progress_New(void* hdl);
  typedef void GUIDialog_Progress_Delete(void* hdl, void* handle);
  typedef void GUIDialog_Progress_Open(void* hdl, void* handle);
  typedef void GUIDialog_Progress_SetHeading(void* hdl, void* handle, const char* heading);
  typedef void GUIDialog_Progress_SetLine(void* hdl, void* handle, unsigned int iLine, const char* line);
  typedef void GUIDialog_Progress_SetCanCancel(void* hdl, void* handle, bool bCanCancel);
  typedef bool GUIDialog_Progress_IsCanceled(void* hdl, void* handle);
  typedef void GUIDialog_Progress_SetPercentage(void* hdl, void* handle, int iPercentage);
  typedef int GUIDialog_Progress_GetPercentage(void* hdl, void* handle);
  typedef void GUIDialog_Progress_ShowProgressBar(void* hdl, void* handle, bool bOnOff);
  typedef void GUIDialog_Progress_SetProgressMax(void* hdl, void* handle, int iMax);
  typedef void GUIDialog_Progress_SetProgressAdvance(void* hdl, void* handle, int nSteps);
  typedef bool GUIDialog_Progress_Abort(void* hdl, void* handle);

  struct CB_GUI_Dialog_Progress
  {
    GUIDialog_Progress_New* New;
    GUIDialog_Progress_Delete* Delete;
    GUIDialog_Progress_Open* Open;
    GUIDialog_Progress_SetHeading* SetHeading;
    GUIDialog_Progress_SetLine* SetLine;
    GUIDialog_Progress_SetCanCancel* SetCanCancel;
    GUIDialog_Progress_IsCanceled* IsCanceled;
    GUIDialog_Progress_SetPercentage* SetPercentage;
    GUIDialog_Progress_GetPercentage* GetPercentage;
    GUIDialog_Progress_ShowProgressBar* ShowProgressBar;
    GUIDialog_Progress_SetProgressMax* SetProgressMax;
    GUIDialog_Progress_SetProgressAdvance* SetProgressAdvance;
    GUIDialog_Progress_Abort* Abort;
  };

  #define IMPL_GUI_PROGRESS_DIALOG \
    private: \
      void* m_DialogHandle;
  //-----------------------------------------------------------------------------


  //=============================================================================
  typedef int GUIDialog_Select_Open(const char *heading, const char *entries[], unsigned int size, int selected, bool autoclose);

  struct CB_GUI_Dialog_Select
  {
    GUIDialog_Select_Open* Open;
  };
  //-----------------------------------------------------------------------------


  //=============================================================================
  typedef int GUIDialog_ContextMenu_Open(const char *heading, const char *entries[], unsigned int size);

  struct CB_GUI_Dialog_ContextMenu
  {
    GUIDialog_ContextMenu_Open* Open;
  };
  //-----------------------------------------------------------------------------


  //=============================================================================
  typedef void GUIDialog_TextViewer_Open(const char *heading, const char *text);

  struct CB_GUI_Dialog_TextViewer
  {
    GUIDialog_TextViewer_Open* Open;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  typedef bool GUIDialog_YesNo_ShowAndGetInputSingleText(const char *heading, const char *text, bool &bCanceled, const char *noLabel, const char *yesLabel);
  typedef bool GUIDialog_YesNo_ShowAndGetInputLineText(const char *heading, const char *line0, const char *line1, const char *line2, const char *noLabel, const char *yesLabel);
  typedef bool GUIDialog_YesNo_ShowAndGetInputLineButtonText(const char *heading, const char *line0, const char *line1, const char *line2, bool &bCanceled, const char *noLabel, const char *yesLabel);

  struct CB_GUI_Dialog_YesNo
  {
    GUIDialog_YesNo_ShowAndGetInputSingleText* ShowAndGetInputSingleText;
    GUIDialog_YesNo_ShowAndGetInputLineText* ShowAndGetInputLineText;
    GUIDialog_YesNo_ShowAndGetInputLineButtonText* ShowAndGetInputLineButtonText;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  /*
   * Internal Structures to have "C"-Style data transfer
   */
  typedef struct ADDON_VideoInfoTag_cast_DATA_STRUCT
  {
    const char* name;
    const char* role;
    int order;
    const char* thumbnail;
  } ADDON_VideoInfoTag_cast_DATA_STRUCT;

  typedef void* GUIListItem_Create(void* hdl, const char* label, const char* label2, const char* iconImage, const char* thumbnailImage, const char* path);
  typedef void GUIListItem_Destroy(void* hdl, void* handle);
  typedef void GUIListItem_GetLabel(void* hdl, void* handle, char& label, unsigned int &iMaxStringSize);
  typedef void GUIListItem_SetLabel(void* hdl, void* handle, const char* label);
  typedef void GUIListItem_GetLabel2(void* hdl, void* handle, char& label, unsigned int& iMaxStringSize);
  typedef void GUIListItem_SetLabel2(void* hdl, void* handle, const char* label);
  typedef void GUIListItem_GetIconImage(void* hdl, void* handle, char& image, unsigned int &iMaxStringSize);
  typedef void GUIListItem_SetIconImage(void* hdl, void* handle, const char* image);
  typedef void GUIListItem_GetOverlayImage(void* hdl, void* handle, char& image, unsigned int& iMaxStringSize);
  typedef void GUIListItem_SetOverlayImage(void* hdl, void* handle, unsigned int image, bool bOnOff);
  typedef void GUIListItem_SetThumbnailImage(void* hdl, void* handle, const char* image);
  typedef void GUIListItem_SetArt(void* hdl, void* handle, const char* type, const char* url);
  typedef char* GUIListItem_GetArt(void* hdl, void* handle, const char* type);
  typedef void GUIListItem_SetArtFallback(void* hdl, void* handle, const char* from, const char* to);
  typedef void GUIListItem_SetLabel(void* hdl, void* handle, const char* label);
  typedef bool GUIListItem_HasArt(void* hdl, void* handle, const char *type);
  typedef void GUIListItem_Select(void* hdl, void* handle, bool bOnOff);
  typedef bool GUIListItem_IsSelected(void* hdl, void* handle);
  typedef bool GUIListItem_HasIcon(void* hdl, void* handle);
  typedef bool GUIListItem_HasOverlay(void* hdl, void* handle);
  typedef bool GUIListItem_IsFileItem(void* hdl, void* handle);
  typedef bool GUIListItem_IsFolder(void* hdl, void* handle);
  typedef void GUIListItem_SetProperty(void* hdl, void* handle, const char *key, const char *value);
  typedef void GUIListItem_GetProperty(void* hdl, void* handle, const char* key, char& property, unsigned int &iMaxStringSize);
  typedef void GUIListItem_ClearProperty(void* hdl, void* handle, const char* key);
  typedef void GUIListItem_ClearProperties(void* hdl, void* handle);
  typedef bool GUIListItem_HasProperties(void* hdl, void* handle);
  typedef bool GUIListItem_HasProperty(void* hdl, void* handle, const char* key);
  typedef void GUIListItem_SetPath(void* hdl, void* handle, const char* path);
  typedef char* GUIListItem_GetPath(void* hdl, void* handle);
  typedef char* GUIListItem_GetDescription(void* hdl, void* handle);
  typedef int GUIListItem_GetDuration(void* hdl, void* handle);
  typedef void GUIListItem_SetSubtitles(void* hdl, void* handle, const char** streams, unsigned int entries);
  typedef void GUIListItem_SetMimeType(void* hdl, void* handle, const char* mimetype);
  typedef void GUIListItem_SetContentLookup(void* hdl, void* handle, bool enable);
  typedef void GUIListItem_AddContextMenuItems(void* hdl, void* handle, const char** streams[2], unsigned int entries, bool replaceItems);
  typedef void GUIListItem_AddStreamInfo(void* hdl, void* handle, const char* cType, const char** dictionary[2], unsigned int entries);
  typedef void GUIListItem_SetMusicInfo(void* hdl, void* handle, unsigned int type, void* data, unsigned int entries);
  typedef void GUIListItem_SetVideoInfo(void* hdl, void* handle, unsigned int type, void* data, unsigned int entries);
  typedef void GUIListItem_SetPictureInfo(void* hdl, void* handle, unsigned int type, void* data, unsigned int entries);

  struct CB_GUI_ListItem
  {
    GUIListItem_Create* Create;
    GUIListItem_Destroy* Destroy;
    GUIListItem_GetLabel* GetLabel;
    GUIListItem_SetLabel* SetLabel;
    GUIListItem_GetLabel2* GetLabel2;
    GUIListItem_SetLabel2* SetLabel2;
    GUIListItem_GetIconImage* GetIconImage;
    GUIListItem_SetIconImage* SetIconImage;
    GUIListItem_GetOverlayImage* GetOverlayImage;
    GUIListItem_SetOverlayImage* SetOverlayImage;
    GUIListItem_SetThumbnailImage* SetThumbnailImage;
    GUIListItem_SetArt* SetArt;
    GUIListItem_GetArt* GetArt;
    GUIListItem_SetArtFallback* SetArtFallback;
    GUIListItem_HasArt* HasArt;
    GUIListItem_Select* Select;
    GUIListItem_IsSelected* IsSelected;
    GUIListItem_HasIcon* HasIcon;
    GUIListItem_HasOverlay* HasOverlay;
    GUIListItem_IsFileItem* IsFileItem;
    GUIListItem_IsFolder* IsFolder;
    GUIListItem_SetProperty* SetProperty;
    GUIListItem_GetProperty* GetProperty;
    GUIListItem_ClearProperty* ClearProperty;
    GUIListItem_ClearProperties* ClearProperties;
    GUIListItem_HasProperties* HasProperties;
    GUIListItem_HasProperty* HasProperty;
    GUIListItem_SetPath* SetPath;
    GUIListItem_GetPath* GetPath;
    GUIListItem_GetDuration* GetDuration;
    GUIListItem_SetSubtitles* SetSubtitles;
    GUIListItem_SetMimeType* SetMimeType;
    GUIListItem_SetContentLookup* SetContentLookup;
    GUIListItem_AddContextMenuItems* AddContextMenuItems;
    GUIListItem_AddStreamInfo* AddStreamInfo;
    GUIListItem_SetMusicInfo* SetMusicInfo;
    GUIListItem_SetVideoInfo* SetVideoInfo;
    GUIListItem_SetPictureInfo* SetPictureInfo;
  };

  #define IMPL_ADDON_GUI_LIST \
    public: \
      CListItem(void* listItemHandle); \
      const void* GetListItemHandle() const \
      { \
 return m_ListItemHandle; \
      } \
    protected: \
      void* m_ListItemHandle; \
    private: \
      friend class CWindow;
  //-----------------------------------------------------------------------------


  //=============================================================================
  typedef void* GUIWindow_New(void* hdl, const char* xmlFilename, const char* defaultSkin, bool forceFallback, bool asDialog);
  typedef void GUIWindow_Delete(void* hdl, void* handle);
  typedef void GUIWindow_SetCallbacks(void* hdl, void* handle, void* clienthandle,
       bool (*)(void* handle),
       bool (*)(void* handle, int),
       bool (*)(void* handle, int),
       bool (*)(void* handle, int));
  typedef bool GUIWindow_Show(void* hdl, void* handle);
  typedef bool GUIWindow_Close(void* hdl, void* handle);
  typedef bool GUIWindow_DoModal(void* hdl, void* handle);
  typedef bool GUIWindow_SetFocusId(void* hdl, void* handle, int iControlId);
  typedef int GUIWindow_GetFocusId(void* hdl, void* handle);
  typedef void GUIWindow_SetProperty(void* hdl, void* handle, const char* key, const char* value);
  typedef void GUIWindow_SetPropertyInt(void* hdl, void* handle, const char* key, int value);
  typedef void GUIWindow_SetPropertyBool(void* hdl, void* handle, const char* key, bool value);
  typedef void GUIWindow_SetPropertyDouble(void* hdl, void* handle, const char* key, double value);
  typedef void GUIWindow_GetProperty(void* hdl, void* handle, const char *key, char &property, unsigned int &iMaxStringSize);
  typedef int GUIWindow_GetPropertyInt(void* hdl, void* handle, const char  *key);
  typedef bool GUIWindow_GetPropertyBool(void* hdl, void* handle, const char *key);
  typedef double GUIWindow_GetPropertyDouble(void* hdl, void* handle, const char* key);
  typedef void GUIWindow_ClearProperties(void* hdl, void* handle);
  typedef void GUIWindow_ClearProperty(void* hdl, void* handle, const char  *key);
  typedef int GUIWindow_GetListSize(void* hdl, void* handle);
  typedef void GUIWindow_ClearList(void* hdl, void* handle);
  typedef void* GUIWindow_AddItem(void* hdl, void* handle, void* item, int itemPosition);
  typedef void* GUIWindow_AddStringItem(void* hdl, void* handle, const char  *itemName, int itemPosition);
  typedef void GUIWindow_RemoveItem(void* hdl, void* handle, int itemPosition);
  typedef void GUIWindow_RemoveItemFile(void* hdl, void* handle, void* fileItem);
  typedef void* GUIWindow_GetListItem(void* hdl, void* handle, int listPos);
  typedef void GUIWindow_SetCurrentListPosition(void* hdl, void* handle, int listPos);
  typedef int GUIWindow_GetCurrentListPosition(void* hdl, void* handle);
  typedef void GUIWindow_SetControlLabel(void* hdl, void* handle, int controlId, const char* label);
  typedef void GUIWindow_MarkDirtyRegion(void* hdl, void* handle);
  typedef void* GUIWindow_GetControl(void* hdl, void* handle, int controlId);

  struct CB_GUI_Window
  {
    GUIWindow_New* New;
    GUIWindow_Delete* Delete;
    GUIWindow_SetCallbacks* SetCallbacks;
    GUIWindow_Show* Show;
    GUIWindow_Close* Close;
    GUIWindow_DoModal* DoModal;
    GUIWindow_SetFocusId* SetFocusId;
    GUIWindow_GetFocusId* GetFocusId;
    GUIWindow_SetProperty* SetProperty;
    GUIWindow_SetPropertyInt* SetPropertyInt;
    GUIWindow_SetPropertyBool* SetPropertyBool;
    GUIWindow_SetPropertyDouble* SetPropertyDouble;
    GUIWindow_GetProperty* GetProperty;
    GUIWindow_GetPropertyInt* GetPropertyInt;
    GUIWindow_GetPropertyBool* GetPropertyBool;
    GUIWindow_GetPropertyDouble* GetPropertyDouble;
    GUIWindow_ClearProperties* ClearProperties;
    GUIWindow_ClearProperty* ClearProperty;
    GUIWindow_GetListSize* GetListSize;
    GUIWindow_ClearList* ClearList;
    GUIWindow_AddItem* AddItem;
    GUIWindow_AddStringItem* AddStringItem;
    GUIWindow_RemoveItem* RemoveItem;
    GUIWindow_RemoveItemFile* RemoveItemFile;
    GUIWindow_GetListItem* GetListItem;
    GUIWindow_SetCurrentListPosition* SetCurrentListPosition;
    GUIWindow_GetCurrentListPosition* GetCurrentListPosition;
    GUIWindow_SetControlLabel* SetControlLabel;
    GUIWindow_MarkDirtyRegion* MarkDirtyRegion;

    GUIWindow_GetControl* GetControl_Button;
    GUIWindow_GetControl* GetControl_Edit;
    GUIWindow_GetControl* GetControl_FadeLabel;
    GUIWindow_GetControl* GetControl_Image;
    GUIWindow_GetControl* GetControl_Label;
    GUIWindow_GetControl* GetControl_Spin;
    GUIWindow_GetControl* GetControl_RadioButton;
    GUIWindow_GetControl* GetControl_Progress;
    GUIWindow_GetControl* GetControl_RenderAddon;
    GUIWindow_GetControl* GetControl_Slider;
    GUIWindow_GetControl* GetControl_SettingsSlider;
    GUIWindow_GetControl* GetControl_TextBox;

  };

  #define IMPLEMENT_ADDON_GUI_WINDOW \
    protected: \
      void* m_WindowHandle; \
    private: \
      static bool OnInitCB(void* cbhdl); \
      static bool OnClickCB(void* cbhdl, int controlId); \
      static bool OnFocusCB(void* cbhdl, int controlId); \
      static bool OnActionCB(void* cbhdl, int actionId); \
      friend class CControlButton; \
      friend class CControlEdit; \
      friend class CControlImage; \
      friend class CControlFadeLabel; \
      friend class CControlLabel; \
      friend class CControlSpin; \
      friend class CControlProgress; \
      friend class CControlRadioButton; \
      friend class CControlRendering; \
      friend class CControlSlider; \
      friend class CControlSettingsSlider; \
      friend class CControlTextBox;
  //-----------------------------------------------------------------------------


  //============================================================================
  typedef void GUILock();
  typedef void GUIUnlock();
  typedef int GUIGetScreenHeight();
  typedef int GUIGetScreenWidth();
  typedef int GUIGetVideoResolution();
  typedef int GUIGetCurrentWindowDialogId();
  typedef int GUIGetCurrentWindowId();

  struct CB_GUI_General
  {
    GUILock* Lock;
    GUIUnlock* Unlock;
    GUIGetScreenHeight* GetScreenHeight;
    GUIGetScreenWidth* GetScreenWidth;
    GUIGetVideoResolution* GetVideoResolution;
    GUIGetCurrentWindowDialogId* GetCurrentWindowDialogId;
    GUIGetCurrentWindowId* GetCurrentWindowId;
  };

  struct CB_GUI_Controls
  {
    CB_GUI_Control_Button Button;
    CB_GUI_Control_Edit Edit;
    CB_GUI_Control_FadeLabel FadeLabel;
    CB_GUI_Control_Image Image;
    CB_GUI_Control_Label Label;
    CB_GUI_Control_Progress Progress;
    CB_GUI_Control_RadioButton RadioButton;
    CB_GUI_Control_Rendering Rendering;
    CB_GUI_Control_SettingsSlider SettingsSlider;
    CB_GUI_Control_Slider Slider;
    CB_GUI_Control_Spin Spin;
    CB_GUI_Control_TextBox TextBox;
  };

  struct CB_GUI_Dialogs
  {
    CB_GUI_Dialog_ExtendedProgress ExtendedProgress;
    CB_GUI_Dialog_FileBrowser FileBrowser;
    CB_GUI_Dialog_Keyboard Keyboard;
    CB_GUI_Dialog_Numeric Numeric;
    CB_GUI_Dialog_OK OK;
    CB_GUI_Dialog_Progress Progress;
    CB_GUI_Dialog_ContextMenu ContextMenu;
    CB_GUI_Dialog_Select Select;
    CB_GUI_Dialog_TextViewer TextViewer;
    CB_GUI_Dialog_YesNo YesNo;
  };

  struct CB_GUI
  {
    CB_GUI_General General;
    CB_GUI_Controls Control;
    CB_GUI_Dialogs Dialogs;
    CB_GUI_ListItem ListItem;
    CB_GUI_Window Window;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void _trigger_scan(void* hdl);
  typedef void _refresh_button_maps(void* hdl, const char* deviceName, const char* controllerId);

  struct CB_Addon_Peripheral
  {
    _trigger_scan* trigger_scan;
    _refresh_button_maps* refresh_button_maps;
  };
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void _addon_log_msg(void* hdl, const int loglevel, const char *msg);
  typedef void _free_string(void* hdl, char* str);

  struct CB_AddOnLib
  {
    _addon_log_msg* addon_log_msg;
    _free_string* free_string;

    CB_Addon_General General;
    CB_Addon_Audio Audio;
    CB_Addon_Directory Directory;
    CB_Addon_VFS VFS;
    CB_Addon_File File;
    CB_Addon_Network Network;

    CB_AudioEngine AudioEngine;
    CB_AudioEngine_Stream AudioEngineStream;

    CB_GUI GUI;

    CB_PVR PVR;

    CB_Addon_Peripheral Peripheral;

    CB_Player_AddonPlayer AddonPlayer;
    CB_Player_AddonPlayList AddonPlayList;
    CB_Player_AddonInfoTagMusic AddonInfoTagMusic;
    CB_Player_AddonInfoTagVideo AddonInfoTagVideo;

    CB_InputStream InputStream;

  };

  typedef CB_AddOnLib* _register_level(void* HANDLE, int level);
  typedef void _unregister_me(void* HANDLE, void* CB);

} /* namespace KodiAPI */
} /* namespace V2 */

END_NAMESPACE()
