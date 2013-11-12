#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

/*!
 \ingroup settings
 \brief Interface defining methods being called by the settings system if an
 action is performed on multiple/all settings
 */
class ISettingsHandler
{
public:
  virtual ~ISettingsHandler() { }

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
