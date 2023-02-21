/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameSettingsHandle.h"

#include "GUIGameRenderManager.h"

using namespace KODI;
using namespace RETRO;

CGUIGameSettingsHandle::CGUIGameSettingsHandle(CGUIGameRenderManager& renderManager)
  : m_renderManager(renderManager)
{
}

CGUIGameSettingsHandle::~CGUIGameSettingsHandle()
{
  m_renderManager.UnregisterHandle(this);
}

std::string CGUIGameSettingsHandle::GameClientID()
{
  return m_renderManager.GameClientID();
}

std::string CGUIGameSettingsHandle::GetPlayingGame()
{
  return m_renderManager.GetPlayingGame();
}

std::string CGUIGameSettingsHandle::CreateSavestate(bool autosave)
{
  return m_renderManager.CreateSavestate(autosave);
}

bool CGUIGameSettingsHandle::UpdateSavestate(const std::string& savestatePath)
{
  return m_renderManager.UpdateSavestate(savestatePath);
}

bool CGUIGameSettingsHandle::LoadSavestate(const std::string& savestatePath)
{
  return m_renderManager.LoadSavestate(savestatePath);
}

void CGUIGameSettingsHandle::FreeSavestateResources(const std::string& savestatePath)
{
  return m_renderManager.FreeSavestateResources(savestatePath);
}

void CGUIGameSettingsHandle::CloseOSD()
{
  m_renderManager.CloseOSD();
}
