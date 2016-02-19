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

#include "addons/binary/binary-executable/requestpacket.h"
#include "addons/binary/binary-executable/responsepacket.h"

struct AddonCB;

namespace V2
{
namespace KodiAPI
{

struct CAddonExeCB_Addon_VFSUtils
{
  static bool CreateDirectory(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool DirectoryExists(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool RemoveDirectory(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetVFSDirectory(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool FreeVFSDirectory(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetFileMD5(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetCacheThumbName(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool MakeLegalFileName(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool MakeLegalPath(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool OpenFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool OpenFileForWrite(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool ReadFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool ReadFileString(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool WriteFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool FlushFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool SeekFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool TruncateFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetFilePosition(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetFileLength(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool CloseFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetFileChunkSize(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool FileExists(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool StatFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool DeleteFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
};

}; /* namespace KodiAPI */
}; /* namespace V2 */
