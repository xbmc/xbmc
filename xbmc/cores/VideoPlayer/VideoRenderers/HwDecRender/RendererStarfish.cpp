/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RendererStarfish.h"

#include "../RenderFactory.h"
#include "DVDCodecs/Video/DVDVideoCodecStarfish.h"
#include "ServiceBroker.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "settings/MediaSettings.h"
#include "utils/log.h"

CRendererStarfish::CRendererStarfish()
{
  CLog::Log(LOGINFO, "Instancing CRendererStarfish");
}

CRendererStarfish::~CRendererStarfish()
{
}

CBaseRenderer* CRendererStarfish::Create(CVideoBuffer* buffer)
{
  if (buffer && dynamic_cast<CStarfishVideoBuffer*>(buffer))
    return new CRendererStarfish();
  return nullptr;
}

bool CRendererStarfish::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("starfish", CRendererStarfish::Create);
  return true;
}

void CRendererStarfish::AddVideoPicture(const VideoPicture& picture, int index)
{
}

void CRendererStarfish::ReleaseBuffer(int idx)
{
}

CRenderInfo CRendererStarfish::GetRenderInfo()
{
  CRenderInfo info;
  info.max_buffer_size = 4;
  info.optimal_buffer_size = 3;
  return info;
}

bool CRendererStarfish::LoadShadersHook()
{
  return true;
}

bool CRendererStarfish::RenderHook(int index)
{
  return true;
}

bool CRendererStarfish::CreateTexture(int index)
{
  return true;
}

void CRendererStarfish::DeleteTexture(int index)
{
}

bool CRendererStarfish::UploadTexture(int index)
{
  return true;
}

bool CRendererStarfish::IsGuiLayer()
{
  return false;
}

void CRendererStarfish::RenderUpdateVideo(bool clear, unsigned int flags, unsigned int alpha)
{
}
