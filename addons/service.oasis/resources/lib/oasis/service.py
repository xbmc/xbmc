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
import xbmcgui  # pylint: disable=import-error

from oasis.windows.camera_view import CameraView
from oasis.windows.fireworks_hud import FireworksHUD
from oasis.windows.station_hud import StationHUD


class OasisService:
    @staticmethod
    def run(hostname: str) -> None:
        addon: xbmcaddon.Addon = xbmcaddon.Addon()
        addon_path: str = addon.getAddonInfo("path")

        xbmc.log(f"Running OASIS service on {hostname}", level=xbmc.LOGDEBUG)

        window: xbmcgui.WindowXML

        # TODO: Hardware configuration
        if hostname == "nuc":
            window = StationHUD("StationHUD.xml", addon_path, "default", "1080i", False)
        elif hostname == "asus":
            window = CameraView("CameraView1.xml", addon_path, "default", "1080i", False)
        elif hostname == "lenovo":
            window = CameraView("CameraViewVertical2.xml", addon_path, "default", "1080i", False)
        elif hostname == "starship":
            window = CameraView("CameraView16.xml", addon_path, "default", "1080i", False)
        elif hostname == "zotac":
            window = FireworksHUD("FireworksHUD.xml", addon_path, "default", "1080i", False)
        else:
            window = CameraView("CameraView4.xml", addon_path, "default", "1080i", False)

        window.doModal()
        xbmc.sleep(100)
        del window

        xbmc.log("Exiting OASIS service", level=xbmc.LOGDEBUG)
