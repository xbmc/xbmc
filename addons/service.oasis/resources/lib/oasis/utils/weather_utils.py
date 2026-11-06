################################################################################
#
#  Copyright (C) 2022 Garrett Brown
#  This file is part of OASIS - https://github.com/eigendude/OASIS
#
#  SPDX-License-Identifier: Apache-2.0
#  See the file LICENSE.txt for more information.
#
################################################################################

import xbmc  # pylint: disable=import-error
import xbmcaddon  # pylint: disable=import-error


# HD Animated Weather Icons add-on ID
WEATHER_ICONS_ADDON_ID: str = "resource.images.weathericons.hd.animated"


class WeatherUtils:
    @staticmethod
    def configure_weather_icons() -> None:
        """Configure Estuary skin to use HD animated weather icons, if available."""
        try:
            if xbmc.getCondVisibility(f"System.HasAddon({WEATHER_ICONS_ADDON_ID})"):
                xbmc.log(
                    "Configuring HD animated weather icons...", level=xbmc.LOGDEBUG
                )

                hd_addon: xbmcaddon.Addon = xbmcaddon.Addon(id=WEATHER_ICONS_ADDON_ID)

                hd_path: str = f"resource://{WEATHER_ICONS_ADDON_ID}/"
                hd_name: str = hd_addon.getAddonInfo("name")

                if not xbmc.getCondVisibility(
                    "Skin.HasSetting(WeatherOutlookIcon.multi)"
                ):
                    xbmc.log(
                        "Setting WeatherOutlookIcon.multi to true", level=xbmc.LOGDEBUG
                    )
                    xbmc.executebuiltin("Skin.SetBool(WeatherOutlookIcon.multi)")
                else:
                    xbmc.log(
                        "WeatherOutlookIcon.multi is already true", level=xbmc.LOGDEBUG
                    )

                if xbmc.getInfoLabel("Skin.String(WeatherOutlookIcon.path)") != hd_path:
                    xbmc.log(
                        f"Setting WeatherOutlookIcon.path to {hd_path}",
                        level=xbmc.LOGDEBUG,
                    )
                    xbmc.executebuiltin(
                        f"Skin.SetString(WeatherOutlookIcon.path,{hd_path})"
                    )
                else:
                    xbmc.log(
                        "WeatherOutlookIcon.path is already set correctly",
                        level=xbmc.LOGDEBUG,
                    )

                if xbmc.getInfoLabel("Skin.String(WeatherOutlookIcon.name)") != hd_name:
                    xbmc.log(
                        f"Setting WeatherOutlookIcon.name to {hd_name}",
                        level=xbmc.LOGDEBUG,
                    )
                    xbmc.executebuiltin(
                        f"Skin.SetString(WeatherOutlookIcon.name,{hd_name})"
                    )
                else:
                    xbmc.log(
                        "WeatherOutlookIcon.name is already set correctly",
                        level=xbmc.LOGDEBUG,
                    )
        except Exception as exc:  # pylint: disable=broad-except
            xbmc.log(f"Failed to configure weather icons: {exc}", level=xbmc.LOGERROR)
