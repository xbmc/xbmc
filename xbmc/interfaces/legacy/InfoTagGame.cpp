/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoTagGame.h"

#include "AddonUtils.h"
#include "games/tags/GameInfoTag.h"

using namespace KODI::GAME;
using namespace XBMCAddonUtils;

namespace XBMCAddon
{
namespace xbmc
{

InfoTagGame::InfoTagGame(bool offscreen /* = false */)
  : infoTag(new CGameInfoTag), offscreen(offscreen), owned(true)
{
}

InfoTagGame::InfoTagGame(const CGameInfoTag* tag)
  : infoTag(new CGameInfoTag(*tag)), offscreen(true), owned(true)
{
}

InfoTagGame::InfoTagGame(CGameInfoTag* tag, bool offscreen /* = false */)
  : infoTag(tag), offscreen(offscreen), owned(false)
{
}

InfoTagGame::~InfoTagGame()
{
  if (owned)
    delete infoTag;
}

String InfoTagGame::getTitle()
{
  return infoTag->GetTitle();
}

String InfoTagGame::getPlatform()
{
  return infoTag->GetPlatform();
}

std::vector<String> InfoTagGame::getGenres()
{
  return infoTag->GetGenres();
}

String InfoTagGame::getPublisher()
{
  return infoTag->GetPublisher();
}

String InfoTagGame::getDeveloper()
{
  return infoTag->GetDeveloper();
}

String InfoTagGame::getOverview()
{
  return infoTag->GetOverview();
}

unsigned int InfoTagGame::getYear()
{
  return infoTag->GetYear();
}

String InfoTagGame::getGameClient()
{
  return infoTag->GetGameClient();
}

void InfoTagGame::setTitle(const String& title)
{
  XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
  setTitleRaw(infoTag, title);
}

void InfoTagGame::setPlatform(const String& platform)
{
  XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
  setPlatformRaw(infoTag, platform);
}

void InfoTagGame::setGenres(const std::vector<String>& genres)
{
  XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
  setGenresRaw(infoTag, genres);
}

void InfoTagGame::setPublisher(const String& publisher)
{
  XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
  setPublisherRaw(infoTag, publisher);
}

void InfoTagGame::setDeveloper(const String& developer)
{
  XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
  setDeveloperRaw(infoTag, developer);
}

void InfoTagGame::setOverview(const String& overview)
{
  XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
  setOverviewRaw(infoTag, overview);
}

void InfoTagGame::setYear(unsigned int year)
{
  XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
  setYearRaw(infoTag, year);
}

void InfoTagGame::setGameClient(const String& gameClient)
{
  XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
  setGameClientRaw(infoTag, gameClient);
}

void InfoTagGame::setTitleRaw(KODI::GAME::CGameInfoTag* infoTag, const String& title)
{
  infoTag->SetTitle(title);
}

void InfoTagGame::setPlatformRaw(KODI::GAME::CGameInfoTag* infoTag, const String& platform)
{
  infoTag->SetPlatform(platform);
}

void InfoTagGame::setGenresRaw(KODI::GAME::CGameInfoTag* infoTag, const std::vector<String>& genres)
{
  infoTag->SetGenres(genres);
}

void InfoTagGame::setPublisherRaw(KODI::GAME::CGameInfoTag* infoTag, const String& publisher)
{
  infoTag->SetPublisher(publisher);
}

void InfoTagGame::setDeveloperRaw(KODI::GAME::CGameInfoTag* infoTag, const String& developer)
{
  infoTag->SetDeveloper(developer);
}

void InfoTagGame::setOverviewRaw(KODI::GAME::CGameInfoTag* infoTag, const String& overview)
{
  infoTag->SetOverview(overview);
}

void InfoTagGame::setYearRaw(KODI::GAME::CGameInfoTag* infoTag, unsigned int year)
{
  infoTag->SetYear(year);
}

void InfoTagGame::setGameClientRaw(KODI::GAME::CGameInfoTag* infoTag, const String& gameClient)
{
  infoTag->SetGameClient(gameClient);
}

} // namespace xbmc
} // namespace XBMCAddon
