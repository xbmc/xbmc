/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
 \ingroup settings
 \brief Interface defining methods being called by the settings system if an
 action is performed on multiple/all settings
 */
class ISettingsHandler
{
public:
  virtual ~ISettingsHandler() = default;

  /*!
   \brief Settings loading has been initiated.

   \return True if the settings should be loaded, false if the loading should be aborted.
   */
  virtual bool OnSettingsLoading() { return true; }
  /*!
   \brief Settings have been loaded.

   This callback can be used to trigger loading other settings.
   */
  virtual void OnSettingsLoaded() { }
  /*!
   \brief Settings saving has been initiated.

   \return True if the settings should be saved, false if the saving should be aborted.
   */
  virtual bool OnSettingsSaving() const { return true; }
  /*!
   \brief Settings have been saved.

   This callback can be used to trigger saving other settings.
   */
  virtual void OnSettingsSaved() const { }
  /*!
   \brief Setting values have been unloaded.

   This callback can be used to trigger uninitializing any state variables
   (e.g. before re-loading the settings).
   */
  virtual void OnSettingsUnloaded() { }
  /*!
   \brief Settings have been cleared.

   This callback can be used to trigger clearing any state variables.
   */
  virtual void OnSettingsCleared() { }
};
