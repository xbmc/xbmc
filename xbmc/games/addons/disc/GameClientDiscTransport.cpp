/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscTransport.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "games/addons/GameClient.h"

#include <mutex>

using namespace KODI;
using namespace GAME;

CGameClientDiscTransport::CGameClientDiscTransport(CGameClient& gameClient,
                                                   AddonInstance_Game& addonStruct,
                                                   CCriticalSection& clientAccess)
  : m_gameClient(gameClient),
    m_struct(addonStruct),
    m_clientAccess(clientAccess)
{
}

bool CGameClientDiscTransport::GetEjectState()
{
  bool bEjected = true;

  std::unique_lock lock(m_clientAccess);

  try
  {
    bEjected = m_struct.toAddon->GetEjectState(&m_struct);
  }
  catch (...)
  {
    m_gameClient.LogException("GetEjectState()");
  }

  return bEjected;
}

bool CGameClientDiscTransport::SetEjectState(bool ejected)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  std::unique_lock lock(m_clientAccess);

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->SetEjectState(&m_struct, ejected),
                          "SetEjectState()");
  }
  catch (...)
  {
    m_gameClient.LogException("SetEjectState()");
  }

  return error == GAME_ERROR_NO_ERROR;
}

unsigned int CGameClientDiscTransport::GetImageIndex()
{
  unsigned int imageIndex = 0;

  std::unique_lock lock(m_clientAccess);

  try
  {
    imageIndex = m_struct.toAddon->GetImageIndex(&m_struct);
  }
  catch (...)
  {
    m_gameClient.LogException("GetImageIndex()");
  }

  return imageIndex;
}

bool CGameClientDiscTransport::SetImageIndex(unsigned int imageIndex)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  std::unique_lock lock(m_clientAccess);

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->SetImageIndex(&m_struct, imageIndex),
                          "SetImageIndex()");
  }
  catch (...)
  {
    m_gameClient.LogException("SetImageIndex()");
  }

  return error == GAME_ERROR_NO_ERROR;
}

unsigned int CGameClientDiscTransport::GetImageCount()
{
  unsigned int imageCount = 0;

  std::unique_lock lock(m_clientAccess);

  try
  {
    imageCount = m_struct.toAddon->GetImageCount(&m_struct);
  }
  catch (...)
  {
    m_gameClient.LogException("GetImageCount()");
  }

  return imageCount;
}

bool CGameClientDiscTransport::AddImageIndex()
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  std::unique_lock lock(m_clientAccess);

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->AddImageIndex(&m_struct), "AddImageIndex()");
  }
  catch (...)
  {
    m_gameClient.LogException("AddImageIndex()");
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientDiscTransport::ReplaceImageIndex(unsigned int imageIndex,
                                                 const std::string& filePath)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  std::unique_lock lock(m_clientAccess);

  try
  {
    m_gameClient.LogError(
        error = m_struct.toAddon->ReplaceImageIndex(&m_struct, imageIndex, filePath.c_str()),
        "ReplaceImageIndex()");
  }
  catch (...)
  {
    m_gameClient.LogException("ReplaceImageIndex()");
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientDiscTransport::RemoveImageIndex(unsigned int imageIndex)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  std::unique_lock lock(m_clientAccess);

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RemoveImageIndex(&m_struct, imageIndex),
                          "RemoveImageIndex()");
  }
  catch (...)
  {
    m_gameClient.LogException("RemoveImageIndex()");
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientDiscTransport::SetInitialImage(unsigned int imageIndex, const std::string& filePath)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  std::unique_lock lock(m_clientAccess);

  try
  {
    m_gameClient.LogError(
        error = m_struct.toAddon->SetInitialImage(&m_struct, imageIndex, filePath.c_str()),
        "SetInitialImage()");
  }
  catch (...)
  {
    m_gameClient.LogException("SetInitialImage()");
  }

  return error == GAME_ERROR_NO_ERROR;
}

std::string CGameClientDiscTransport::GetImagePath(unsigned int imageIndex)
{
  std::string imagePath;
  char* imagePathRaw = nullptr;

  std::unique_lock lock(m_clientAccess);

  try
  {
    imagePathRaw = m_struct.toAddon->GetImagePath(&m_struct, imageIndex);
  }
  catch (...)
  {
    m_gameClient.LogException("GetImagePath()");
  }

  if (imagePathRaw)
  {
    imagePath = imagePathRaw;
    m_struct.toAddon->FreeString(&m_struct, imagePathRaw);
  }

  return imagePath;
}

std::string CGameClientDiscTransport::GetImageLabel(unsigned int imageIndex)
{
  std::string imageLabel;
  char* imageLabelRaw = nullptr;

  std::unique_lock lock(m_clientAccess);

  try
  {
    imageLabelRaw = m_struct.toAddon->GetImageLabel(&m_struct, imageIndex);
  }
  catch (...)
  {
    m_gameClient.LogException("GetImageLabel()");
  }

  if (imageLabelRaw)
  {
    imageLabel = imageLabelRaw;
    m_struct.toAddon->FreeString(&m_struct, imageLabelRaw);
  }

  return imageLabel;
}
