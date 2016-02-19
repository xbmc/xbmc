/*
 *      Copyright (C) 2015 Team KODI
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

#include "BinaryAddon.h"
#include "tools.h"

#include "Application.h"
#include "CompileInfo.h"
#include "addons/IAddon.h"
#include "addons/binary/callbacks/AddonCallbacks.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <netinet/in.h>

namespace ADDON
{

CBinaryAddon::CBinaryAddon(bool isLocal, int fd, unsigned int id, bool isFather, int rand)
  : CThread("Addon handler"),
    m_isLocal(isLocal),
    m_active(false),
    m_fd(fd),
    m_Id(id),
    m_loggedIn(false),
    m_isIndependent(false),
    m_isFather(isFather),
    m_randomConnectionNumber(rand),
    m_addon(nullptr),
    m_addonCB(nullptr),
    m_pHelpers(nullptr),
    m_sharedMem(nullptr)
{
  if (m_isFather)
  {
    m_socket.SetHandle(fd);
    StartNetStreamThread();
  }
}

CBinaryAddon::CBinaryAddon(const CBinaryAddon *right, int rand)
  : CThread("Addon handler (child)"),
    m_isLocal(right->m_isLocal),
    m_active(false),
    m_fd(right->m_fd),
    m_Id(right->m_Id),
    m_loggedIn(false),
    m_isIndependent(false),
    m_isFather(false),
    m_randomConnectionNumber(rand),
    m_sharedMem(right->m_sharedMem)
{
  m_addon               = new CAddon(*right->m_addon);
  m_pHelpers            = new CAddonCallbacks(m_addon);
  m_addonData.addonData = m_pHelpers;
  m_addonCB = (V2::KodiAPI::CB_AddOnLib*)CAddonCallbacksAddonBase::CreateHelper(m_pHelpers, 2);
}

CBinaryAddon::~CBinaryAddon()
{
  if (m_isFather)
  {
    if (m_sharedMem)
      delete m_sharedMem;
    m_socket.close(); // force closing connection
    StopThread();
  }

  if (m_addon)
    delete m_addon;
  if (m_pHelpers)
  {
    CAddonCallbacksAddonBase::DestroyHelper(m_pHelpers);
    delete m_pHelpers;
  }
}

bool CBinaryAddon::StartNetStreamThread()
{
  Create();
  return true;
}

bool CBinaryAddon::StartSharedMemoryThread(int rand)
{
#if (defined TARGET_WINDOWS)
  m_sharedMem = new CBinaryAddonSharedMemoryWindows(rand, this);
#elif (defined TARGET_POSIX)
  m_sharedMem = new CBinaryAddonSharedMemoryPosix(rand, this);
#endif
  return m_sharedMem->CreateSharedMemory();
}

void CBinaryAddon::Process(void)
{
  uint32_t channelID;
  uint32_t requestID;
  uint32_t opcode;
  uint32_t dataLength;
  uint8_t* data;

  while (!m_bStop && !g_application.m_bStop)
  {
    if (!m_socket.read((uint8_t*)&channelID, sizeof(uint32_t)))
      break;
    channelID = ntohl(channelID);

    if (channelID == 1)
    {
      if (!m_socket.read((uint8_t*)&requestID, sizeof(uint32_t), 10000))
        break;
      requestID = ntohl(requestID);

      if (!m_socket.read((uint8_t*)&opcode, sizeof(uint32_t), 10000))
        break;
      opcode = ntohl(opcode);

      if (!m_socket.read((uint8_t*)&dataLength, sizeof(uint32_t), 10000))
        break;
      dataLength = ntohl(dataLength);
      if (dataLength > 200000) // a random sanity limit
      {
        CLog::Log(LOGERROR, "dataLength > 200000!");
        break;
      }

      if (dataLength)
      {
        try
        {
          data = new uint8_t[dataLength];
        }
        catch (const std::bad_alloc &)
        {
          CLog::Log(LOGERROR, "Extra data buffer malloc error");
          break;
        }

        if (!m_socket.read(data, dataLength, 10000))
        {
          CLog::Log(LOGERROR, "Could not read data");
          free(data);
          break;
        }
      }
      else
      {
        data = NULL;
      }

      //fprintf(stderr, "Received chan=%u, ser=%u, op=%u, edl=%u\n", channelID, requestID, opcode, dataLength);

      if (!m_loggedIn && (opcode != KODICall_LoginVerify) && (opcode != KODICall_CreateSubThread))
      {
        CLog::Log(LOGERROR, "Add-ons must be logged in before sending commands! Aborting. (opcode = '%i')", opcode);
        if (data) free(data);
        break;
      }

      try
      {
        CRequestPacket req(requestID, opcode, data, dataLength);
        CResponsePacket resp;
        processRequest(req, resp);
        m_socket.write(resp.getPtr(), resp.getLen());
      }
      catch (const std::exception &e)
      {
        CLog::Log(LOGERROR, "%s", e.what());
        break;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "Incoming stream channel number unknown %i", channelID);
      break;
    }
  }
}

//--- - --- - --- - --- - --- - --- - --- - --- - --- - --- - --- - --- - ---

bool CBinaryAddon::processRequest(CRequestPacket& req, CResponsePacket& resp)
{
  CSingleLock lock(m_msgLock);

  bool result = false;
  switch(req.getOpCode())
  {
  case KODICall_LoginVerify:
    result = process_Login(req, resp);
    break;

  case KODICall_Logout:
    result = process_Logout(req, resp);
    break;

  case KODICall_Ping:
    result = process_Ping(req, resp);
    break;

  case KODICall_Log:
    result = process_Log(req, resp);
    break;

  case KODICall_CreateSubThread:
    result = process_CreateSubThread(req, resp);
    break;

  case KODICall_DeleteSubThread:
    result = process_DeleteSubThread(req, resp);
    break;

  case KODICall_AddOn_General_GetSettingString:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetSettingString(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetSettingBoolean:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetSettingBoolean(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetSettingInt:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetSettingInt(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetSettingFloat:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetSettingFloat(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_OpenSettings:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::OpenSettings(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetAddonInfo:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetAddonInfo(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_QueueFormattedNotification:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::QueueFormattedNotification(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_QueueNotificationFromType:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::QueueNotificationFromType(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_QueueNotificationWithImage:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::QueueNotificationWithImage(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetMD5:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetMD5(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_UnknownToUTF8:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::UnknownToUTF8(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetLocalizedString:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetLocalizedString(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetLanguage:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetLanguage(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetDVDMenuLanguage:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetDVDMenuLanguage(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_StartServer:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::StartServer(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_AudioSuspend:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::AudioSuspend(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_AudioResume:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::AudioResume(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetVolume:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetVolume(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_SetVolume:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::SetVolume(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_IsMuted:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::IsMuted(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_ToggleMute:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::ToggleMute(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_SetMute:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::SetMute(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetOpticalDriveState:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetOpticalDriveState(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_EjectOpticalDrive:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::EjectOpticalDrive(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_KodiVersion:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::KodiVersion(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_KodiQuit:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::KodiQuit(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_HTPCShutdown:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::HTPCShutdown(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_HTPCRestart:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::HTPCRestart(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_ExecuteScript:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::ExecuteScript(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_ExecuteBuiltin:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::ExecuteBuiltin(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_ExecuteJSONRPC:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::ExecuteJSONRPC(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetRegion:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetRegion(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetFreeMem:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetFreeMem(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_GetGlobalIdleTime:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::GetGlobalIdleTime(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_General_TranslatePath:
    result = V2::KodiAPI::CAddonExeCB_Addon_General::TranslatePath(&m_addonData, req, resp);
    break;

  case KODICall_AddOn_Codec_GetCodecByName:
    result = V2::KodiAPI::CAddonExeCB_Addon_Codec::GetCodecByName(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_Codec_AllocateDemuxPacket:
    result = V2::KodiAPI::CAddonExeCB_Addon_Codec::AllocateDemuxPacket(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_Codec_FreeDemuxPacket:
    result = V2::KodiAPI::CAddonExeCB_Addon_Codec::FreeDemuxPacket(&m_addonData, req, resp);
    break;

  case KODICall_AddOn_Network_WakeOnLan:
    result = V2::KodiAPI::CAddonExeCB_Addon_Network::WakeOnLan(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_Network_GetIPAddress:
    result = V2::KodiAPI::CAddonExeCB_Addon_Network::GetIPAddress(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_Network_URLEncode:
    result = V2::KodiAPI::CAddonExeCB_Addon_Network::URLEncode(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_Network_DNSLookup:
    result = V2::KodiAPI::CAddonExeCB_Addon_Network::DNSLookup(&m_addonData, req, resp);
    break;

  case KODICall_AddOn_SoundPlay_GetHandle:
    result = V2::KodiAPI::CAddonExeCB_Addon_SoundPlay::SoundPlay_GetHandle(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_SoundPlay_ReleaseHandle:
    result = V2::KodiAPI::CAddonExeCB_Addon_SoundPlay::SoundPlay_ReleaseHandle(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_SoundPlay_Play:
    result = V2::KodiAPI::CAddonExeCB_Addon_SoundPlay::SoundPlay_Play(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_SoundPlay_Stop:
    result = V2::KodiAPI::CAddonExeCB_Addon_SoundPlay::SoundPlay_Stop(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_SoundPlay_IsPlaying:
    result = V2::KodiAPI::CAddonExeCB_Addon_SoundPlay::SoundPlay_IsPlaying(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_SoundPlay_SetChannel:
    result = V2::KodiAPI::CAddonExeCB_Addon_SoundPlay::SoundPlay_SetChannel(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_SoundPlay_GetChannel:
    result = V2::KodiAPI::CAddonExeCB_Addon_SoundPlay::SoundPlay_GetChannel(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_SoundPlay_SetVolume:
    result = V2::KodiAPI::CAddonExeCB_Addon_SoundPlay::SoundPlay_SetVolume(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_SoundPlay_GetVolume:
    result = V2::KodiAPI::CAddonExeCB_Addon_SoundPlay::SoundPlay_GetVolume(&m_addonData, req, resp);
    break;

  case KODICall_AddOn_VFSUtils_CreateDirectory:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::CreateDirectory(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_DirectoryExists:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::DirectoryExists(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_RemoveDirectory:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::RemoveDirectory(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_GetVFSDirectory:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::GetVFSDirectory(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_FreeVFSDirectory:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::FreeVFSDirectory(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_GetFileMD5:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::GetFileMD5(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_GetCacheThumbName:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::GetCacheThumbName(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_MakeLegalFileName:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::MakeLegalFileName(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_MakeLegalPath:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::MakeLegalPath(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_OpenFile:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::OpenFile(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_OpenFileForWrite:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::OpenFileForWrite(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_ReadFile:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::ReadFile(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_ReadFileString:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::ReadFileString(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_WriteFile:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::WriteFile(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_FlushFile:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::FlushFile(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_SeekFile:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::SeekFile(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_TruncateFile:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::TruncateFile(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_GetFilePosition:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::GetFilePosition(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_GetFileLength:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::GetFileLength(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_CloseFile:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::CloseFile(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_GetFileChunkSize:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::GetFileChunkSize(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_FileExists:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::FileExists(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_StatFile:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::StatFile(&m_addonData, req, resp);
    break;
  case KODICall_AddOn_VFSUtils_DeleteFile:
    result = V2::KodiAPI::CAddonExeCB_Addon_VFSUtils::DeleteFile(&m_addonData, req, resp);
    break;

  case KODICall_AudioEngine_General_AddDSPMenuHook:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_General::AddDSPMenuHook(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_General_RemoveDSPMenuHook:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_General::RemoveDSPMenuHook(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_General_RegisterDSPMode:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_General::RegisterDSPMode(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_General_UnregisterDSPMode:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_General::UnregisterDSPMode(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_General_GetCurrentSinkFormat:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_General::GetCurrentSinkFormat(&m_addonData, req, resp);
    break;

  case KODICall_AudioEngine_Stream_MakeStream:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_MakeStream(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_FreeStream:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_FreeStream(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_GetSpace:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_GetSpace(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_AddData:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_AddData(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_GetDelay:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_GetDelay(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_IsBuffering:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_IsBuffering(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_GetCacheTime:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_GetCacheTime(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_GetCacheTotal:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_GetCacheTotal(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_Pause:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_Pause(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_Resume:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_Resume(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_Drain:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_Drain(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_IsDraining:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_IsDraining(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_IsDrained:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_IsDrained(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_Flush:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_Flush(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_GetVolume:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_GetVolume(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_SetVolume:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_SetVolume(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_GetAmplification:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_GetAmplification(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_SetAmplification:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_SetAmplification(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_GetFrameSize:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_GetFrameSize(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_GetChannelCount:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_GetChannelCount(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_GetSampleRate:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_GetSampleRate(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_GetDataFormat:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_GetDataFormat(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_GetResampleRatio:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_GetResampleRatio(&m_addonData, req, resp);
    break;
  case KODICall_AudioEngine_Stream_SetResampleRatio:
    result = V2::KodiAPI::CAddonExeCB_AudioEngine_Stream::AE_Stream_SetResampleRatio(&m_addonData, req, resp);
    break;

  case KODICall_GUI_General_Lock:
    result = V2::KodiAPI::CAddonExeCB_GUI_General::GUIGeneral_Lock(&m_addonData, req, resp);
    break;
  case KODICall_GUI_General_Unlock:
    result = V2::KodiAPI::CAddonExeCB_GUI_General::GUIGeneral_Unlock(&m_addonData, req, resp);
    break;
  case KODICall_GUI_General_GetScreenHeight:
    result = V2::KodiAPI::CAddonExeCB_GUI_General::GUIGeneral_GetScreenHeight(&m_addonData, req, resp);
    break;
  case KODICall_GUI_General_GetScreenWidth:
    result = V2::KodiAPI::CAddonExeCB_GUI_General::GUIGeneral_GetScreenWidth(&m_addonData, req, resp);
    break;
  case KODICall_GUI_General_GetVideoResolution:
    result = V2::KodiAPI::CAddonExeCB_GUI_General::GUIGeneral_GetVideoResolution(&m_addonData, req, resp);
    break;
  case KODICall_GUI_General_GetCurrentWindowDialogId:
    result = V2::KodiAPI::CAddonExeCB_GUI_General::GUIGeneral_GetCurrentWindowDialogId(&m_addonData, req, resp);
    break;
  case KODICall_GUI_General_GetCurrentWindowId:
    result = V2::KodiAPI::CAddonExeCB_GUI_General::GUIGeneral_GetCurrentWindowId(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Window_New:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_New(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_Delete:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_Delete(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_Show:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_Show(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_Close:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_Close(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_DoModal:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_DoModal(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_SetFocusId:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_SetFocusId(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetFocusId:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetFocusId(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_SetProperty:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_SetProperty(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_SetPropertyInt:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_SetPropertyInt(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_SetPropertyBool:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_SetPropertyBool(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_SetPropertyDouble:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_SetPropertyDouble(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetProperty:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetProperty(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetPropertyInt:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetPropertyInt(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetPropertyBool:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetPropertyBool(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetPropertyDouble:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetPropertyDouble(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_ClearProperties:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_ClearProperties(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_ClearProperty:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_ClearProperty(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetListSize:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetListSize(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_ClearList:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_ClearList(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_AddStringItem:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_AddStringItem(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_AddItem:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_AddItem(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_RemoveItem:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_RemoveItem(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_RemoveItemFile:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_RemoveItemFile(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetListItem:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetListItem(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_SetCurrentListPosition:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_SetCurrentListPosition(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetCurrentListPosition:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetCurrentListPosition(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_SetControlLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_SetControlLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_MarkDirtyRegion:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_MarkDirtyRegion(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_SetCallbacks:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_SetCallbacks(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetControl_Button:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetControl_Button(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetControl_Edit:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetControl_Edit(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetControl_FadeLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetControl_FadeLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetControl_Image:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetControl_Image(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetControl_Label:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetControl_Label(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetControl_Progress:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetControl_Progress(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetControl_RadioButton:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetControl_RadioButton(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetControl_Rendering:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetControl_Rendering(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetControl_SettingsSlider:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetControl_SettingsSlider(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetControl_Slider:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetControl_Slider(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetControl_Spin:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetControl_Spin(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Window_GetControl_TextBox:
    result = V2::KodiAPI::CAddonExeCB_GUI_Window::Window_GetControl_TextBox(&m_addonData, req, resp);
    break;

  case KODICall_GUI_ListItem_Create:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_Create(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_Destroy:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_Destroy(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_GetLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_GetLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_GetLabel2:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_GetLabel2(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetLabel2:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetLabel2(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_GetIconImage:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_GetIconImage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetIconImage:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetIconImage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_GetOverlayImage:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_GetOverlayImage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetOverlayImage:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetOverlayImage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetThumbnailImage:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetThumbnailImage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetArt:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetArt(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetArtFallback:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetArtFallback(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_HasArt:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_HasArt(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_Select:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_Select(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_IsSelected:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_IsSelected(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_HasIcon:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_HasIcon(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_HasOverlay:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_HasOverlay(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_IsFileItem:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_IsFileItem(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_IsFolder:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_IsFolder(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetProperty:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetProperty(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_GetProperty:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_GetProperty(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_ClearProperty:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_ClearProperty(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_ClearProperties:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_ClearProperties(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_HasProperties:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_HasProperties(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_HasProperty:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_HasProperty(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetPath:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetPath(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_GetPath:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_GetPath(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_GetDuration:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_GetDuration(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetSubtitles:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetSubtitles(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetMimeType:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetMimeType(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetContentLookup:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetContentLookup(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_AddContextMenuItems:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_AddContextMenuItems(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_AddStreamInfo:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_AddStreamInfo(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetMusicInfo_BOOL:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetMusicInfo_BOOL(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetMusicInfo_INT:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetMusicInfo_INT(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetMusicInfo_UINT:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetMusicInfo_UINT(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetMusicInfo_FLOAT:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetMusicInfo_FLOAT(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetMusicInfo_STRING:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetMusicInfo_STRING(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetMusicInfo_STRING_LIST:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetMusicInfo_STRING_LIST(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetVideoInfo_BOOL:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetVideoInfo_BOOL(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetVideoInfo_INT:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetVideoInfo_INT(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetVideoInfo_UINT:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetVideoInfo_UINT(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetVideoInfo_FLOAT:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetVideoInfo_FLOAT(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetVideoInfo_STRING:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetVideoInfo_STRING(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetVideoInfo_STRING_LIST:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetVideoInfo_STRING_LIST(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetVideoInfo_Resume:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetVideoInfo_Resume(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetVideoInfo_Cast:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetVideoInfo_Cast(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetPictureInfo_BOOL:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetPictureInfo_BOOL(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetPictureInfo_INT:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetPictureInfo_INT(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetPictureInfo_UINT:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetPictureInfo_UINT(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetPictureInfo_FLOAT:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetPictureInfo_FLOAT(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetPictureInfo_STRING:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetPictureInfo_STRING(&m_addonData, req, resp);
    break;
  case KODICall_GUI_ListItem_SetPictureInfo_STRING_LIST:
    result = V2::KodiAPI::CAddonExeCB_GUI_ListItem::ListItem_SetPictureInfo_STRING_LIST(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Control_Button_SetVisible:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlButton::Control_Button_SetVisible(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Button_SetEnabled:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlButton::Control_Button_SetEnabled(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Button_SetLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlButton::Control_Button_SetLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Button_GetLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlButton::Control_Button_GetLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Button_SetLabel2:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlButton::Control_Button_SetLabel2(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Button_GetLabel2:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlButton::Control_Button_GetLabel2(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Control_Edit_SetVisible:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlEdit::Control_Edit_SetVisible(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Edit_SetEnabled:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlEdit::Control_Edit_SetEnabled(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Edit_SetLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlEdit::Control_Edit_SetLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Edit_GetLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlEdit::Control_Edit_GetLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Edit_SetText:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlEdit::Control_Edit_SetText(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Edit_GetText:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlEdit::Control_Edit_GetText(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Edit_SetCursorPosition:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlEdit::Control_Edit_SetCursorPosition(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Edit_GetCursorPosition:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlEdit::Control_Edit_GetCursorPosition(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Edit_SetInputType:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlEdit::Control_Edit_SetInputType(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Control_FadeLabel_SetVisible:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlFadeLabel::Control_FadeLabel_SetVisible(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_FadeLabel_AddLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlFadeLabel::Control_FadeLabel_AddLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_FadeLabel_GetLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlFadeLabel::Control_FadeLabel_GetLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_FadeLabel_SetScrolling:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlFadeLabel::Control_FadeLabel_SetScrolling(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_FadeLabel_Reset:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlFadeLabel::Control_FadeLabel_Reset(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Control_Image_SetVisible:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlImage::Control_Image_SetVisible(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Image_SetFileName:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlImage::Control_Image_SetFileName(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Image_SetColorDiffuse:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlImage::Control_Image_SetColorDiffuse(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Control_Label_SetVisible:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlLabel::Control_Label_SetVisible(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Label_SetLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlLabel::Control_Label_SetLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Label_GetLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlLabel::Control_Label_GetLabel(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Control_Progress_SetVisible:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlProgress::Control_Progress_SetVisible(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Progress_SetPercentage:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlProgress::Control_Progress_SetPercentage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Progress_GetPercentage:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlProgress::Control_Progress_GetPercentage(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Control_RadioButton_SetVisible:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlRadioButton::Control_RadioButton_SetVisible(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_RadioButton_SetEnabled:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlRadioButton::Control_RadioButton_SetEnabled(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_RadioButton_SetLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlRadioButton::Control_RadioButton_SetLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_RadioButton_GetLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlRadioButton::Control_RadioButton_GetLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_RadioButton_SetSelected:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlRadioButton::Control_RadioButton_SetSelected(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_RadioButton_IsSelected:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlRadioButton::Control_RadioButton_IsSelected(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Control_Rendering_Delete:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlRendering::Control_Rendering_Delete(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Rendering_SetCallbacks:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlRendering::Control_Rendering_SetCallbacks(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Control_SettingsSlider_SetVisible:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_SetVisible(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_SetEnabled:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_SetEnabled(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_SetText:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_SetText(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_Reset:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_Reset(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_SetIntRange:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_SetIntRange(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_SetIntValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_SetIntValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_GetIntValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_GetIntValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_SetIntInterval:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_SetIntInterval(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_SetPercentage:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_SetPercentage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_GetPercentage:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_GetPercentage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_SetFloatRange:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_SetFloatRange(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_SetFloatValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_SetFloatValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_GetFloatValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_GetFloatValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_SettingsSlider_SetFloatInterval:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSettingsSlider::Control_SettingsSlider_SetFloatInterval(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Control_Slider_SetVisible:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_SetVisible(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Slider_SetEnabled:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_SetEnabled(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Slider_GetDescription:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_GetDescription(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Slider_SetIntRange:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_SetIntRange(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Slider_SetIntValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_SetIntValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Slider_GetIntValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_GetIntValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Slider_SetIntInterval:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_SetIntInterval(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Slider_SetPercentage:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_SetPercentage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Slider_GetPercentage:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_GetPercentage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Slider_SetFloatRange:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_SetFloatRange(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Slider_SetFloatValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_SetFloatValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Slider_GetFloatValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_GetFloatValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Slider_SetFloatInterval:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSlider::Control_Slider_SetFloatInterval(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Control_Spin_SetVisible:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_SetVisible(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_SetEnabled:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_SetEnabled(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_SetText:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_SetText(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_Reset:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_Reset(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_SetType:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_SetType(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_AddStringLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_AddStringLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_AddIntLabel:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_AddIntLabel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_SetStringValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_SetStringValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_GetStringValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_GetStringValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_SetIntRange:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_SetIntRange(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_SetIntValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_SetIntValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_GetIntValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_GetIntValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_SetFloatRange:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_SetFloatRange(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_SetFloatValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_SetFloatValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_GetFloatValue:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_GetFloatValue(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_Spin_SetFloatInterval:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlSpin::Control_Spin_SetFloatInterval(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Control_TextBox_SetVisible:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlTextBox::Control_TextBox_SetVisible(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_TextBox_Reset:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlTextBox::Control_TextBox_Reset(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_TextBox_SetText:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlTextBox::Control_TextBox_SetText(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_TextBox_GetText:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlTextBox::Control_TextBox_GetText(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_TextBox_Scroll:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlTextBox::Control_TextBox_Scroll(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Control_TextBox_SetAutoScrolling:
    result = V2::KodiAPI::CAddonExeCB_GUI_ControlTextBox::Control_TextBox_SetAutoScrolling(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Dialogs_ExtendedProgress_New:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_New(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_ExtendedProgress_Delete:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Delete(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_ExtendedProgress_Title:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Title(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_ExtendedProgress_SetTitle:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetTitle(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_ExtendedProgress_Text:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Text(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_ExtendedProgress_SetText:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetText(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_ExtendedProgress_IsFinished:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_IsFinished(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_ExtendedProgress_MarkFinished:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_MarkFinished(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_ExtendedProgress_Percentage:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Percentage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_ExtendedProgress_SetPercentage:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetPercentage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_ExtendedProgress_SetProgress:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetProgress(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Dialogs_FileBrowser_ShowAndGetDirectory:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetDirectory(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_FileBrowser_ShowAndGetFile:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetFile(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_FileBrowser_ShowAndGetFileFromDir:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetFileFromDir(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_FileBrowser_ShowAndGetFileList:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetFileList(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_FileBrowser_ShowAndGetSource:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetSource(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_FileBrowser_ShowAndGetImage:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetImage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_FileBrowser_ShowAndGetImageList:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogFileBrowser::Dialogs_FileBrowser_ShowAndGetImageList(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Dialogs_Keyboard_ShowAndGetInputWithHead:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetInputWithHead(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Keyboard_ShowAndGetInput:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetInput(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Keyboard_ShowAndGetNewPasswordWithHead:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetNewPasswordWithHead(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Keyboard_ShowAndGetNewPassword:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetNewPassword(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Keyboard_ShowAndVerifyNewPasswordWithHead:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndVerifyNewPasswordWithHead(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Keyboard_ShowAndVerifyNewPassword:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndVerifyNewPassword(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Keyboard_ShowAndVerifyPassword:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndVerifyPassword(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Keyboard_ShowAndGetFilter:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_ShowAndGetFilter(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Keyboard_SendTextToActiveKeyboard:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_SendTextToActiveKeyboard(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Keyboard_isKeyboardActivated:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogKeyboard::Dialogs_Keyboard_isKeyboardActivated(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Dialogs_Numeric_ShowAndVerifyNewPassword:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndVerifyNewPassword(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Numeric_ShowAndVerifyPassword:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndVerifyPassword(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Numeric_ShowAndVerifyInput:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndVerifyInput(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Numeric_ShowAndGetTime:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetTime(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Numeric_ShowAndGetDate:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetDate(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Numeric_ShowAndGetIPAddress:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetIPAddress(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Numeric_ShowAndGetNumber:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetNumber(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Numeric_ShowAndGetSeconds:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogNumeric::Dialogs_Numeric_ShowAndGetSeconds(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Dialogs_OK_ShowAndGetInputSingleText:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogOK::Dialogs_OK_ShowAndGetInputSingleText(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_OK_ShowAndGetInputLineText:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogOK::Dialogs_OK_ShowAndGetInputLineText(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Dialogs_Progress_New:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_New(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Progress_Delete:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_Delete(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Progress_Open:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_Open(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Progress_SetHeading:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_SetHeading(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Progress_SetLine:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_SetLine(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Progress_SetCanCancel:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_SetCanCancel(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Progress_IsCanceled:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_IsCanceled(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Progress_SetPercentage:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_SetPercentage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Progress_GetPercentage:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_GetPercentage(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Progress_ShowProgressBar:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_ShowProgressBar(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Progress_SetProgressMax:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_SetProgressMax(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Progress_SetProgressAdvance:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_SetProgressAdvance(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_Progress_Abort:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_Abort(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Dialogs_Select_Show:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogSelect::Dialogs_Select_Show(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Dialogs_TextViewer_Show:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogTextViewer::Dialogs_TextViewer_Show(&m_addonData, req, resp);
    break;

  case KODICall_GUI_Dialogs_YesNo_ShowAndGetInputSingleText:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogYesNo::Dialogs_YesNo_ShowAndGetInputSingleText(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_YesNo_ShowAndGetInputLineText:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogYesNo::Dialogs_YesNo_ShowAndGetInputLineText(&m_addonData, req, resp);
    break;
  case KODICall_GUI_Dialogs_YesNo_ShowAndGetInputLineButtonText:
    result = V2::KodiAPI::CAddonExeCB_GUI_DialogYesNo::Dialogs_YesNo_ShowAndGetInputLineButtonText(&m_addonData, req, resp);
    break;

  case KODICall_Player_AddonInfoTagMusic_GetFromPlayer:
    result = V2::KodiAPI::CAddonExeCB_Player_InfoTagMusic::AddonInfoTagMusic_GetFromPlayer(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonInfoTagMusic_Release:
    result = V2::KodiAPI::CAddonExeCB_Player_InfoTagMusic::AddonInfoTagMusic_Release(&m_addonData, req, resp);
    break;

  case KODICall_Player_AddonInfoTagVideo_GetFromPlayer:
    result = V2::KodiAPI::CAddonExeCB_Player_InfoTagVideo::AddonInfoTagVideo_GetFromPlayer(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonInfoTagVideo_Release:
    result = V2::KodiAPI::CAddonExeCB_Player_InfoTagVideo::AddonInfoTagVideo_Release(&m_addonData, req, resp);
    break;

  case KODICall_Player_PlayList_New:
    result = V2::KodiAPI::CAddonExeCB_Player_PlayList::PlayList_New(&m_addonData, req, resp);
    break;
  case KODICall_Player_PlayList_Delete:
    result = V2::KodiAPI::CAddonExeCB_Player_PlayList::PlayList_Delete(&m_addonData, req, resp);
    break;
  case KODICall_Player_PlayList_LoadPlaylist:
    result = V2::KodiAPI::CAddonExeCB_Player_PlayList::PlayList_LoadPlaylist(&m_addonData, req, resp);
    break;
  case KODICall_Player_PlayList_AddItemURL:
    result = V2::KodiAPI::CAddonExeCB_Player_PlayList::PlayList_AddItemURL(&m_addonData, req, resp);
    break;
  case KODICall_Player_PlayList_AddItemList:
    result = V2::KodiAPI::CAddonExeCB_Player_PlayList::PlayList_AddItemList(&m_addonData, req, resp);
    break;
  case KODICall_Player_PlayList_RemoveItem:
    result = V2::KodiAPI::CAddonExeCB_Player_PlayList::PlayList_RemoveItem(&m_addonData, req, resp);
    break;
  case KODICall_Player_PlayList_ClearList:
    result = V2::KodiAPI::CAddonExeCB_Player_PlayList::PlayList_ClearList(&m_addonData, req, resp);
    break;
  case KODICall_Player_PlayList_GetListSize:
    result = V2::KodiAPI::CAddonExeCB_Player_PlayList::PlayList_GetListSize(&m_addonData, req, resp);
    break;
  case KODICall_Player_PlayList_GetListPosition:
    result = V2::KodiAPI::CAddonExeCB_Player_PlayList::PlayList_GetListPosition(&m_addonData, req, resp);
    break;
  case KODICall_Player_PlayList_Shuffle:
    result = V2::KodiAPI::CAddonExeCB_Player_PlayList::PlayList_Shuffle(&m_addonData, req, resp);
    break;
  case KODICall_Player_PlayList_GetItem:
    result = V2::KodiAPI::CAddonExeCB_Player_PlayList::PlayList_GetItem(&m_addonData, req, resp);
    break;

  case KODICall_Player_AddonPlayer_GetSupportedMedia:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_GetSupportedMedia(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_New:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_New(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_Delete:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_Delete(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_SetCallbacks:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_SetCallbacks(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_PlayFile:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_PlayFile(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_PlayFileItem:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_PlayFileItem(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_PlayList:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_PlayList(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_Stop:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_Stop(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_Pause:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_Pause(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_PlayNext:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_PlayNext(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_PlayPrevious:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_PlayPrevious(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_PlaySelected:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_PlaySelected(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_IsPlaying:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_IsPlaying(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_IsPlayingAudio:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_IsPlayingAudio(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_IsPlayingVideo:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_IsPlayingVideo(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_IsPlayingRDS:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_IsPlayingRDS(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_GetPlayingFile:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_GetPlayingFile(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_GetTotalTime:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_GetTotalTime(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_GetTime:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_GetTime(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_SeekTime:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_SeekTime(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_GetAvailableVideoStreams:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_GetAvailableVideoStreams(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_SetVideoStream:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_SetVideoStream(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_GetAvailableAudioStreams:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_GetAvailableAudioStreams(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_SetAudioStream:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_SetAudioStream(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_GetAvailableSubtitleStreams:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_GetAvailableSubtitleStreams(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_SetSubtitleStream:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_SetSubtitleStream(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_ShowSubtitles:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_ShowSubtitles(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_GetCurrentSubtitleName:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_GetCurrentSubtitleName(&m_addonData, req, resp);
    break;
  case KODICall_Player_AddonPlayer_AddSubtitle:
    result = V2::KodiAPI::CAddonExeCB_Player_Player::AddonPlayer_AddSubtitle(&m_addonData, req, resp);
    break;

  case KODICall_PVR_AddMenuHook:
    result = V2::KodiAPI::CAddonExeCB_PVR_General::AddMenuHook(&m_addonData, req, resp);
    break;
  case KODICall_PVR_Recording:
    result = V2::KodiAPI::CAddonExeCB_PVR_General::Recording(&m_addonData, req, resp);
    break;

  case KODICall_PVR_EpgEntry:
    result = V2::KodiAPI::CAddonExeCB_PVR_Transfer::EpgEntry(&m_addonData, req, resp);
    break;
  case KODICall_PVR_ChannelEntry:
    result = V2::KodiAPI::CAddonExeCB_PVR_Transfer::ChannelEntry(&m_addonData, req, resp);
    break;
  case KODICall_PVR_TimerEntry:
    result = V2::KodiAPI::CAddonExeCB_PVR_Transfer::TimerEntry(&m_addonData, req, resp);
    break;
  case KODICall_PVR_RecordingEntry:
    result = V2::KodiAPI::CAddonExeCB_PVR_Transfer::RecordingEntry(&m_addonData, req, resp);
    break;
  case KODICall_PVR_ChannelGroup:
    result = V2::KodiAPI::CAddonExeCB_PVR_Transfer::ChannelGroup(&m_addonData, req, resp);
    break;
  case KODICall_PVR_ChannelGroupMember:
    result = V2::KodiAPI::CAddonExeCB_PVR_Transfer::ChannelGroupMember(&m_addonData, req, resp);
    break;

  case KODICall_PVR_TriggerTimerUpdate:
    result = V2::KodiAPI::CAddonExeCB_PVR_Trigger::TriggerTimerUpdate(&m_addonData, req, resp);
    break;
  case KODICall_PVR_TriggerRecordingUpdate:
    result = V2::KodiAPI::CAddonExeCB_PVR_Trigger::TriggerRecordingUpdate(&m_addonData, req, resp);
    break;
  case KODICall_PVR_TriggerChannelUpdate:
    result = V2::KodiAPI::CAddonExeCB_PVR_Trigger::TriggerChannelUpdate(&m_addonData, req, resp);
    break;
  case KODICall_PVR_TriggerEpgUpdate:
    result = V2::KodiAPI::CAddonExeCB_PVR_Trigger::TriggerEpgUpdate(&m_addonData, req, resp);
    break;
  case KODICall_PVR_TriggerChannelGroupsUpdate:
    result = V2::KodiAPI::CAddonExeCB_PVR_Trigger::TriggerChannelGroupsUpdate(&m_addonData, req, resp);
    break;

  default:
    CLog::Log(LOGERROR, "Invalid add-on command opcode %i", req.getOpCode());
    break;
  }
  return result;
}

bool CBinaryAddon::process_Login(CRequestPacket& req, CResponsePacket& resp)
{
  if (req.getDataLength() <= 4)
    return false;

  char* str;
  bool useNet;
  req.pop(API_UINT32_T, &m_addonAPILevel);
  m_addonAPIVersion    = (char*)req.pop(API_STRING, &str);
  req.pop(API_UINT64_T, &m_addonAPIpid);
  m_addonId            = (char*)req.pop(API_STRING, &str);
  m_addonType          = (char*)req.pop(API_STRING, &str);
  m_addonVersion       = (char*)req.pop(API_STRING, &str);
  m_addonName          = (char*)req.pop(API_STRING, &str);

  if (!m_isSubThread)
  {
    CLog::Log(LOGINFO, "Binary AddOn: Welcome %s Add-on '%s' with API Level '%u' (Version: %s)",
                m_isIndependent ? "independent" : "child",
                m_addonName.c_str(),
                m_addonAPILevel,
                m_addonAPIVersion.c_str());
  }

  AddonProps addonProps(m_addonName,
                        TranslateType(m_addonType),
                        m_addonVersion,
                        m_addonVersion);

  addonProps.name         = m_addonName;
  addonProps.license      = (char*)req.pop(API_STRING, &str);
  addonProps.summary      = (char*)req.pop(API_STRING, &str);
  addonProps.description  = (char*)req.pop(API_STRING, &str);
  addonProps.path         = (char*)req.pop(API_STRING, &str);
  addonProps.libname      = (char*)req.pop(API_STRING, &str);
  addonProps.author       = (char*)req.pop(API_STRING, &str);
  addonProps.source       = (char*)req.pop(API_STRING, &str);
  addonProps.icon         = (char*)req.pop(API_STRING, &str);
  addonProps.disclaimer   = (char*)req.pop(API_STRING, &str);
  addonProps.changelog    = (char*)req.pop(API_STRING, &str);
  addonProps.fanart       = (char*)req.pop(API_STRING, &str);
  req.pop(API_BOOLEAN,  &m_isIndependent);
  req.pop(API_BOOLEAN,  &useNet);
  req.pop(API_BOOLEAN,  &m_isSubThread);

  m_addon               = new CAddon(addonProps);
  m_pHelpers            = new CAddonCallbacks(m_addon);
  m_addonData.addonData = m_pHelpers;
  m_addonCB             = (V2::KodiAPI::CB_AddOnLib*)CAddonCallbacksAddonBase::CreateHelper(m_pHelpers, 2);

  srand(clock());
  int     usedRand      = std::rand();
  int     sharedMemOK   = (!useNet && m_isLocal && StartSharedMemoryThread(usedRand)) ? 1 : 0;
  uint32_t netStreamOK  = (m_addonAPILevel <= V2::KodiAPI::KODI_API_Level ? API_SUCCESS : API_ERR_REQUEST);
  if (netStreamOK != API_SUCCESS)
  {
    m_active = false;
    CLog::Log(LOGERROR, "Binary AddOn: Add-on '%s' have a not allowed API level '%u' (Version: %s), terminating client",
                m_addonName.c_str(),
                m_addonAPILevel,
                m_addonAPIVersion.c_str());
  }
  else
  {
    m_active = true;
    SetLoggedIn(true);
  }

  // Send the login reply
  resp.init(req.getRequestID());
  resp.push(API_UINT32_T, &netStreamOK);
  resp.push(API_INT,      &usedRand);
  resp.push(API_UINT32_T, &V2::KodiAPI::KODI_API_Level);
  resp.push(API_STRING,    CCompileInfo::GetAppName());
  resp.push(API_STRING,    StringUtils::Format("%u.%u", CCompileInfo::GetMajor(), CCompileInfo::GetMinor()).c_str());
  resp.push(API_INT,  &sharedMemOK);
  int sharedMemSize = m_sharedMem ? m_sharedMem->m_sharedMemSize : 0;
  resp.push(API_INT,      &sharedMemSize);
  resp.finalise();

  return true;
}

bool CBinaryAddon::process_Logout(CRequestPacket& req, CResponsePacket& resp)
{
  ADDON_NET_SEND_RETURN(req.getRequestID(), API_SUCCESS);
  for (std::vector<CBinaryAddon*>::iterator itr = m_childThreads.begin(); itr != m_childThreads.end();)
  {
    delete (*itr);
    itr = m_childThreads.erase(itr);
  }

  m_active = false;

  if (m_isFather)
  {
    if (m_sharedMem)
      delete m_sharedMem;
    if (m_addon)
      delete m_addon;
    if (m_pHelpers)
    {
      CAddonCallbacksAddonBase::DestroyHelper(m_pHelpers);
      delete m_pHelpers;
    }
  }

  m_sharedMem = nullptr;
  m_addon     = nullptr;
  m_pHelpers  = nullptr;
  m_addonCB   = nullptr;

  return true;
}

bool CBinaryAddon::process_Ping(CRequestPacket& req, CResponsePacket& resp)
{
  ADDON_NET_SEND_RETURN(req.getRequestID(), API_SUCCESS);
  return true;
}

bool CBinaryAddon::process_Log(CRequestPacket& req, CResponsePacket& resp)
{
  uint32_t logLevel;
  char* str;
  req.pop(API_UINT32_T, &logLevel);
  req.pop(API_STRING, &str);
  m_addonCB->addon_log_msg(&m_addonData, (V2::KodiAPI::addon_log)logLevel, str);
  ADDON_NET_SEND_RETURN(req.getRequestID(), API_SUCCESS);
  return true;
}

bool CBinaryAddon::process_CreateSubThread(CRequestPacket& req, CResponsePacket& resp)
{
  int rand;
  req.pop(API_INT, &rand);

  CBinaryAddon *childThread = new CBinaryAddon(this, rand);
  uint32_t ret = childThread->StartSharedMemoryThread(rand) ? API_SUCCESS : API_ERR_OP;
  if (ret == API_SUCCESS)
  {
    m_childThreads.push_back(childThread);
    childThread->m_active       = true;
    childThread->m_isSubThread  = true;
  }
  else
  {
    delete childThread;
  }
  resp.init(req.getRequestID());
  resp.push(API_UINT32_T, &ret);
  resp.finalise();

  return true;
}

bool CBinaryAddon::process_DeleteSubThread(CRequestPacket& req, CResponsePacket& resp)
{
  int rand;
  req.pop(API_INT, &rand);

  uint32_t ret = API_ERR_UNKNOWN;
  for (std::vector<CBinaryAddon*>::const_iterator itr = m_childThreads.begin(); itr != m_childThreads.end(); ++itr)
  {
    if ((*itr)->m_randomConnectionNumber == rand)
    {
      delete (*itr);
      m_childThreads.erase(itr);
      ret = API_SUCCESS;
      break;
    }
  }
  resp.init(req.getRequestID());
  resp.push(API_UINT32_T, &ret);
  resp.finalise();

  return true;
}

//--- - --- - --- - --- - --- - --- - --- - --- - --- - --- - --- - --- - ---

bool CBinaryAddon::addon_Ping()
{
  try
  {
    // if (!m_sharedMem->m_sharedMem_KodiToAddon)
    //   return false;
    //
    // m_sharedMem->m_sharedMem_KodiToAddon->message.messageId = KODICall_Ping;
    // m_sharedMem->m_sharedMem_KodiToAddon->message.retValue.bool_par = false;
    // m_sharedMem->Unlock_KodiToAddon_Addon();
    // m_sharedMem->Lock_KodiToAddon_Kodi();
    // bool ret = m_sharedMem->m_sharedMem_KodiToAddon->message.retValue.bool_par;
    // m_sharedMem->m_sharedMem_KodiToAddon->message.retValue.bool_par = false;
    // return ret;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Binary AddOn: %s - Undefined exception", __FUNCTION__); // Show with normal output to prevent possible dead loops
  }
  return false;
}

} /* namespace ADDON */
