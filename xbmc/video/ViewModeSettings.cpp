/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ViewModeSettings.h"

#include "cores/IPlayer.h"
#include "guilib/LocalizeStrings.h"
#include "settings/VideoSettings.h"

struct ViewModeProperties
{
  int stringIndex;
  int viewMode;
  bool hideFromQuickCycle;
  bool hideFromList;
};

#define HIDE_ITEM true

/** The list of all the view modes along with their properties
 */
static const ViewModeProperties viewModes[] =
{
  { 630,   ViewModeNormal },
  { 631,   ViewModeZoom },
  { 39008, ViewModeZoom120Width },
  { 39009, ViewModeZoom110Width },
  { 632,   ViewModeStretch4x3 },
  { 633,   ViewModeWideZoom },
  { 634,   ViewModeStretch16x9 },
  { 644,   ViewModeStretch16x9Nonlin, HIDE_ITEM, HIDE_ITEM },
  { 635,   ViewModeOriginal },
  { 636,   ViewModeCustom, HIDE_ITEM }
};

#define NUMBER_OF_VIEW_MODES (sizeof(viewModes) / sizeof(viewModes[0]))

/** Gets the index of a view mode
 *
 * @param viewMode The view mode
 * @return The index of the view mode in the viewModes array
 */
static int GetViewModeIndex(int viewMode)
{
  size_t i;

  // Find the current view mode
  for (i = 0; i < NUMBER_OF_VIEW_MODES; i++)
  {
    if (viewModes[i].viewMode == viewMode)
      return i;
  }

  return 0; // An invalid view mode will always return the first view mode
}

/** Gets the next view mode for quick cycling through the modes
 *
 * @param viewMode The current view mode
 * @return The next view mode
 */
int CViewModeSettings::GetNextQuickCycleViewMode(int viewMode)
{
  // Find the next quick cycle view mode
  for (size_t i = GetViewModeIndex(viewMode) + 1; i < NUMBER_OF_VIEW_MODES; i++)
  {
    if (!viewModes[i].hideFromQuickCycle)
      return viewModes[i].viewMode;
  }

  return ViewModeNormal;
}

/** Gets the string index for the view mode
 *
 * @param viewMode The current view mode
 * @return The string index
 */
int CViewModeSettings::GetViewModeStringIndex(int viewMode)
{
  return viewModes[GetViewModeIndex(viewMode)].stringIndex;
}

/** Fills the list with all visible view modes
 */
void CViewModeSettings::ViewModesFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  // Add all appropriate view modes to the list control
  for (const auto &item : viewModes)
  {
    if (!item.hideFromList)
      list.push_back(make_pair(g_localizeStrings.Get(item.stringIndex), item.viewMode));
  }
}
