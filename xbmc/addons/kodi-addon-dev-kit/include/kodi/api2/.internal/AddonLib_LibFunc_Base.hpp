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

#include "../definitions.hpp"

#ifdef BUILD_KODI_ADDON
  #include "kodi/AEChannelData.h"
  #include "kodi/AEChannelInfo.h"
  #include "kodi/AEStreamData.h"
#else
  #include "cores/AudioEngine/Utils/AEChannelData.h"
  #include "cores/AudioEngine/Utils/AEChannelInfo.h"
  #include "cores/AudioEngine/Utils/AEStreamData.h"
#endif

#include "kodi/xbmc_pvr_types.h"
#include "kodi/kodi_adsp_types.h"

/*
 * This file includes not for add-on developer used parts, but required for the
 * interface to Kodi over the library.
 */

struct DemuxPacket;

namespace V2
{
namespace KodiAPI
{
extern "C"
{

  #define PROCESS_ADDON_CALL(ses, vrp)                                         \
  ({                                                                           \
    uint32_t retCode;                                                          \
    CLockObject lock(ses->m_callMutex);                                        \
    std::unique_ptr<CResponsePacket> vresp(ses->ReadResult(&vrp));             \
    if (!vresp)                                                                \
      throw API_ERR_BUFFER;                                                    \
    vresp->pop(API_UINT32_T, &retCode);                                        \
    retCode;                                                                   \
  })

  #define PROCESS_ADDON_CALL_WITH_RETURN_STRING(ses, vrp, s)                   \
  ({                                                                           \
    uint32_t retCode;                                                          \
    CLockObject lock(ses->m_callMutex);                                        \
    std::unique_ptr<CResponsePacket> vresp(ses->ReadResult(&vrp));             \
    if (!vresp)                                                                \
      throw API_ERR_BUFFER;                                                    \
    vresp->pop(API_UINT32_T, &retCode);                                        \
    vresp->pop(API_STRING, s);                                                 \
    retCode;                                                                   \
  })

  #define PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN_AND_STRING(ses, vrp, s1, s2)  \
  ({                                                                           \
    uint32_t retCode;                                                          \
    CLockObject lock(ses->m_callMutex);                                        \
    std::unique_ptr<CResponsePacket> vresp(ses->ReadResult(&vrp));             \
    if (!vresp)                                                                \
      throw API_ERR_BUFFER;                                                    \
    vresp->pop(API_UINT32_T, &retCode);                                        \
    vresp->pop(API_BOOLEAN, s1);                                               \
    vresp->pop(API_STRING, s2);                                                \
    retCode;                                                                   \
  })

  #define PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(ses, vrp, s)                  \
  ({                                                                           \
    uint32_t retCode;                                                          \
    CLockObject lock(ses->m_callMutex);                                        \
    std::unique_ptr<CResponsePacket> vresp(ses->ReadResult(&vrp));             \
    if (!vresp)                                                                \
      throw API_ERR_BUFFER;                                                    \
    vresp->pop(API_UINT32_T, &retCode);                                        \
    vresp->pop(API_BOOLEAN, s);                                                \
    retCode;                                                                   \
  })

  #define PROCESS_ADDON_CALL_WITH_RETURN_TWO_BOOLEAN(ses, vrp, s1, s2)         \
  ({                                                                           \
    uint32_t retCode;                                                          \
    CLockObject lock(ses->m_callMutex);                                        \
    std::unique_ptr<CResponsePacket> vresp(ses->ReadResult(&vrp));             \
    if (!vresp)                                                                \
      throw API_ERR_BUFFER;                                                    \
    vresp->pop(API_UINT32_T, &retCode);                                        \
    vresp->pop(API_BOOLEAN, s1);                                               \
    vresp->pop(API_BOOLEAN, s2);                                               \
    retCode;                                                                   \
  })

  #define PROCESS_ADDON_CALL_WITH_RETURN_INTEGER(ses, vrp, s)                  \
  ({                                                                           \
    uint32_t retCode;                                                          \
    CLockObject lock(ses->m_callMutex);                                        \
    std::unique_ptr<CResponsePacket> vresp(ses->ReadResult(&vrp));             \
    if (!vresp)                                                                \
      throw API_ERR_BUFFER;                                                    \
    vresp->pop(API_UINT32_T, &retCode);                                        \
    vresp->pop(API_INT, s);                                                    \
    retCode;                                                                   \
  })

  #define PROCESS_ADDON_CALL_WITH_RETURN_UNSIGNED_INTEGER(ses, vrp, s)         \
  ({                                                                           \
    uint32_t retCode;                                                          \
    CLockObject lock(ses->m_callMutex);                                        \
    std::unique_ptr<CResponsePacket> vresp(ses->ReadResult(&vrp));             \
    if (!vresp)                                                                \
      throw API_ERR_BUFFER;                                                    \
    vresp->pop(API_UINT32_T, &retCode);                                        \
    vresp->pop(API_UNSIGNED_INT, s);                                           \
    retCode;                                                                   \
  })

  #define PROCESS_ADDON_CALL_WITH_RETURN_LONG(ses, vrp, s)                     \
  ({                                                                           \
    uint32_t retCode;                                                          \
    CLockObject lock(ses->m_callMutex);                                        \
    std::unique_ptr<CResponsePacket> vresp(ses->ReadResult(&vrp));             \
    if (!vresp)                                                                \
      throw API_ERR_BUFFER;                                                    \
    vresp->pop(API_UINT32_T, &retCode);                                        \
    vresp->pop(API_LONG, s);                                                   \
    retCode;                                                                   \
  })

  #define PROCESS_ADDON_CALL_WITH_RETURN_FLOAT(ses, vrp, s)                    \
  ({                                                                           \
    uint32_t retCode;                                                          \
    CLockObject lock(ses->m_callMutex);                                        \
    std::unique_ptr<CResponsePacket> vresp(ses->ReadResult(&vrp));             \
    if (!vresp)                                                                \
      throw API_ERR_BUFFER;                                                    \
    vresp->pop(API_UINT32_T, &retCode);                                        \
    vresp->pop(API_FLOAT, s);                                                  \
    retCode;                                                                   \
  })

  #define PROCESS_ADDON_CALL_WITH_RETURN_DOUBLE(ses, vrp, s)                   \
  ({                                                                           \
    uint32_t retCode;                                                          \
    CLockObject lock(ses->m_callMutex);                                        \
    std::unique_ptr<CResponsePacket> vresp(ses->ReadResult(&vrp));             \
    if (!vresp)                                                                \
      throw API_ERR_BUFFER;                                                    \
    vresp->pop(API_UINT32_T, &retCode);                                        \
    vresp->pop(API_DOUBLE, s);                                                 \
    retCode;                                                                   \
  })

  #define PROCESS_ADDON_CALL_WITH_RETURN_POINTER(ses, vrp, s)                  \
  ({                                                                           \
    uint32_t retCode;                                                          \
    CLockObject lock(ses->m_callMutex);                                        \
    std::unique_ptr<CResponsePacket> vresp(ses->ReadResult(&vrp));             \
    if (!vresp)                                                                \
      throw API_ERR_BUFFER;                                                    \
    vresp->pop(API_UINT32_T, &retCode);                                        \
    vresp->pop(API_UINT64_T, s);                                               \
    retCode;                                                                   \
  })

  #define PROCESS_ADDON_RETURN_CALL(r)                                         \
  ({                                                                           \
    uint32_t retValue = r;                                                     \
    resp.init(req.getRequestID());                                             \
    resp.push(API_UINT32_T, &retValue);                                        \
    resp.finalise();                                                           \
  })

  #define PROCESS_ADDON_RETURN_CALL_WITH_STRING(r, s)                          \
  ({                                                                           \
    uint32_t retValue = r;                                                     \
    resp.init(req.getRequestID());                                             \
    resp.push(API_UINT32_T, &retValue);                                        \
    resp.push(API_STRING, s);                                                  \
    resp.finalise();                                                           \
  })

  #define PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN_AND_STRING(r, s1, s2)         \
  ({                                                                           \
    uint32_t retValue = r;                                                     \
    resp.init(req.getRequestID());                                             \
    resp.push(API_UINT32_T, &retValue);                                        \
    resp.push(API_BOOLEAN, s1);                                                \
    resp.push(API_STRING, s2);                                                 \
    resp.finalise();                                                           \
  })

  #define PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(r, s)                         \
  ({                                                                           \
    uint32_t retValue = r;                                                     \
    resp.init(req.getRequestID());                                             \
    resp.push(API_UINT32_T, &retValue);                                        \
    resp.push(API_BOOLEAN, s);                                                 \
    resp.finalise();                                                           \
  })

  #define PROCESS_ADDON_RETURN_CALL_WITH_TWO_BOOLEAN(r, s1, s2)                \
  ({                                                                           \
    uint32_t retValue = r;                                                     \
    resp.init(req.getRequestID());                                             \
    resp.push(API_UINT32_T, &retValue);                                        \
    resp.push(API_BOOLEAN, s1);                                                \
    resp.push(API_BOOLEAN, s2);                                                \
    resp.finalise();                                                           \
  })

  #define PROCESS_ADDON_RETURN_CALL_WITH_INT(r, s)                             \
  ({                                                                           \
    uint32_t retValue = r;                                                     \
    resp.init(req.getRequestID());                                             \
    resp.push(API_UINT32_T, &retValue);                                        \
    resp.push(API_INT, s);                                                     \
    resp.finalise();                                                           \
  })

  #define PROCESS_ADDON_RETURN_CALL_WITH_UNSIGNED_INT(r, s)                    \
  ({                                                                           \
    uint32_t retValue = r;                                                     \
    resp.init(req.getRequestID());                                             \
    resp.push(API_UINT32_T, &retValue);                                        \
    resp.push(API_UNSIGNED_INT, s);                                            \
    resp.finalise();                                                           \
  })

  #define PROCESS_ADDON_RETURN_CALL_WITH_DOUBLE(r, s)                          \
  ({                                                                           \
    uint32_t retValue = r;                                                     \
    resp.init(req.getRequestID());                                             \
    resp.push(API_UINT32_T, &retValue);                                        \
    resp.push(API_DOUBLE, s);                                                  \
    resp.finalise();                                                           \
  })

  #define PROCESS_ADDON_RETURN_CALL_WITH_LONG(r, s)                            \
  ({                                                                           \
    uint32_t retValue = r;                                                     \
    resp.init(req.getRequestID());                                             \
    resp.push(API_UINT32_T, &retValue);                                        \
    resp.push(API_LONG, s);                                                    \
    resp.finalise();                                                           \
  })

  #define PROCESS_ADDON_RETURN_CALL_WITH_FLOAT(r, s)                           \
  ({                                                                           \
    uint32_t retValue = r;                                                     \
    resp.init(req.getRequestID());                                             \
    resp.push(API_UINT32_T, &retValue);                                        \
    resp.push(API_FLOAT, s);                                                   \
    resp.finalise();                                                           \
  })

  #define PROCESS_ADDON_RETURN_CALL_WITH_POINTER(r, s)                         \
  ({                                                                           \
    uint32_t retValue = r;                                                     \
    uint64_t retPointer = (uint64_t)s;                                         \
    resp.init(req.getRequestID());                                             \
    resp.push(API_UINT32_T, &retValue);                                        \
    resp.push(API_UINT64_T, &retPointer);                                      \
    resp.finalise();                                                           \
  })

  #define PROCESS_HANDLE_EXCEPTION                                             \
    catch (const std::out_of_range& oor)                                       \
    {                                                                          \
      g_interProcess.Log(ADDON_LOG_ERROR, StringUtils::Format("Binary AddOn - %s: Thread out of Range error: %s", __FUNCTION__, oor.what()).c_str()); \
      std::cerr << __FUNCTION__ << " - Thread out of Range error: " << oor.what() << '\n'; \
      exit(1);                                                                 \
    }                                                                          \
    catch (uint32_t err)                                                       \
    {                                                                          \
      g_interProcess.Log(ADDON_LOG_ERROR, StringUtils::Format("Binary AddOn - %s: Addon received error after send of log entry to Kodi: %s", __FUNCTION__, errorTranslator[err].errorName).c_str()); \
    }                                                                          \

  typedef char*   _get_addon_info(void* hdl, const char* id);
  typedef bool    _get_setting(void* hdl, const char* settingName, void *settingValue, bool global);
  typedef void    _open_settings_dialog(void* hdl);
  typedef void    _queue_notification(void* hdl, const queue_msg type, const char *msg);
  typedef void    _queue_notification_from_type(void* hdl, const queue_msg type, const char* aCaption, const char* aDescription, unsigned int displayTime, bool withSound, unsigned int messageTime);
  typedef void    _queue_notification_with_image(void* hdl, const char* aImageFile, const char* aCaption, const char* aDescription, unsigned int displayTime, bool withSound, unsigned int messageTime);
  typedef void    _get_md5(const char* text, char& md5);
  typedef char*   _unknown_to_utf8(void* hdl, const char* str, bool &ret, bool failOnBadChar);
  typedef char*   _get_localized_string(void* hdl, long dwCode);
  typedef void    _get_language(void* hdl, char& language, unsigned int& iMaxStringSize, int format, bool region);
  typedef void    _get_dvd_menu_language(void* hdl, char &language, unsigned int &iMaxStringSize);
  typedef bool    _start_server(void* hdl, int typ, bool start, bool wait);
  typedef void    _audio_suspend(void* hdl);
  typedef void    _audio_resume(void* hdl);
  typedef float   _get_volume(void* hdl, bool percentage);
  typedef void    _set_volume(void* hdl, float value, bool isPercentage);
  typedef bool    _is_muted(void* hdl);
  typedef void    _toggle_mute(void* hdl);
  typedef long    _get_optical_state(void* hdl);
  typedef bool    _eject_optical_drive(void* hdl);
  typedef void    _kodi_version(void* hdl, char*& compile_name, int& major, int& minor, char*& revision, char*& tag, char*& tagversion);
  typedef void    _kodi_quit(void* hdl);
  typedef void    _htpc_shutdown(void* hdl);
  typedef void    _htpc_restart(void* hdl);
  typedef void    _execute_script(void* hdl, const char* script);
  typedef void    _execute_builtin(void* hdl, const char* function, bool wait);
  typedef char*   _execute_jsonrpc(void* hdl, const char* jsonrpccommand);
  typedef char*   _get_region(void* hdl, const char* id);
  typedef long    _get_free_mem(void* hdl);
  typedef int     _get_global_idle_time(void* hdl);
  typedef char*   _translate_path(void* hdl, const char* path);

  typedef struct CB_AddonLib_General
  {
    _get_addon_info*                get_addon_info;
    _get_setting*                   get_setting;
    _open_settings_dialog*          open_settings_dialog;
    _queue_notification*            queue_notification;
    _queue_notification_from_type*  queue_notification_from_type;
    _queue_notification_with_image* queue_notification_with_image;
    _get_md5*                       get_md5;
    _unknown_to_utf8*               unknown_to_utf8;
    _get_localized_string*          get_localized_string;
    _get_language*                  get_language;
    _get_dvd_menu_language*         get_dvd_menu_language;
    _start_server*                  start_server;
    _audio_suspend*                 audio_suspend;
    _audio_resume*                  audio_resume;
    _get_volume*                    get_volume;
    _set_volume*                    set_volume;
    _is_muted*                      is_muted;
    _toggle_mute*                   toggle_mute;
    _get_optical_state*             get_optical_state;
    _eject_optical_drive*           eject_optical_drive;
    _kodi_version*                  kodi_version;
    _kodi_quit*                     kodi_quit;
    _htpc_shutdown*                 htpc_shutdown;
    _htpc_restart*                  htpc_restart;
    _execute_script*                execute_script;
    _execute_builtin*               execute_builtin;
    _execute_jsonrpc*               execute_jsonrpc;
    _get_region*                    get_region;
    _get_free_mem*                  get_free_mem;
    _get_global_idle_time*          get_global_idle_time;
    _translate_path*                translate_path;
  } CB_AddonLib_General;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef KODI_HANDLE _soundplay_get_handle(void* HANDLE, const char *filename);
  typedef void _soundplay_release_handle(void* HANDLE, KODI_HANDLE sndHandle);
  typedef void _soundplay_play(void* HANDLE, KODI_HANDLE sndHandle, bool waitUntilEnd);
  typedef void _soundplay_stop(void* HANDLE, KODI_HANDLE sndHandle);
  typedef bool _soundplay_is_playing(void* HANDLE, KODI_HANDLE sndHandle);
  typedef void _soundplay_set_channel(void* HANDLE, KODI_HANDLE sndHandle, audio_channel channel);
  typedef audio_channel _soundplay_get_channel(void* HANDLE, KODI_HANDLE sndHandle);
  typedef void _soundplay_set_volume(void* HANDLE, KODI_HANDLE sndHandle, float volume);
  typedef float _soundplay_get_volume(void* HANDLE, KODI_HANDLE sndHandle);

  typedef struct CB_AddonLib_Audio
  {
    _soundplay_get_handle*      soundplay_get_handle;
    _soundplay_release_handle*  soundplay_release_handle;
    _soundplay_play*            soundplay_play;
    _soundplay_stop*            soundplay_stop;
    _soundplay_is_playing*      soundplay_is_playing;
    _soundplay_set_channel*     soundplay_set_channel;
    _soundplay_get_channel*     soundplay_get_channel;
    _soundplay_set_volume*      soundplay_set_volume;
    _soundplay_get_volume*      soundplay_get_volume;
  } CB_AddonLib_Audio;

  #define IMPL_ADDONSOUNDPLAY                                                    \
    private:                                                                     \
      void*     m_PlayHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef KodiAPI::kodi_codec _get_codec_by_name(void* HANDLE, const char* strCodecName);
  typedef DemuxPacket* _allocate_demux_packet(void* HANDLE, int iDataSize);
  typedef void _free_demux_packet(void* HANDLE, DemuxPacket* pPacket);

  typedef struct CB_Addon_Codec
  {
    _get_codec_by_name*             get_codec_by_name;
    _allocate_demux_packet*         allocate_demux_packet;
    _free_demux_packet*             free_demux_packet;
  } CB_AddonLib_Codec;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef bool    _can_open_directory(void* HANDLE, const char* strURL);
  typedef bool    _create_directory(void* HANDLE, const char* strPath);
  typedef bool    _directory_exists(void* HANDLE, const char* strPath);
  typedef bool    _remove_directory(void* HANDLE, const char* strPath);

  typedef struct CB_Addon_Directory
  {
    _can_open_directory*            can_open_directory;
    _create_directory*              create_directory;
    _directory_exists*              directory_exists;
    _remove_directory*              remove_directory;
  } CB_AddonLib_Directory;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void*   _open_file(void* hdl, const char* strFileName, unsigned int flags);
  typedef void*   _open_file_for_write(void* hdl, const char* strFileName, bool bOverWrite);
  typedef ssize_t _read_file(void* hdl, void* file, void* lpBuf, size_t uiBufSize);
  typedef bool    _read_file_string(void* hdl, void* file, char *szLine, int iLineLength);
  typedef ssize_t _write_file(void* hdl, void* file, const void* lpBuf, size_t uiBufSize);
  typedef void    _flush_file(void* hdl, void* file);
  typedef int64_t _seek_file(void* hdl, void* file, int64_t iFilePosition, int iWhence);
  typedef int     _truncate_file(void* hdl, void* file, int64_t iSize);
  typedef int64_t _get_file_position(void* hdl, void* file);
  typedef int64_t _get_file_length(void* hdl, void* file);
  typedef void    _close_file(void* hdl, void* file);
  typedef int     _get_file_chunk_size(void* hdl, void* file);
  typedef bool    _file_exists(void* hdl, const char *strFileName, bool bUseCache);
  typedef int     _stat_file(void* hdl, const char *strFileName, struct __stat64* buffer);
  typedef bool    _delete_file(void* hdl, const char *strFileName);
  typedef char*   _get_file_md5(void* hdl, const char* strFileName);
  typedef char*   _get_cache_thumb_name(void* hdl, const char* strFileName);
  typedef char*   _make_legal_filename(void* hdl, const char* strFileName);
  typedef char*   _make_legal_path(void* hdl, const char* strPath);

  typedef struct CB_AddonLib_File
  {
    _open_file*                     open_file;
    _open_file_for_write*           open_file_for_write;
    _read_file*                     read_file;
    _read_file_string*              read_file_string;
    _write_file*                    write_file;
    _flush_file*                    flush_file;
    _seek_file*                     seek_file;
    _truncate_file*                 truncate_file;
    _get_file_position*             get_file_position;
    _get_file_length*               get_file_length;
    _close_file*                    close_file;
    _get_file_chunk_size*           get_file_chunk_size;
    _file_exists*                   file_exists;
    _stat_file*                     stat_file;
    _delete_file*                   delete_file;
    _get_file_md5*                  get_file_md5;
    _get_cache_thumb_name*          get_cache_thumb_name;
    _make_legal_filename*           make_legal_filename;
    _make_legal_path*               make_legal_path;
  } CB_AddonLib_File;

 #define IMPL_FILE                                                             \
   private:                                                                    \
     void*         m_pFile;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef bool    _wake_on_lan(void* HANDLE, const char *mac);
  typedef void    _get_ip_address(void* HANDLE, char &ip, unsigned int &iMaxStringSize);
  typedef char*   _dns_lookup(void* HANDLE, const char* url, bool& ret);
  typedef char*   _url_encode(void* HANDLE, const char* url);

  typedef struct CB_Addon_Network
  {
    _wake_on_lan*                   wake_on_lan;
    _get_ip_address*                get_ip_address;
    _dns_lookup*                    dns_lookup;
    _url_encode*                    url_encode;
  } CB_AddonLib_Network;
  //----------------------------------------------------------------------------


  //============================================================================
  struct VFSDirEntry
  {
    char* label;             //!< item label
    char* path;              //!< item path
    bool folder;             //!< Item is a folder
    uint64_t size;           //!< Size of file represented by item
  };

  typedef bool    _create_directory(
        void*                     HANDLE,
        const char*               strPath);

  typedef bool    _directory_exists(
        void*                     HANDLE,
        const char*               strPath);

  typedef bool    _remove_directory(
        void*                     HANDLE,
        const char*               strPath);

  typedef bool    _get_vfs_directory(
        void*                     HANDLE,
        const char*               strPath,
        const char*               mask,
        VFSDirEntry**             items,
        unsigned int*             num_items);

  typedef void    _free_vfs_directory(
        void*                     HANDLE,
        VFSDirEntry*              items,
        unsigned int              num_items);

  typedef struct CB_Addon_VFS
  {
    _create_directory*              create_directory;
    _directory_exists*              directory_exists;
    _remove_directory*              remove_directory;
    _get_vfs_directory*             get_vfs_directory;
    _free_vfs_directory*            free_vfs_directory;
  } CB_AddonLib_VFS;

  #define IMPL_VFS_DIR_ENTRY                                                   \
    private:                                                                   \
      std::string m_label;                                                     \
      std::string m_path;                                                      \
      bool m_bFolder;                                                          \
      int64_t m_size;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void _add_dsp_menu_hook(void *hdl, AE_DSP_MENUHOOK *hook);
  typedef void _remove_dsp_menu_hook(void *hdl, AE_DSP_MENUHOOK *hook);
  typedef void _register_dsp_mode(void *hdl, AE_DSP_MODES::AE_DSP_MODE *mode);
  typedef void _unregister_dsp_Mode(void *hdl, AE_DSP_MODES::AE_DSP_MODE *mode);
  typedef bool _get_current_sink_format(void* hdl, AudioEngineFormat* sinkFormat);
  typedef AEStreamHandle* _make_stream(void* hdl, AudioEngineFormat Format, unsigned int Options);
  typedef void _free_stream(void* hdl, AEStreamHandle* StreamHandle);

  typedef struct CB_AudioEngineLib
  {
    _add_dsp_menu_hook*             add_dsp_menu_hook;
    _remove_dsp_menu_hook*          remove_dsp_menu_hook;

    _register_dsp_mode*             register_dsp_mode;
    _unregister_dsp_Mode*           unregister_dsp_Mode;

    _get_current_sink_format*       get_current_sink_format;

    _make_stream*                   make_stream;
    _free_stream*                   free_stream;
  } CB_AudioEngineLib;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef unsigned int _Stream_GetSpace(void *addonData, AEStreamHandle *handle);
  typedef unsigned int _Stream_AddData(void *addonData, AEStreamHandle *handle, uint8_t* const *Data, unsigned int Offset, unsigned int Frames);
  typedef double  _Stream_GetDelay(void *addonData, AEStreamHandle *handle);
  typedef bool    _Stream_IsBuffering(void *addonData, AEStreamHandle *handle);
  typedef double  _Stream_GetCacheTime(void *addonData, AEStreamHandle *handle);
  typedef double  _Stream_GetCacheTotal(void *addonData, AEStreamHandle *handle);
  typedef void    _Stream_Pause(void *addonData, AEStreamHandle *handle);
  typedef void    _Stream_Resume(void *addonData, AEStreamHandle *handle);
  typedef void    _Stream_Drain(void *addonData, AEStreamHandle *handle, bool Wait);
  typedef bool    _Stream_IsDraining(void *addonData, AEStreamHandle *handle);
  typedef bool    _Stream_IsDrained(void *addonData, AEStreamHandle *handle);
  typedef void    _Stream_Flush(void *addonData, AEStreamHandle *handle);
  typedef float   _Stream_GetVolume(void *addonData, AEStreamHandle *handle);
  typedef void    _Stream_SetVolume(void *addonData, AEStreamHandle *handle, float Volume);
  typedef float   _Stream_GetAmplification(void *addonData, AEStreamHandle *handle);
  typedef void    _Stream_SetAmplification(void *addonData, AEStreamHandle *handle, float Amplify);
  typedef const unsigned int _Stream_GetFrameSize(void *addonData, AEStreamHandle *handle);
  typedef const unsigned int _Stream_GetChannelCount(void *addonData, AEStreamHandle *handle);
  typedef const unsigned int _Stream_GetSampleRate(void *addonData, AEStreamHandle *handle);
  typedef const AEDataFormat _Stream_GetDataFormat(void *addonData, AEStreamHandle *handle);
  typedef double  _Stream_GetResampleRatio(void *addonData, AEStreamHandle *handle);
  typedef void    _Stream_SetResampleRatio(void *addonData, AEStreamHandle *handle, double Ratio);

  typedef struct CB_AudioEngineLib_Stream
  {
    // AudioEngine stream callbacks
    _Stream_GetSpace*               AEStream_GetSpace;
    _Stream_AddData*                AEStream_AddData;
    _Stream_GetDelay*               AEStream_GetDelay;
    _Stream_IsBuffering*            AEStream_IsBuffering;
    _Stream_GetCacheTime*           AEStream_GetCacheTime;
    _Stream_GetCacheTotal*          AEStream_GetCacheTotal;
    _Stream_Pause*                  AEStream_Pause;
    _Stream_Resume*                 AEStream_Resume;
    _Stream_Drain*                  AEStream_Drain;
    _Stream_IsDraining*             AEStream_IsDraining;
    _Stream_IsDrained*              AEStream_IsDrained;
    _Stream_Flush*                  AEStream_Flush;
    _Stream_GetVolume*              AEStream_GetVolume;
    _Stream_SetVolume*              AEStream_SetVolume;
    _Stream_GetAmplification*       AEStream_GetAmplification;
    _Stream_SetAmplification*       AEStream_SetAmplification;
    _Stream_GetFrameSize*           AEStream_GetFrameSize;
    _Stream_GetChannelCount*        AEStream_GetChannelCount;
    _Stream_GetSampleRate*          AEStream_GetSampleRate;
    _Stream_GetDataFormat*          AEStream_GetDataFormat;
    _Stream_GetResampleRatio*       AEStream_GetResampleRatio;
    _Stream_SetResampleRatio*       AEStream_SetResampleRatio;
  } CB_AudioEngineLib_Stream;

#define IMPLEMENT_ADDON_AE_STREAM                                              \
  private:                                                                     \
    void* m_StreamHandle;                                                      \
    unsigned int m_planes;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void _add_menu_hook(void* addonData, PVR_MENUHOOK* hook);
  typedef void _recording(void* addonData, const char* strName, const char* strFileName, bool bOnOff);

  typedef void _transfer_epg_entry(void *addonData, const ADDON_HANDLE handle, const EPG_TAG *epgentry);
  typedef void _transfer_channel_entry(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL *chan);
  typedef void _transfer_channel_group(void* addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP*);
  typedef void _transfer_channel_group_member(void* addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER*);
  typedef void _transfer_timer_entry(void *addonData, const ADDON_HANDLE handle, const PVR_TIMER *timer);
  typedef void _transfer_recording_entry(void *addonData, const ADDON_HANDLE handle, const PVR_RECORDING *recording);

  typedef void _trigger_channel_update(void* HANDLE);
  typedef void _trigger_channel_groups_update(void* HANDLE);
  typedef void _trigger_timer_update(void* HANDLE);
  typedef void _trigger_recording_update(void* HANDLE);
  typedef void _trigger_epg_update(void* HANDLE, unsigned int iChannelUid);

  typedef struct CB_PVRLib
  {
    _add_menu_hook*                 add_menu_hook;
    _recording*                     recording;

    _transfer_epg_entry*            transfer_epg_entry;
    _transfer_channel_entry*        transfer_channel_entry;
    _transfer_channel_group*        transfer_channel_group;
    _transfer_channel_group_member* transfer_channel_group_member;
    _transfer_timer_entry*          transfer_timer_entry;
    _transfer_recording_entry*      transfer_recording_entry;

    _trigger_channel_update*        trigger_channel_update;
    _trigger_channel_groups_update* trigger_channel_groups_update;
    _trigger_timer_update*          trigger_timer_update;
    _trigger_recording_update*      trigger_recording_update;
    _trigger_epg_update*            trigger_epg_update;
  } CB_PVRLib;

  #define IMPL_STREAM_PROPS                         \
    private:                                        \
      stream_vector              *m_streamVector;   \
      std::map<unsigned int, int> m_streamIndex;    \
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

  typedef bool AddonInfoTagMusic_GetFromPlayer(
        void*               addonData,
        PLAYERHANDLE        player,
        AddonInfoTagMusic*  tag);

  typedef void AddonInfoTagMusic_Release(
        void*               addonData,
        AddonInfoTagMusic*  tag);

  typedef struct CB_PlayerLib_AddonInfoTagMusic
  {
    AddonInfoTagMusic_GetFromPlayer*  GetFromPlayer;
    AddonInfoTagMusic_Release*        Release;
  } CB_PlayerLib_AddonInfoTagMusic;

  #define IMPL_ADDON_INFO_TAG_MUSIC                                            \
    private:                                                                   \
      inline void TransferInfoTag(AddonInfoTagMusic& infoTag);                 \
                                                                               \
      PLAYERHANDLE      m_ControlHandle;                                       \
      std::string       m_url;                                                 \
      std::string       m_title;                                               \
      std::string       m_artist;                                              \
      std::string       m_album;                                               \
      std::string       m_albumArtist;                                         \
      std::string       m_genre;                                               \
      int               m_duration;                                            \
      int               m_tracks;                                              \
      int               m_disc;                                                \
      std::string       m_releaseDate;                                         \
      int               m_listener;                                            \
      int               m_playCount;                                           \
      std::string       m_lastPlayed;                                          \
      std::string       m_comment;                                             \
      std::string       m_lyrics;
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
    unsigned int m_duration;
  } AddonInfoTagVideo;

  typedef bool AddonInfoTagVideo_GetFromPlayer(
        void*               addonData,
        PLAYERHANDLE        player,
        AddonInfoTagVideo*  tag);

  typedef void AddonInfoTagVideo_Release(
        void*               addonData,
        AddonInfoTagVideo*  tag);

  typedef struct CB_PlayerLib_AddonInfoTagVideo
  {
    AddonInfoTagVideo_GetFromPlayer*  GetFromPlayer;
    AddonInfoTagVideo_Release*        Release;
  } CB_PlayerLib_AddonInfoTagVideo;

  #define IMPL_ADDON_INFO_TAG_VIDEO                                            \
    private:                                                                   \
      inline void TransferInfoTag(AddonInfoTagVideo& infoTag);                 \
                                                                               \
      PLAYERHANDLE      m_ControlHandle;                                       \
      std::string       m_director;                                            \
      std::string       m_writingCredits;                                      \
      std::string       m_genre;                                               \
      std::string       m_country;                                             \
      std::string       m_tagLine;                                             \
      std::string       m_plotOutline;                                         \
      std::string       m_plot;                                                \
      std::string       m_trailer;                                             \
      std::string       m_pictureURL;                                          \
      std::string       m_title;                                               \
      std::string       m_votes;                                               \
      std::string       m_cast;                                                \
      std::string       m_file;                                                \
      std::string       m_path;                                                \
      std::string       m_IMDBNumber;                                          \
      std::string       m_MPAARating;                                          \
      int               m_year;                                                \
      double            m_rating;                                              \
      int               m_playCount;                                           \
      std::string       m_lastPlayed;                                          \
      std::string       m_originalTitle;                                       \
      std::string       m_premiered;                                           \
      std::string       m_firstAired;                                          \
      unsigned int      m_duration;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef char* AddonPlayer_GetSupportedMedia(
        void*             addonData,
        int               mediaType);

  typedef PLAYERHANDLE AddonPlayer_New(
        void*             addonData);

  typedef void AddonPlayer_Delete(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef void AddonPlayer_SetCallbacks(
        void*             addonData,
        PLAYERHANDLE      handle,
        PLAYERHANDLE      cbhdl,
        void      (*CBOnPlayBackStarted)     (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackEnded)       (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackStopped)     (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackPaused)      (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackResumed)     (PLAYERHANDLE cbhdl),
        void      (*CBOnQueueNextItem)       (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackSpeedChanged)(PLAYERHANDLE cbhdl, int iSpeed),
        void      (*CBOnPlayBackSeek)        (PLAYERHANDLE cbhdl, int iTime, int seekOffset),
        void      (*CBOnPlayBackSeekChapter) (PLAYERHANDLE cbhdl, int iChapter));

  typedef bool AddonPlayer_PlayFile(
        void*             addonData,
        PLAYERHANDLE      handle,
        const char*       item,
        bool              windowed);

  typedef bool AddonPlayer_PlayFileItem(
        void*             addonData,
        PLAYERHANDLE      handle,
        const GUIHANDLE   listitem,
        bool              windowed);

  typedef bool AddonPlayer_PlayList(
        void*             addonData,
        PLAYERHANDLE      handle,
        const PLAYERHANDLE list,
        int               playList,
        bool              windowed,
        int               startpos);

  typedef void AddonPlayer_Stop(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef void AddonPlayer_Pause(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef void AddonPlayer_PlayNext(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef void AddonPlayer_PlayPrevious(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef void AddonPlayer_PlaySelected(
        void*             addonData,
        PLAYERHANDLE      handle,
        int               selected);

  typedef bool AddonPlayer_IsPlaying(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef bool AddonPlayer_IsPlayingAudio(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef bool AddonPlayer_IsPlayingVideo(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef bool AddonPlayer_IsPlayingRDS(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef bool AddonPlayer_GetPlayingFile(
        void*             addonData,
        PLAYERHANDLE      handle,
        char&             file,
        unsigned int&     iMaxStringSize);

  typedef double AddonPlayer_GetTotalTime(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef double AddonPlayer_GetTime(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef void AddonPlayer_SeekTime(
        void*             addonData,
        PLAYERHANDLE      handle,
        double            seekTime);

  typedef bool AddonPlayer_GetAvailableVideoStreams(
        void*             addonData,
        PLAYERHANDLE      handle,
        char**&           streams,
        unsigned int&     entries);

  typedef void AddonPlayer_SetVideoStream(
        void*             addonData,
        PLAYERHANDLE      handle,
        int               iStream);

  typedef bool AddonPlayer_GetAvailableAudioStreams(
        void*             addonData,
        PLAYERHANDLE      handle,
        char**&           streams,
        unsigned int&     entries);

  typedef void AddonPlayer_SetAudioStream(
        void*             addonData,
        PLAYERHANDLE      handle,
        int               iStream);

  typedef bool AddonPlayer_GetAvailableSubtitleStreams(
        void*             addonData,
        PLAYERHANDLE      handle,
        char**&           streams,
        unsigned int&     entries);

  typedef void AddonPlayer_SetSubtitleStream(
        void*             addonData,
        PLAYERHANDLE      handle,
        int               iStream);

  typedef void AddonPlayer_ShowSubtitles(
        void*             addonData,
        PLAYERHANDLE      handle,
        bool              bVisible);

  typedef bool AddonPlayer_GetCurrentSubtitleName(
        void*             addonData,
        PLAYERHANDLE      handle,
        char&             name,
        unsigned int&     iMaxStringSize);

  typedef void AddonPlayer_AddSubtitle(
        void*             addonData,
        PLAYERHANDLE      handle,
        const char*       strSubPath);

  typedef void AddonPlayer_ClearList(
        char**&       path,
        unsigned int  entries);

  typedef struct CB_PlayerLib_AddonPlayer
  {
    AddonPlayer_GetSupportedMedia*            GetSupportedMedia;

    AddonPlayer_New*                          New;
    AddonPlayer_Delete*                       Delete;
    AddonPlayer_SetCallbacks*                 SetCallbacks;

    AddonPlayer_PlayFile*                     PlayFile;
    AddonPlayer_PlayFileItem*                 PlayFileItem;
    AddonPlayer_PlayList*                     PlayList;
    AddonPlayer_Stop*                         Stop;
    AddonPlayer_Pause*                        Pause;
    AddonPlayer_PlayNext*                     PlayNext;
    AddonPlayer_PlayPrevious*                 PlayPrevious;
    AddonPlayer_PlaySelected*                 PlaySelected;
    AddonPlayer_IsPlaying*                    IsPlaying;
    AddonPlayer_IsPlayingAudio*               IsPlayingAudio;
    AddonPlayer_IsPlayingVideo*               IsPlayingVideo;
    AddonPlayer_IsPlayingRDS*                 IsPlayingRDS;

    AddonPlayer_GetPlayingFile*               GetPlayingFile;
    AddonPlayer_GetTotalTime*                 GetTotalTime;
    AddonPlayer_GetTime*                      GetTime;
    AddonPlayer_SeekTime*                     SeekTime;

    AddonPlayer_GetAvailableVideoStreams*     GetAvailableVideoStreams;
    AddonPlayer_SetVideoStream*               SetVideoStream;
    AddonPlayer_GetAvailableAudioStreams*     GetAvailableAudioStreams;
    AddonPlayer_SetAudioStream*               SetAudioStream;

    AddonPlayer_GetAvailableSubtitleStreams*  GetAvailableSubtitleStreams;
    AddonPlayer_SetSubtitleStream*            SetSubtitleStream;
    AddonPlayer_ShowSubtitles*                ShowSubtitles;
    AddonPlayer_GetCurrentSubtitleName*       GetCurrentSubtitleName;
    AddonPlayer_AddSubtitle*                  AddSubtitle;

    AddonPlayer_ClearList*                    ClearList;
  } CB_PlayerLib_AddonPlayer;

  #define IMPL_ADDON_PLAYER                                                    \
    private:                                                                   \
      friend class CInfoTagMusic;                                              \
      friend class CInfoTagVideo;                                              \
      PLAYERHANDLE      m_ControlHandle;                                       \
      static void CBOnPlayBackStarted(PLAYERHANDLE cbhdl);                     \
      static void CBOnPlayBackEnded(PLAYERHANDLE cbhdl);                       \
      static void CBOnPlayBackStopped(PLAYERHANDLE cbhdl);                     \
      static void CBOnPlayBackPaused(PLAYERHANDLE cbhdl);                      \
      static void CBOnPlayBackResumed(PLAYERHANDLE cbhdl);                     \
      static void CBOnQueueNextItem(PLAYERHANDLE cbhdl);                       \
      static void CBOnPlayBackSpeedChanged(PLAYERHANDLE cbhdl, int iSpeed);    \
      static void CBOnPlayBackSeek(PLAYERHANDLE cbhdl, int iTime, int seekOffset);\
      static void CBOnPlayBackSeekChapter(PLAYERHANDLE cbhdl, int iChapter);
  //----------------------------------------------------------------------------


  //============================================================================
  typedef PLAYERHANDLE AddonPlayList_New(
        void*             addonData,
        int               playlist);

  typedef void AddonPlayList_Delete(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef bool AddonPlayList_LoadPlaylist(
        void*             addonData,
        PLAYERHANDLE      handle,
        const char*       filename,
        int               playList);

  typedef void AddonPlayList_AddItemURL(
        void*             addonData,
        PLAYERHANDLE      handle,
        const char*       url,
        int               index);

  typedef void AddonPlayList_AddItemList(
        void*             addonData,
        PLAYERHANDLE      handle,
        const GUIHANDLE   listitem,
        int               index);

  typedef void AddonPlayList_RemoveItem(
        void*             addonData,
        PLAYERHANDLE      handle,
        const char*       url);

  typedef void AddonPlayList_ClearList(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef int AddonPlayList_GetListSize(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef int AddonPlayList_GetListPosition(
        void*             addonData,
        PLAYERHANDLE      handle);

  typedef void AddonPlayList_Shuffle(
        void*             addonData,
        PLAYERHANDLE      handle,
        bool              shuffle);

  typedef void* AddonPlayList_GetItem(
        void*             addonData,
        PLAYERHANDLE      handle,
        long              position);

  typedef struct CB_PlayerLib_AddonPlayList
  {
    AddonPlayList_New*                      New;
    AddonPlayList_Delete*                   Delete;
    AddonPlayList_LoadPlaylist*             LoadPlaylist;
    AddonPlayList_AddItemURL*               AddItemURL;
    AddonPlayList_AddItemList*              AddItemList;
    AddonPlayList_RemoveItem*               RemoveItem;
    AddonPlayList_ClearList*                ClearList;
    AddonPlayList_GetListSize*              GetListSize;
    AddonPlayList_GetListPosition*          GetListPosition;
    AddonPlayList_Shuffle*                  Shuffle;
    AddonPlayList_GetItem*                  GetItem;
  } CB_PlayerLib_AddonPlayList;

  #define IMPL_ADDON_PLAYLIST                                                  \
    public:                                                                    \
      const GUIHANDLE GetListHandle() const                                    \
      {                                                                        \
        return m_ControlHandle;                                                \
      }                                                                        \
      const AddonPlayListType GetListType() const                              \
      {                                                                        \
        return m_playlist;                                                     \
      }                                                                        \
    private:                                                                   \
      PLAYERHANDLE      m_ControlHandle;                                       \
      AddonPlayListType m_playlist;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Button_SetVisible(
       void         *addonData,
       GUIHANDLE     handle,
       bool          visible);

  typedef void GUIControl_Button_SetEnabled(
       void*         addonData,
       GUIHANDLE     spinhandle,
       bool          enabled);

  typedef void GUIControl_Button_SetLabel(
       void         *addonData,
       GUIHANDLE     handle,
       const char   *label);

  typedef void GUIControl_Button_GetLabel(
       void         *addonData,
       GUIHANDLE     handle,
       char         &label,
       unsigned int &iMaxStringSize);

  typedef void GUIControl_Button_SetLabel2(
       void         *addonData,
       GUIHANDLE     handle,
       const char   *label);

  typedef void GUIControl_Button_GetLabel2(
       void         *addonData,
       GUIHANDLE     handle,
       char         &label,
       unsigned int &iMaxStringSize);

  typedef struct CB_GUILib_Control_Button
  {
    GUIControl_Button_SetVisible*     SetVisible;
    GUIControl_Button_SetEnabled*     SetEnabled;

    GUIControl_Button_SetLabel*       SetLabel;
    GUIControl_Button_GetLabel*       GetLabel;

    GUIControl_Button_SetLabel2*      SetLabel2;
    GUIControl_Button_GetLabel2*      GetLabel2;
  } CB_GUILib_Control_Button;

  #define IMPL_GUI_BUTTON_CONTROL               \
    private:                                    \
      CWindow*          m_Window;               \
      int               m_ControlId;            \
      GUIHANDLE         m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Edit_SetVisible
      (void         *addonData,
       GUIHANDLE     handle,
       bool          visible);

  typedef void GUIControl_Edit_SetEnabled
      (void         *addonData,
       GUIHANDLE     handle,
       bool          enabled);

  typedef void GUIControl_Edit_SetLabel
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *label);

  typedef void GUIControl_Edit_GetLabel
      (void         *addonData,
       GUIHANDLE     handle,
       char         &label,
       unsigned int &iMaxStringSize);

  typedef void GUIControl_Edit_SetText
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *text);

  typedef void GUIControl_Edit_GetText
      (void         *addonData,
       GUIHANDLE     handle,
       char         &text,
       unsigned int &iMaxStringSize);

  typedef void GUIControl_Edit_SetCursorPosition
      (void         *addonData,
       GUIHANDLE     handle,
       unsigned int  iPosition);

  typedef unsigned int GUIControl_Edit_GetCursorPosition
      (void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIControl_Edit_SetInputType
      (void         *addonData,
       GUIHANDLE     handle,
       AddonGUIInputType type,
       const char   *heading);

  typedef struct CB_GUILib_Control_Edit
  {
    GUIControl_Edit_SetVisible*        SetVisible;
    GUIControl_Edit_SetEnabled*        SetEnabled;

    GUIControl_Edit_SetLabel*          SetLabel;
    GUIControl_Edit_GetLabel*          GetLabel;
    GUIControl_Edit_SetText*           SetText;
    GUIControl_Edit_GetText*           GetText;
    GUIControl_Edit_SetCursorPosition* SetCursorPosition;
    GUIControl_Edit_GetCursorPosition* GetCursorPosition;
    GUIControl_Edit_SetInputType*      SetInputType;
  } CB_GUILib_Control_Edit;

  #define IMPL_GUI_EDIT_CONTROL                 \
    private:                                    \
      CWindow*          m_Window;               \
      int               m_ControlId;            \
      GUIHANDLE         m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_FadeLabel_SetVisible(
       void         *addonData,
       GUIHANDLE     handle,
       bool          visible);

  typedef void GUIControl_FadeLabel_AddLabel(
       void         *addonData,
       GUIHANDLE     handle,
       const char   *text);

  typedef void GUIControl_FadeLabel_GetLabel(
       void         *addonData,
       GUIHANDLE     handle,
       char         &text,
       unsigned int &iMaxStringSize);

  typedef void GUIControl_FadeLabel_SetScrolling(
       void*         addonData,
       GUIHANDLE     handle,
       bool          scroll);

  typedef void GUIControl_FadeLabel_Reset(
       void*         addonData,
       GUIHANDLE     handle);

  typedef struct CB_GUILib_Control_FadeLabel
  {
    GUIControl_FadeLabel_SetVisible*       SetVisible;
    GUIControl_FadeLabel_AddLabel*         AddLabel;
    GUIControl_FadeLabel_GetLabel*         GetLabel;
    GUIControl_FadeLabel_SetScrolling*     SetScrolling;
    GUIControl_FadeLabel_Reset*            Reset;
  } CB_GUILib_Control_FadeLabel;

  #define IMPL_GUI_FADELABEL_CONTROL            \
    private:                                    \
      CWindow*          m_Window;               \
      int               m_ControlId;            \
      GUIHANDLE         m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Image_SetVisible(
       void         *addonData,
       GUIHANDLE     handle,
       bool          visible);

  typedef void GUIControl_Image_SetFileName(
       void         *addonData,
       GUIHANDLE     handle,
       const char*   strFileName,
       const bool    useCache);

  typedef void GUIControl_Image_SetColorDiffuse(
       void         *addonData,
       GUIHANDLE     handle,
       uint32_t      colorDiffuse);

  typedef struct CB_GUILib_Control_Image
  {
    GUIControl_Image_SetVisible*       SetVisible;
    GUIControl_Image_SetFileName*      SetFileName;
    GUIControl_Image_SetColorDiffuse*  SetColorDiffuse;
  } CB_GUILib_Control_Image;

  #define IMPL_GUI_IMAGE_CONTROL                \
    private:                                    \
      CWindow*  m_Window;               \
      int               m_ControlId;            \
      GUIHANDLE         m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Label_SetVisible(
       void         *addonData,
       GUIHANDLE     handle,
       bool          visible);

  typedef void GUIControl_Label_SetLabel(
       void         *addonData,
       GUIHANDLE     handle,
       const char   *text);

  typedef void GUIControl_Label_GetLabel(
       void         *addonData,
       GUIHANDLE     handle,
       char         &text,
       unsigned int &iMaxStringSize);

  typedef struct CB_GUILib_Control_Label
  {
    GUIControl_Label_SetVisible*       SetVisible;
    GUIControl_Label_SetLabel*         SetLabel;
    GUIControl_Label_GetLabel*         GetLabel;
  } CB_GUILib_Control_Label;

  #define IMPL_GUI_LABEL_CONTROL                \
    private:                                    \
      CWindow*          m_Window;               \
      int               m_ControlId;            \
      GUIHANDLE         m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Progress_SetVisible(
       void*         addonData,
       GUIHANDLE     spinhandle,
       bool          visible);

  typedef void GUIControl_Progress_SetPercentage
      (void       *addonData,
       GUIHANDLE   handle,
       float       fPercent);

  typedef float GUIControl_Progress_GetPercentage
      (void       *addonData,
       GUIHANDLE   handle);

  typedef struct CB_GUILib_Control_Progress
  {
    GUIControl_Progress_SetVisible*      SetVisible;
    GUIControl_Progress_SetPercentage*   SetPercentage;
    GUIControl_Progress_GetPercentage*   GetPercentage;
  } CB_GUILib_Control_Progress;

  #define IMPL_GUI_PROGRESS_CONTROL             \
    private:                                    \
      CWindow*  m_Window;               \
      int               m_ControlId;            \
      GUIHANDLE         m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_RadioButton_SetVisible
      (void       *addonData,
       GUIHANDLE   handle,
       bool        yesNo);

  typedef void GUIControl_RadioButton_SetEnabled(
       void*         addonData,
       GUIHANDLE     spinhandle,
       bool          enabled);

  typedef void GUIControl_RadioButton_SetLabel
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *text);

  typedef void GUIControl_RadioButton_GetLabel
      (void         *addonData,
       GUIHANDLE     handle,
       char         &text,
       unsigned int &iMaxStringSize);

  typedef void GUIControl_RadioButton_SetSelected
      (void       *addonData,
       GUIHANDLE   handle,
       bool        yesNo);

  typedef bool GUIControl_RadioButton_IsSelected
      (void       *addonData,
       GUIHANDLE   handle);

  typedef struct CB_GUILib_Control_RadioButton
  {
    GUIControl_RadioButton_SetVisible*   SetVisible;
    GUIControl_RadioButton_SetEnabled*   SetEnabled;
    GUIControl_RadioButton_SetLabel*     SetLabel;
    GUIControl_RadioButton_GetLabel*     GetLabel;
    GUIControl_RadioButton_SetSelected*  SetSelected;
    GUIControl_RadioButton_IsSelected*   IsSelected;
  } CB_GUILib_Control_RadioButton;

  #define IMPL_GUI_RADIO_BUTTON_CONTROL         \
    private:                                    \
      CWindow*          m_Window;               \
      int               m_ControlId;            \
      GUIHANDLE         m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIRenderAddon_SetCallbacks
      (void       *addonData, GUIHANDLE handle,
       GUIHANDLE   clienthandle,
       bool        (*createCB)(GUIHANDLE,int,int,int,int,void*),
       void        (*renderCB)(GUIHANDLE),
       void        (*stopCB)(GUIHANDLE),
       bool        (*dirtyCB)(GUIHANDLE));

  typedef void GUIRenderAddon_Delete
      (void       *addonData,
       GUIHANDLE   handle);

  typedef struct CB_GUILib_Control_Rendering
  {
    GUIRenderAddon_SetCallbacks*         SetCallbacks;
    GUIRenderAddon_Delete*               Delete;
  } CB_GUILib_Control_Rendering;

  #define IMPL_GUI_RENDERING_CONTROL                                           \
    private:                                                                   \
      CWindow*          m_Window;                                              \
      int               m_ControlId;                                           \
      GUIHANDLE         m_ControlHandle;                                       \
      static bool OnCreateCB(GUIHANDLE cbhdl, int x, int y, int w, int h, void* device); \
      static void OnRenderCB(GUIHANDLE cbhdl);                                 \
      static void OnStopCB(GUIHANDLE cbhdl);                                   \
      static bool OnDirtyCB(GUIHANDLE cbhdl);
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_SettingsSlider_SetVisible(
       void         *addonData,
       GUIHANDLE     handle,
       bool          yesNo);

  typedef void GUIControl_SettingsSlider_SetEnabled(
       void         *addonData,
       GUIHANDLE     handle,
       bool          yesNo);

  typedef void GUIControl_SettingsSlider_SetText(
       void         *addonData,
       GUIHANDLE     handle,
       const char   *label);

  typedef void GUIControl_SettingsSlider_Reset(
       void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIControl_SettingsSlider_SetIntRange(
       void         *addonData,
       GUIHANDLE     handle,
       int           iStart,
       int           iEnd);

  typedef void GUIControl_SettingsSlider_SetIntValue(
       void         *addonData,
       GUIHANDLE     handle,
       int           iValue);

  typedef int  GUIControl_SettingsSlider_GetIntValue(
       void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIControl_SettingsSlider_SetIntInterval(
       void         *addonData,
       GUIHANDLE     handle,
       int           iInterval);

  typedef void GUIControl_SettingsSlider_SetPercentage(
       void         *addonData,
       GUIHANDLE     handle,
       float         fPercent);

  typedef float GUIControl_SettingsSlider_GetPercentage(
       void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIControl_SettingsSlider_SetFloatRange(
       void         *addonData,
       GUIHANDLE     handle,
       float         fStart,
       float         fEnd);

  typedef void GUIControl_SettingsSlider_SetFloatValue(
       void         *addonData,
       GUIHANDLE     handle,
       float         fValue);

  typedef float GUIControl_SettingsSlider_GetFloatValue(
       void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIControl_SettingsSlider_SetFloatInterval(
       void         *addonData,
       GUIHANDLE     handle,
       float         fInterval);

  typedef struct CB_GUILib_Control_SettingsSlider
  {
    GUIControl_SettingsSlider_SetVisible*          SetVisible;
    GUIControl_SettingsSlider_SetEnabled*          SetEnabled;

    GUIControl_SettingsSlider_SetText*             SetText;
    GUIControl_SettingsSlider_Reset*               Reset;

    GUIControl_SettingsSlider_SetIntRange*         SetIntRange;
    GUIControl_SettingsSlider_SetIntValue*         SetIntValue;
    GUIControl_SettingsSlider_GetIntValue*         GetIntValue;
    GUIControl_SettingsSlider_SetIntInterval*      SetIntInterval;

    GUIControl_SettingsSlider_SetPercentage*       SetPercentage;
    GUIControl_SettingsSlider_GetPercentage*       GetPercentage;

    GUIControl_SettingsSlider_SetFloatRange*       SetFloatRange;
    GUIControl_SettingsSlider_SetFloatValue*       SetFloatValue;
    GUIControl_SettingsSlider_GetFloatValue*       GetFloatValue;
    GUIControl_SettingsSlider_SetFloatInterval*    SetFloatInterval;
  } CB_GUILib_Control_SettingsSlider;

  #define IMPL_GUI_SETTINGS_SLIDER_CONTROL      \
    private:                                    \
      CWindow*  m_Window;                       \
      int               m_ControlId;            \
      GUIHANDLE         m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Slider_SetVisible(
       void         *addonData,
       GUIHANDLE     handle,
       bool          yesNo);

  typedef void GUIControl_Slider_SetEnabled(
       void         *addonData,
       GUIHANDLE     handle,
       bool          yesNo);

  typedef void GUIControl_Slider_Reset(
       void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIControl_Slider_GetDescription(
       void         *addonData,
       GUIHANDLE     handle,
       char         &text,
       unsigned int &iMaxStringSize);

  typedef void GUIControl_Slider_SetIntRange(
       void         *addonData,
       GUIHANDLE     handle,
       int           start,
       int           end);

  typedef void GUIControl_Slider_SetIntValue(
       void         *addonData,
       GUIHANDLE     handle,
       int           value);

  typedef int  GUIControl_Slider_GetIntValue(
       void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIControl_Slider_SetIntInterval(
       void         *addonData,
       GUIHANDLE     handle,
       int           interval);

  typedef void GUIControl_Slider_SetPercentage(
       void         *addonData,
       GUIHANDLE     handle,
       float         percent);

  typedef float GUIControl_Slider_GetPercentage(
       void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIControl_Slider_SetFloatRange(
       void         *addonData,
       GUIHANDLE     handle,
       float         start,
       float         end);

  typedef void GUIControl_Slider_SetFloatValue(
       void         *addonData,
       GUIHANDLE     handle,
       float         value);

  typedef float GUIControl_Slider_GetFloatValue(
       void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIControl_Slider_SetFloatInterval(
       void         *addonData,
       GUIHANDLE     handle,
       float         interval);

  typedef struct CB_GUILib_Control_Slider
  {
    GUIControl_Slider_SetVisible*        SetVisible;
    GUIControl_Slider_SetEnabled*        SetEnabled;

    GUIControl_Slider_Reset*             Reset;
    GUIControl_Slider_GetDescription*    GetDescription;

    GUIControl_Slider_SetIntRange*       SetIntRange;
    GUIControl_Slider_SetIntValue*       SetIntValue;
    GUIControl_Slider_GetIntValue*       GetIntValue;
    GUIControl_Slider_SetIntInterval*    SetIntInterval;

    GUIControl_Slider_SetPercentage*     SetPercentage;
    GUIControl_Slider_GetPercentage*     GetPercentage;

    GUIControl_Slider_SetFloatRange*     SetFloatRange;
    GUIControl_Slider_SetFloatValue*     SetFloatValue;
    GUIControl_Slider_GetFloatValue*     GetFloatValue;
    GUIControl_Slider_SetFloatInterval*  SetFloatInterval;
  } CB_GUILib_Control_Slider;

  #define IMPL_GUI_SLIDER_CONTROL               \
    private:                                    \
      CWindow*          m_Window;               \
      int               m_ControlId;            \
      GUIHANDLE         m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_Spin_SetVisible(
       void*         addonData,
       GUIHANDLE     spinhandle,
       bool          visible);

  typedef void GUIControl_Spin_SetEnabled(
       void*         addonData,
       GUIHANDLE     spinhandle,
       bool          enabled);

  typedef void GUIControl_Spin_SetText(
       void*         addonData,
       GUIHANDLE     spinhandle,
       const char*   text);

  typedef void GUIControl_Spin_Reset(
       void*         addonData,
       GUIHANDLE     spinhandle);

  typedef void GUIControl_Spin_SetType(
       void*         addonData,
       GUIHANDLE     handle,
       int           type);

  typedef void GUIControl_Spin_AddStringLabel(
       void*         addonData,
       GUIHANDLE     handle,
       const char*   label,
       const char*   value);

  typedef void GUIControl_Spin_SetStringValue(
       void*         addonData,
       GUIHANDLE     handle,
       const char*   value);

  typedef void GUIControl_Spin_GetStringValue(
       void*         addonData,
       GUIHANDLE     handle,
       char&         value,
       unsigned int& maxStringSize);

  typedef void GUIControl_Spin_AddIntLabel(
       void*         addonData,
       GUIHANDLE     handle,
       const char*   label,
       int           value);

  typedef void GUIControl_Spin_SetIntRange(
       void*         addonData,
       GUIHANDLE     handle,
       int           start,
       int           end);

  typedef void GUIControl_Spin_SetIntValue(
       void*         addonData,
       GUIHANDLE     handle,
       int           value);

  typedef int  GUIControl_Spin_GetIntValue(
       void*         addonData,
       GUIHANDLE     handle);

  typedef void GUIControl_Spin_SetFloatRange(
       void*         addonData,
       GUIHANDLE     handle,
       float         start,
       float         end);

  typedef void GUIControl_Spin_SetFloatValue(
       void*         addonData,
       GUIHANDLE     handle,
       float         value);

  typedef float GUIControl_Spin_GetFloatValue(
       void*         addonData,
       GUIHANDLE     handle);

  typedef void GUIControl_Spin_SetFloatInterval(
       void*         addonData,
       GUIHANDLE     handle,
       float         interval);

  typedef struct CB_GUILib_Control_Spin
  {
    GUIControl_Spin_SetVisible*          SetVisible;
    GUIControl_Spin_SetEnabled*          SetEnabled;

    GUIControl_Spin_SetText*             SetText;
    GUIControl_Spin_Reset*               Reset;
    GUIControl_Spin_SetType*             SetType;

    GUIControl_Spin_AddStringLabel*      AddStringLabel;
    GUIControl_Spin_SetStringValue*      SetStringValue;
    GUIControl_Spin_GetStringValue*      GetStringValue;

    GUIControl_Spin_AddIntLabel*         AddIntLabel;
    GUIControl_Spin_SetIntRange*         SetIntRange;
    GUIControl_Spin_SetIntValue*         SetIntValue;
    GUIControl_Spin_GetIntValue*         GetIntValue;

    GUIControl_Spin_SetFloatRange*       SetFloatRange;
    GUIControl_Spin_SetFloatValue*       SetFloatValue;
    GUIControl_Spin_GetFloatValue*       GetFloatValue;
    GUIControl_Spin_SetFloatInterval*    SetFloatInterval;
  } CB_GUILib_Control_Spin;

  #define IMPL_GUI_SPIN_CONTROL                 \
    private:                                    \
      CWindow*          m_Window;               \
      int               m_ControlId;            \
      GUIHANDLE         m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIControl_TextBox_SetVisible(
       void         *addonData,
       GUIHANDLE     spinhandle,
       bool          visible);

  typedef void GUIControl_TextBox_Reset(
       void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIControl_TextBox_SetText(
       void         *addonData,
       GUIHANDLE     handle,
       const char*   text);

  typedef void GUIControl_TextBox_GetText(
       void         *addonData,
       GUIHANDLE     handle,
       char         &text,
       unsigned int &iMaxStringSize);

  typedef void GUIControl_TextBox_Scroll(
       void         *addonData,
       GUIHANDLE     handle,
       unsigned int  scroll);

  typedef void GUIControl_TextBox_SetAutoScrolling(
       void         *addonData,
       GUIHANDLE     handle,
       int           delay,
       int           time,
       int           repeat);

  typedef struct CB_GUILib_Control_TextBox
  {
    GUIControl_TextBox_SetVisible*          SetVisible;
    GUIControl_TextBox_Reset*               Reset;
    GUIControl_TextBox_SetText*             SetText;
    GUIControl_TextBox_GetText*             GetText;
    GUIControl_TextBox_Scroll*              Scroll;
    GUIControl_TextBox_SetAutoScrolling*    SetAutoScrolling;
  } CB_GUILib_Control_TextBox;

  #define IMPL_GUI_TEXTBOX_CONTROL              \
    private:                                    \
      CWindow*          m_Window;               \
      int               m_ControlId;            \
      GUIHANDLE         m_ControlHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef GUIHANDLE GUIDialog_ExtendedProgress_New
      (void         *addonData,
       const char   *title);

  typedef void GUIDialog_ExtendedProgress_Delete
      (void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIDialog_ExtendedProgress_Title
      (void         *addonData,
       GUIHANDLE     handle,
       char         &title,
       unsigned int &iMaxStringSize);

  typedef void GUIDialog_ExtendedProgress_SetTitle
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *title);

  typedef void GUIDialog_ExtendedProgress_Text
      (void         *addonData,
       GUIHANDLE     handle,
       char         &text,
       unsigned int &iMaxStringSize);

  typedef void GUIDialog_ExtendedProgress_SetText
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *text);

  typedef bool GUIDialog_ExtendedProgress_IsFinished
      (void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIDialog_ExtendedProgress_MarkFinished
      (void         *addonData,
       GUIHANDLE     handle);

  typedef float GUIDialog_ExtendedProgress_Percentage
      (void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIDialog_ExtendedProgress_SetPercentage
      (void         *addonData,
       GUIHANDLE     handle,
       float         fPercentage);

  typedef void GUIDialog_ExtendedProgress_SetProgress
      (void         *addonData,
       GUIHANDLE     handle,
       int           currentItem,
       int           itemCount);

  typedef struct CB_GUILib_Dialog_ExtendedProgress
  {
    GUIDialog_ExtendedProgress_New*            New;
    GUIDialog_ExtendedProgress_Delete*         Delete;
    GUIDialog_ExtendedProgress_Title*          Title;
    GUIDialog_ExtendedProgress_SetTitle*       SetTitle;
    GUIDialog_ExtendedProgress_Text*           Text;
    GUIDialog_ExtendedProgress_SetText*        SetText;
    GUIDialog_ExtendedProgress_IsFinished*     IsFinished;
    GUIDialog_ExtendedProgress_MarkFinished*   MarkFinished;
    GUIDialog_ExtendedProgress_Percentage*     Percentage;
    GUIDialog_ExtendedProgress_SetPercentage*  SetPercentage;
    GUIDialog_ExtendedProgress_SetProgress*    SetProgress;
  } CB_GUILib_Dialog_ExtendedProgress;

  #define IMPL_GUI_EXTENDED_PROGRESS_DIALOG     \
    private:                                    \
      GUIHANDLE         m_DialogHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef bool GUIDialog_FileBrowser_ShowAndGetDirectory(
       const char*   shares,
       const char*   heading,
       char&         path,
       unsigned int& iMaxStringSize,
       bool          bWriteOnly);

  typedef bool GUIDialog_FileBrowser_ShowAndGetFile(
       const char*   shares,
       const char*   mask,
       const char*   heading,
       char&         file,
       unsigned int& iMaxStringSize,
       bool          useThumbs,
       bool          useFileDirectories);

  typedef bool GUIDialog_FileBrowser_ShowAndGetFileFromDir(
       const char*   directory,
       const char*   mask,
       const char*   heading,
       char&         file,
       unsigned int& iMaxStringSize,
       bool          useThumbs,
       bool          useFileDirectories,
       bool          singleList);

  typedef bool GUIDialog_FileBrowser_ShowAndGetFileList(
       const char*   shares,
       const char*   mask,
       const char*   heading,
       char**&       path,
       unsigned int& entries,
       bool          useThumbs,
       bool          useFileDirectories);

  typedef bool GUIDialog_FileBrowser_ShowAndGetSource(
       char&         path,
       unsigned int& iMaxStringSize,
       bool          allowNetworkShares,
       const char*   additionalShare,
       const char*   strType);

  typedef bool GUIDialog_FileBrowser_ShowAndGetImage(
       const char*   shares,
       const char*   heading,
       char&         path,
       unsigned int& iMaxStringSize);

  typedef bool GUIDialog_FileBrowser_ShowAndGetImageList(
       const char*   shares,
       const char*   heading,
       char**&       path,
       unsigned int& entries);

   typedef void GUIDialog_FileBrowser_ClearList(
       char**&       path,
       unsigned int  entries);

  typedef struct CB_GUILib_Dialog_FileBrowser
  {
    GUIDialog_FileBrowser_ShowAndGetDirectory*     ShowAndGetDirectory;
    GUIDialog_FileBrowser_ShowAndGetFile*          ShowAndGetFile;
    GUIDialog_FileBrowser_ShowAndGetFileFromDir*   ShowAndGetFileFromDir;
    GUIDialog_FileBrowser_ShowAndGetFileList*      ShowAndGetFileList;
    GUIDialog_FileBrowser_ShowAndGetSource*        ShowAndGetSource;
    GUIDialog_FileBrowser_ShowAndGetImage*         ShowAndGetImage;
    GUIDialog_FileBrowser_ShowAndGetImageList*     ShowAndGetImageList;
    GUIDialog_FileBrowser_ClearList*               ClearList;
  } CB_GUILib_Dialog_FileBrowser;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef bool GUIDialog_Keyboard_ShowAndGetInputWithHead
      (char         &strTextString,
       unsigned int &iMaxStringSize,
       const char  *heading,
       bool         allowEmptyResult,
       bool         hiddenInput,
       unsigned int autoCloseMs);

  typedef bool GUIDialog_Keyboard_ShowAndGetInput
      (char         &strTextString,
       unsigned int &iMaxStringSize,
       bool         allowEmptyResult,
       unsigned int autoCloseMs);

  typedef bool GUIDialog_Keyboard_ShowAndGetNewPasswordWithHead
      (char         &newPassword,
       unsigned int &iMaxStringSize,
       const char   *strHeading,
       bool          allowEmptyResult,
       unsigned int  autoCloseMs);

  typedef bool GUIDialog_Keyboard_ShowAndGetNewPassword
      (char         &strNewPassword,
       unsigned int &iMaxStringSize,
       unsigned int  autoCloseMs);

  typedef bool GUIDialog_Keyboard_ShowAndVerifyNewPasswordWithHead
      (char         &strNewPassword,
       unsigned int &iMaxStringSize,
       const char   *strHeading,
       bool          allowEmpty,
       unsigned int  autoCloseMs);

  typedef bool GUIDialog_Keyboard_ShowAndVerifyNewPassword
      (char         &strNewPassword,
       unsigned int &iMaxStringSize,
       unsigned int  autoCloseMs);

  typedef int  GUIDialog_Keyboard_ShowAndVerifyPassword
      (char         &strPassword,
       unsigned int &iMaxStringSize,
       const char   *strHeading,
       int           iRetries,
       unsigned int  autoCloseMs);

  typedef bool GUIDialog_Keyboard_ShowAndGetFilter
      (char         &aTextString,
       unsigned int &iMaxStringSize,
       bool          searching,
       unsigned int  autoCloseMs);

  typedef bool GUIDialog_Keyboard_SendTextToActiveKeyboard
      (const char   *aTextString,
       bool          closeKeyboard);

  typedef bool GUIDialog_Keyboard_isKeyboardActivated
      ();

  typedef struct CB_GUILib_Dialog_Keyboard
  {
    GUIDialog_Keyboard_ShowAndGetInputWithHead*          ShowAndGetInputWithHead;
    GUIDialog_Keyboard_ShowAndGetInput*                  ShowAndGetInput;
    GUIDialog_Keyboard_ShowAndGetNewPasswordWithHead*    ShowAndGetNewPasswordWithHead;
    GUIDialog_Keyboard_ShowAndGetNewPassword*            ShowAndGetNewPassword;
    GUIDialog_Keyboard_ShowAndVerifyNewPasswordWithHead* ShowAndVerifyNewPasswordWithHead;
    GUIDialog_Keyboard_ShowAndVerifyNewPassword*         ShowAndVerifyNewPassword;
    GUIDialog_Keyboard_ShowAndVerifyPassword*            ShowAndVerifyPassword;
    GUIDialog_Keyboard_ShowAndGetFilter*                 ShowAndGetFilter;
    GUIDialog_Keyboard_SendTextToActiveKeyboard*         SendTextToActiveKeyboard;
    GUIDialog_Keyboard_isKeyboardActivated*              isKeyboardActivated;
  } CB_GUILib_Dialog_Keyboard;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef bool GUIDialog_Numeric_ShowAndVerifyNewPassword
      (char         &strNewPassword,
       unsigned int &iMaxStringSize);

  typedef int  GUIDialog_Numeric_ShowAndVerifyPassword
      (char         &strPassword,
       unsigned int &iMaxStringSize,
       const char   *strHeading,
       int           iRetries);

  typedef bool GUIDialog_Numeric_ShowAndVerifyInput
      (char         &strPassword,
       unsigned int &iMaxStringSize,
       const char   *strHeading,
       bool          bVerifyInput);

  typedef bool GUIDialog_Numeric_ShowAndGetTime
      (tm           &time,
       const char   *strHeading);

  typedef bool GUIDialog_Numeric_ShowAndGetDate
      (tm           &date,
       const char   *strHeading);

  typedef bool GUIDialog_Numeric_ShowAndGetIPAddress
      (char         &strIPAddress,
       unsigned int &iMaxStringSize,
       const char   *strHeading);

  typedef bool GUIDialog_Numeric_ShowAndGetNumber
      (char         &strInput,
       unsigned int &iMaxStringSize,
       const char   *strHeading,
       unsigned int  iAutoCloseTimeoutMs);

  typedef bool GUIDialog_Numeric_ShowAndGetSeconds
      (char         &timeString,
       unsigned int &iMaxStringSize,
       const char   *strHeading);

  typedef struct CB_GUILib_Dialog_Numeric
  {
    GUIDialog_Numeric_ShowAndVerifyNewPassword*          ShowAndVerifyNewPassword;
    GUIDialog_Numeric_ShowAndVerifyPassword*             ShowAndVerifyPassword;
    GUIDialog_Numeric_ShowAndVerifyInput*                ShowAndVerifyInput;
    GUIDialog_Numeric_ShowAndGetTime*                    ShowAndGetTime;
    GUIDialog_Numeric_ShowAndGetDate*                    ShowAndGetDate;
    GUIDialog_Numeric_ShowAndGetIPAddress*               ShowAndGetIPAddress;
    GUIDialog_Numeric_ShowAndGetNumber*                  ShowAndGetNumber;
    GUIDialog_Numeric_ShowAndGetSeconds*                 ShowAndGetSeconds;
  } CB_GUILib_Dialog_Numeric;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIDialog_OK_ShowAndGetInputSingleText
      (const char *heading,
       const char *text);

  typedef void GUIDialog_OK_ShowAndGetInputLineText
      (const char *heading,
       const char *line0,
       const char *line1,
       const char *line2);

  typedef struct CB_GUILib_Dialog_OK
  {
    GUIDialog_OK_ShowAndGetInputSingleText*   ShowAndGetInputSingleText;
    GUIDialog_OK_ShowAndGetInputLineText*     ShowAndGetInputLineText;
  } CB_GUILib_Dialog_OK;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef GUIHANDLE GUIDialog_Progress_New
      (void         *addonData);

  typedef void GUIDialog_Progress_Delete
      (void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIDialog_Progress_Open
      (void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIDialog_Progress_SetHeading
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *heading);

  typedef void GUIDialog_Progress_SetLine
      (void         *addonData,
       GUIHANDLE     handle,
       unsigned int  iLine,
       const char   *line);

  typedef void GUIDialog_Progress_SetCanCancel
      (void         *addonData,
       GUIHANDLE     handle,
       bool          bCanCancel);

  typedef bool GUIDialog_Progress_IsCanceled
      (void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIDialog_Progress_SetPercentage
      (void         *addonData,
       GUIHANDLE     handle,
       int           iPercentage);

  typedef int GUIDialog_Progress_GetPercentage
      (void         *addonData,
       GUIHANDLE     handle);

  typedef void GUIDialog_Progress_ShowProgressBar
      (void         *addonData,
       GUIHANDLE     handle,
       bool          bOnOff);

  typedef void GUIDialog_Progress_SetProgressMax
      (void         *addonData,
       GUIHANDLE     handle,
       int           iMax);

  typedef void GUIDialog_Progress_SetProgressAdvance
      (void         *addonData,
       GUIHANDLE     handle,
       int           nSteps);

  typedef bool GUIDialog_Progress_Abort
      (void         *addonData,
       GUIHANDLE     handle);

  typedef struct CB_GUILib_Dialog_Progress
  {
    GUIDialog_Progress_New*                New;
    GUIDialog_Progress_Delete*             Delete;
    GUIDialog_Progress_Open*               Open;
    GUIDialog_Progress_SetHeading*         SetHeading;
    GUIDialog_Progress_SetLine*            SetLine;
    GUIDialog_Progress_SetCanCancel*       SetCanCancel;
    GUIDialog_Progress_IsCanceled*         IsCanceled;
    GUIDialog_Progress_SetPercentage*      SetPercentage;
    GUIDialog_Progress_GetPercentage*      GetPercentage;
    GUIDialog_Progress_ShowProgressBar*    ShowProgressBar;
    GUIDialog_Progress_SetProgressMax*     SetProgressMax;
    GUIDialog_Progress_SetProgressAdvance* SetProgressAdvance;
    GUIDialog_Progress_Abort*              Abort;
  } CB_GUILib_Dialog_Progress;

  #define IMPL_GUI_PROGRESS_DIALOG              \
    private:                                    \
      GUIHANDLE         m_DialogHandle;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef int GUIDialog_Select_Open
      (const char   *heading,
       const char   *entries[],
       unsigned int  size,
       int           selected);

  typedef struct CB_GUILib_Dialog_Select
  {
    GUIDialog_Select_Open*   Open;
  } CB_GUILib_Dialog_Select;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUIDialog_TextViewer_Open(
       const char  *heading,
       const char  *text);

  typedef struct CB_GUILib_Dialog_TextViewer
  {
    GUIDialog_TextViewer_Open*  Open;
  } CB_GUILib_Dialog_TextViewer;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef bool GUIDialog_YesNo_ShowAndGetInputSingleText
      (const char *heading,
       const char *text,
       bool       &bCanceled,
       const char *noLabel,
       const char *yesLabel);

  typedef bool GUIDialog_YesNo_ShowAndGetInputLineText
      (const char *heading,
       const char *line0,
       const char *line1,
       const char *line2,
       const char *noLabel,
       const char *yesLabel);

  typedef bool GUIDialog_YesNo_ShowAndGetInputLineButtonText
      (const char *heading,
       const char *line0,
       const char *line1,
       const char *line2,
       bool       &bCanceled,
       const char *noLabel,
       const char *yesLabel);

  typedef struct CB_GUILib_Dialog_YesNo
  {
    GUIDialog_YesNo_ShowAndGetInputSingleText*     ShowAndGetInputSingleText;
    GUIDialog_YesNo_ShowAndGetInputLineText*       ShowAndGetInputLineText;
    GUIDialog_YesNo_ShowAndGetInputLineButtonText* ShowAndGetInputLineButtonText;
  } CB_GUILib_Dialog_YesNo;
  //----------------------------------------------------------------------------


  //============================================================================
  /*
   * Internal Structures to have "C"-Style data transfer
   */
  typedef struct ADDON_VideoInfoTag__cast__DATA_STRUCT
  {
    const char* name;
    const char* role;
    int         order;
    const char* thumbnail;
  } ADDON_VideoInfoTag__cast__DATA_STRUCT;

  typedef GUIHANDLE   GUIListItem_Create
      (void         *addonData,
       const char   *label,
       const char   *label2,
       const char   *iconImage,
       const char   *thumbnailImage,
       const char   *path);

  typedef void        GUIListItem_Destroy
      (void         *addonData,
       GUIHANDLE     handle);

  typedef void        GUIListItem_GetLabel
      (void         *addonData,
       GUIHANDLE     handle,
       char         &label,
       unsigned int &iMaxStringSize);

  typedef void        GUIListItem_SetLabel
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *label);

  typedef void        GUIListItem_GetLabel2
      (void         *addonData,
       GUIHANDLE     handle,
       char         &label,
       unsigned int &iMaxStringSize);

  typedef void        GUIListItem_SetLabel2
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *label);

  typedef void        GUIListItem_GetIconImage
      (void         *addonData,
       GUIHANDLE     handle,
       char         &image,
       unsigned int &iMaxStringSize);

  typedef void        GUIListItem_SetIconImage
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *image);

  typedef void        GUIListItem_GetOverlayImage
      (void         *addonData,
       GUIHANDLE     handle,
       char         &image,
       unsigned int &iMaxStringSize);

  typedef void        GUIListItem_SetOverlayImage
      (void         *addonData,
       GUIHANDLE     handle,
       unsigned int  image,
       bool          bOnOff);

  typedef void        GUIListItem_SetThumbnailImage
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *image);

  typedef void        GUIListItem_SetArt
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *type,
       const char   *url);

  typedef void        GUIListItem_SetArtFallback
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *from,
       const char   *to);

  typedef void        GUIListItem_SetLabel
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *label);

  typedef bool        GUIListItem_HasArt
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *type);

  typedef void        GUIListItem_Select
      (void         *addonData,
       GUIHANDLE     handle,
       bool          bOnOff);

  typedef bool        GUIListItem_IsSelected
      (void         *addonData,
       GUIHANDLE     handle);

  typedef bool        GUIListItem_HasIcon
      (void         *addonData,
       GUIHANDLE     handle);

  typedef bool        GUIListItem_HasOverlay
      (void         *addonData,
       GUIHANDLE     handle);

  typedef bool        GUIListItem_IsFileItem
      (void         *addonData,
       GUIHANDLE     handle);

  typedef bool        GUIListItem_IsFolder
      (void         *addonData,
       GUIHANDLE     handle);

  typedef void        GUIListItem_SetProperty
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *key,
       const char   *value);

  typedef void        GUIListItem_GetProperty
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *key,
       char         &property,
       unsigned int &iMaxStringSize);

  typedef void        GUIListItem_ClearProperty
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *key);

  typedef void        GUIListItem_ClearProperties
      (void         *addonData,
       GUIHANDLE     handle);

  typedef bool        GUIListItem_HasProperties
      (void         *addonData,
       GUIHANDLE     handle);

  typedef bool        GUIListItem_HasProperty
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *key);

  typedef void        GUIListItem_SetPath
      (void         *addonData,
       GUIHANDLE     handle,
       const char   *path);

  typedef char*       GUIListItem_GetPath(
       void*         addonData,
       GUIHANDLE     handle);

  typedef char*       GUIListItem_GetDescription(
       void*         addonData,
       GUIHANDLE     handle);

  typedef int         GUIListItem_GetDuration(
       void*         addonData,
       GUIHANDLE     handle);

  typedef void        GUIListItem_SetSubtitles(
       void*         addonData,
       GUIHANDLE     handle,
       const char**  streams,
       unsigned int  entries);

  typedef void        GUIListItem_SetMimeType(
       void*         addonData,
       GUIHANDLE     handle,
       const char*   mimetype);

  typedef void        GUIListItem_SetContentLookup(
       void*         addonData,
       GUIHANDLE     handle,
       bool          enable);

  typedef void        GUIListItem_AddContextMenuItems(
       void*         addonData,
       GUIHANDLE     handle,
       const char**  streams[2],
       unsigned int  entries,
       bool          replaceItems);

  typedef void        GUIListItem_AddStreamInfo(
       void*         addonData,
       GUIHANDLE     handle,
       const char*   cType,
       const char**  dictionary[2],
       unsigned int  entries);

  typedef void        GUIListItem_SetMusicInfo(
       void*         addonData,
       GUIHANDLE     handle,
       unsigned int  type,
       void*         data,
       unsigned int  entries);

  typedef void        GUIListItem_SetVideoInfo(
       void*         addonData,
       GUIHANDLE     handle,
       unsigned int  type,
       void*         data,
       unsigned int  entries);

  typedef void        GUIListItem_SetPictureInfo(
       void*         addonData,
       GUIHANDLE     handle,
       unsigned int  type,
       void*         data,
       unsigned int  entries);

  typedef struct CB_GUILib_ListItem
  {
    GUIListItem_Create*                  Create;
    GUIListItem_Destroy*                 Destroy;
    GUIListItem_GetLabel*                GetLabel;
    GUIListItem_SetLabel*                SetLabel;
    GUIListItem_GetLabel2*               GetLabel2;
    GUIListItem_SetLabel2*               SetLabel2;
    GUIListItem_GetIconImage*            GetIconImage;
    GUIListItem_SetIconImage*            SetIconImage;
    GUIListItem_GetOverlayImage*         GetOverlayImage;
    GUIListItem_SetOverlayImage*         SetOverlayImage;
    GUIListItem_SetThumbnailImage*       SetThumbnailImage;
    GUIListItem_SetArt*                  SetArt;
    GUIListItem_SetArtFallback*          SetArtFallback;
    GUIListItem_HasArt*                  HasArt;
    GUIListItem_Select*                  Select;
    GUIListItem_IsSelected*              IsSelected;
    GUIListItem_HasIcon*                 HasIcon;
    GUIListItem_HasOverlay*              HasOverlay;
    GUIListItem_IsFileItem*              IsFileItem;
    GUIListItem_IsFolder*                IsFolder;
    GUIListItem_SetProperty*             SetProperty;
    GUIListItem_GetProperty*             GetProperty;
    GUIListItem_ClearProperty*           ClearProperty;
    GUIListItem_ClearProperties*         ClearProperties;
    GUIListItem_HasProperties*           HasProperties;
    GUIListItem_HasProperty*             HasProperty;
    GUIListItem_SetPath*                 SetPath;
    GUIListItem_GetPath*                 GetPath;
    GUIListItem_GetDuration*             GetDuration;
    GUIListItem_SetSubtitles*            SetSubtitles;
    GUIListItem_SetMimeType*             SetMimeType;
    GUIListItem_SetContentLookup*        SetContentLookup;
    GUIListItem_AddContextMenuItems*     AddContextMenuItems;
    GUIListItem_AddStreamInfo*           AddStreamInfo;
    GUIListItem_SetMusicInfo*            SetMusicInfo;
    GUIListItem_SetVideoInfo*            SetVideoInfo;
    GUIListItem_SetPictureInfo*          SetPictureInfo;
  } CB_GUILib_ListItem;

  #define IMPL_ADDON_GUI_LIST                   \
    public:                                     \
      CListItem(GUIHANDLE listItemHandle); \
      const GUIHANDLE GetListItemHandle() const \
      {                                         \
        return m_ListItemHandle;                \
      }                                         \
    protected:                                  \
      GUIHANDLE         m_ListItemHandle;       \
    private:                                    \
      friend class CWindow;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef GUIHANDLE   GUIWindow_New
      (void        *addonData,
       const char  *xmlFilename,
       const char  *defaultSkin,
       bool         forceFallback,
       bool         asDialog);

  typedef void        GUIWindow_Delete
      (void        *addonData,
       GUIHANDLE    handle);

  typedef void        GUIWindow_SetCallbacks
      (void        *addonData,
       GUIHANDLE    handle,
       GUIHANDLE    clienthandle,
       bool (*)(GUIHANDLE handle),
       bool (*)(GUIHANDLE handle, int),
       bool (*)(GUIHANDLE handle, int),
       bool (*)(GUIHANDLE handle, int));

  typedef bool        GUIWindow_Show
      (void        *addonData,
       GUIHANDLE    handle);

  typedef bool        GUIWindow_Close
      (void        *addonData,
       GUIHANDLE    handle);

  typedef bool        GUIWindow_DoModal
      (void        *addonData,
       GUIHANDLE    handle);

  typedef bool        GUIWindow_SetFocusId
      (void        *addonData,
       GUIHANDLE    handle,
       int          iControlId);

  typedef int         GUIWindow_GetFocusId
      (void        *addonData,
       GUIHANDLE    handle);

  typedef void        GUIWindow_SetProperty
      (void        *addonData,
       GUIHANDLE    handle,
       const char  *key,
       const char  *value);

  typedef void        GUIWindow_SetPropertyInt
      (void        *addonData,
       GUIHANDLE    handle,
       const char  *key,
       int          value);

  typedef void        GUIWindow_SetPropertyBool
      (void        *addonData,
       GUIHANDLE    handle,
       const char  *key,
       bool         value);

  typedef void        GUIWindow_SetPropertyDouble
      (void        *addonData,
       GUIHANDLE    handle,
       const char  *key,
       double       value);

  typedef void        GUIWindow_GetProperty
      (void        *addonData,
       GUIHANDLE    handle,
       const char  *key,
       char        &property,
       unsigned int &iMaxStringSize);

  typedef int         GUIWindow_GetPropertyInt
      (void        *addonData,
       GUIHANDLE    handle,
       const char  *key);

  typedef bool        GUIWindow_GetPropertyBool
      (void        *addonData,
       GUIHANDLE    handle,
       const char  *key);

  typedef double      GUIWindow_GetPropertyDouble
      (void        *addonData,
       GUIHANDLE    handle,
       const char  *key);

  typedef void        GUIWindow_ClearProperties
      (void        *addonData,
       GUIHANDLE    handle);

  typedef void        GUIWindow_ClearProperty
      (void        *addonData,
       GUIHANDLE    handle,
       const char  *key);

  typedef int         GUIWindow_GetListSize
      (void        *addonData,
       GUIHANDLE    handle);

  typedef void        GUIWindow_ClearList
      (void        *addonData,
       GUIHANDLE    handle);

  typedef GUIHANDLE   GUIWindow_AddItem
      (void        *addonData,
       GUIHANDLE    handle,
       GUIHANDLE    item,
       int          itemPosition);

  typedef GUIHANDLE   GUIWindow_AddStringItem
      (void        *addonData,
       GUIHANDLE    handle,
       const char  *itemName,
       int          itemPosition);

  typedef void        GUIWindow_RemoveItem
      (void        *addonData,
       GUIHANDLE    handle,
       int          itemPosition);

  typedef void        GUIWindow_RemoveItemFile
      (void        *addonData,
       GUIHANDLE    handle,
       GUIHANDLE    fileItem);

  typedef GUIHANDLE   GUIWindow_GetListItem
      (void        *addonData,
       GUIHANDLE    handle,
       int          listPos);

  typedef void        GUIWindow_SetCurrentListPosition
      (void        *addonData,
       GUIHANDLE    handle,
       int          listPos);

  typedef int         GUIWindow_GetCurrentListPosition
      (void        *addonData,
       GUIHANDLE    handle);

  typedef void        GUIWindow_SetControlLabel
      (void        *addonData,
       GUIHANDLE    handle,
       int          controlId,
       const char  *label);

  typedef void        GUIWindow_MarkDirtyRegion
      (void        *addonData,
       GUIHANDLE    handle);

  typedef GUIHANDLE   GUIWindow_GetControl
      (void         *addonData,
       GUIHANDLE     handle,
       int           controlId);

  typedef struct CB_GUILib_Window
  {
    GUIWindow_New*                       New;
    GUIWindow_Delete*                    Delete;
    GUIWindow_SetCallbacks*              SetCallbacks;
    GUIWindow_Show*                      Show;
    GUIWindow_Close*                     Close;
    GUIWindow_DoModal*                   DoModal;
    GUIWindow_SetFocusId*                SetFocusId;
    GUIWindow_GetFocusId*                GetFocusId;
    GUIWindow_SetProperty*               SetProperty;
    GUIWindow_SetPropertyInt*            SetPropertyInt;
    GUIWindow_SetPropertyBool*           SetPropertyBool;
    GUIWindow_SetPropertyDouble*         SetPropertyDouble;
    GUIWindow_GetProperty*               GetProperty;
    GUIWindow_GetPropertyInt*            GetPropertyInt;
    GUIWindow_GetPropertyBool*           GetPropertyBool;
    GUIWindow_GetPropertyDouble*         GetPropertyDouble;
    GUIWindow_ClearProperties*           ClearProperties;
    GUIWindow_ClearProperty*             ClearProperty;
    GUIWindow_GetListSize*               GetListSize;
    GUIWindow_ClearList*                 ClearList;
    GUIWindow_AddItem*                   AddItem;
    GUIWindow_AddStringItem*             AddStringItem;
    GUIWindow_RemoveItem*                RemoveItem;
    GUIWindow_RemoveItemFile*            RemoveItemFile;
    GUIWindow_GetListItem*               GetListItem;
    GUIWindow_SetCurrentListPosition*    SetCurrentListPosition;
    GUIWindow_GetCurrentListPosition*    GetCurrentListPosition;
    GUIWindow_SetControlLabel*           SetControlLabel;
    GUIWindow_MarkDirtyRegion*           MarkDirtyRegion;

    GUIWindow_GetControl*                GetControl_Button;
    GUIWindow_GetControl*                GetControl_Edit;
    GUIWindow_GetControl*                GetControl_FadeLabel;
    GUIWindow_GetControl*                GetControl_Image;
    GUIWindow_GetControl*                GetControl_Label;
    GUIWindow_GetControl*                GetControl_Spin;
    GUIWindow_GetControl*                GetControl_RadioButton;
    GUIWindow_GetControl*                GetControl_Progress;
    GUIWindow_GetControl*                GetControl_RenderAddon;
    GUIWindow_GetControl*                GetControl_Slider;
    GUIWindow_GetControl*                GetControl_SettingsSlider;
    GUIWindow_GetControl*                GetControl_TextBox;

  } CB_GUILib_Window;

  #define IMPLEMENT_ADDON_GUI_WINDOW                                           \
    protected:                                                                 \
      GUIHANDLE         m_WindowHandle;                                        \
    private:                                                                   \
      static bool OnInitCB(GUIHANDLE cbhdl);                                   \
      static bool OnClickCB(GUIHANDLE cbhdl, int controlId);                   \
      static bool OnFocusCB(GUIHANDLE cbhdl, int controlId);                   \
      static bool OnActionCB(GUIHANDLE cbhdl, int actionId);                   \
      friend class CControlButton;                                     \
      friend class CControlEdit;                                       \
      friend class CControlImage;                                      \
      friend class CControlFadeLabel;                                  \
      friend class CControlLabel;                                      \
      friend class CControlSpin;                                       \
      friend class CControlProgress;                                   \
      friend class CControlRadioButton;                                \
      friend class CControlRendering;                                  \
      friend class CControlSlider;                                     \
      friend class CControlSettingsSlider;                             \
      friend class CControlTextBox;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void GUILock                    ();
  typedef void GUIUnlock                  ();
  typedef int  GUIGetScreenHeight         ();
  typedef int  GUIGetScreenWidth          ();
  typedef int  GUIGetVideoResolution      ();
  typedef int  GUIGetCurrentWindowDialogId();
  typedef int  GUIGetCurrentWindowId      ();
  typedef void _free_string(void* hdl, char* str);

  typedef struct CB_GUILib_General
  {
    GUILock*                          Lock;
    GUIUnlock*                        Unlock;
    GUIGetScreenHeight*               GetScreenHeight;
    GUIGetScreenWidth*                GetScreenWidth;
    GUIGetVideoResolution*            GetVideoResolution;
    GUIGetCurrentWindowDialogId*      GetCurrentWindowDialogId;
    GUIGetCurrentWindowId*            GetCurrentWindowId;
  } CB_GUILib_General;

  typedef struct CB_GUILib_Controls
  {
    CB_GUILib_Control_Button             Button;
    CB_GUILib_Control_Edit               Edit;
    CB_GUILib_Control_FadeLabel          FadeLabel;
    CB_GUILib_Control_Image              Image;
    CB_GUILib_Control_Label              Label;
    CB_GUILib_Control_Progress           Progress;
    CB_GUILib_Control_RadioButton        RadioButton;
    CB_GUILib_Control_Rendering          Rendering;
    CB_GUILib_Control_SettingsSlider     SettingsSlider;
    CB_GUILib_Control_Slider             Slider;
    CB_GUILib_Control_Spin               Spin;
    CB_GUILib_Control_TextBox            TextBox;
  } CB_GUILib_Controls;

  typedef struct CB_GUILib_Dialogs
  {
    CB_GUILib_Dialog_ExtendedProgress    ExtendedProgress;
    CB_GUILib_Dialog_FileBrowser         FileBrowser;
    CB_GUILib_Dialog_Keyboard            Keyboard;
    CB_GUILib_Dialog_Numeric             Numeric;
    CB_GUILib_Dialog_OK                  OK;
    CB_GUILib_Dialog_Progress            Progress;
    CB_GUILib_Dialog_Select              Select;
    CB_GUILib_Dialog_TextViewer          TextViewer;
    CB_GUILib_Dialog_YesNo               YesNo;
  } CB_GUILib_Dialogs;

  typedef struct CB_GUILib
  {
    CB_GUILib_General                    General;
    CB_GUILib_Controls                   Control;
    CB_GUILib_Dialogs                    Dialogs;
    CB_GUILib_ListItem                   ListItem;
    CB_GUILib_Window                     Window;
  } CB_GUILib;
  //----------------------------------------------------------------------------


  //============================================================================
  typedef void    _addon_log_msg(void* hdl, const addon_log loglevel, const char *msg);
  typedef void    _free_string(void* hdl, char* str);

  typedef struct CB_AddOnLib
  {
    _addon_log_msg*             addon_log_msg;
    _free_string*               free_string;
    CB_AddonLib_General         General;
    CB_AddonLib_Audio           Audio;
    CB_AddonLib_Codec           Codec;
    CB_AddonLib_Directory       Directory;
    CB_AddonLib_VFS             VFS;
    CB_AddonLib_File            File;
    CB_AddonLib_Network         Network;

    CB_AudioEngineLib           AudioEngine;
    CB_AudioEngineLib_Stream    AudioEngineStream;

    CB_GUILib                   GUI;

    CB_PVRLib                   PVR;

    CB_PlayerLib_AddonPlayer        AddonPlayer;
    CB_PlayerLib_AddonPlayList      AddonPlayList;
    CB_PlayerLib_AddonInfoTagMusic  AddonInfoTagMusic;
    CB_PlayerLib_AddonInfoTagVideo  AddonInfoTagVideo;

  } CB_AddOnLib;

  typedef CB_AddOnLib*  _register_level(void* HANDLE, int level);
  typedef void          _unregister_me(void* HANDLE, void* CB);

}; /* extern "C" */
}; /* namespace KodiAPI */
}; /* namespace V2 */
