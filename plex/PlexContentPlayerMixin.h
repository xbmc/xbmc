/*
 *  Copyright (C) 2011 Plex, Inc.
 *      Author: Elan Feingold
 */

#pragma once

#include <boost/lexical_cast.hpp>

#include "Application.h"
#include "FileItem.h"
#include "GUISettings.h"
#include "guilib/GUIBaseContainer.h"

class PlexContentPlayerMixin
{
protected:
  /// Play the item selected in the specified container.
  void PlayFileFromContainer(const CGUIControl* control);

public:
  static CFileItemPtr GetNextUnwatched(const std::string& container);
  static void PlayMusicPlaylist(const CFileItemPtr& file);
  static void PlayPlexItem(const CFileItemPtr file, CGUIBaseContainer* container = NULL);
  static bool ProcessResumeChoice(CFileItem* file);
  static bool ProcessMediaChoice(CFileItem* file);
};
