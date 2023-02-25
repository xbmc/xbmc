################################################################################
#
#  Copyright (C) 2023 Garrett Brown
#  This file is part of OASIS - https://github.com/eigendude/OASIS
#
#  SPDX-License-Identifier: Apache-2.0
#  See the file LICENSE.txt for more information.
#
################################################################################

import os
from typing import List

import xbmc  # pylint: disable=import-error
import xbmcgui  # pylint: disable=import-error

from oasis.utils.playlist_player import PlaylistPlayer


# Get the current user's home directory and set the asset directory
HOME_DIR = os.path.expanduser("~")
ASSET_DIR = os.path.join(HOME_DIR, "Videos", "Ventura")


class VenturaHUD(xbmcgui.WindowXML):
    def onInit(self) -> None:
        """Service entry-point."""
        self._play()

    @staticmethod
    def _play() -> None:
        # Check if the asset directory exists
        if not os.path.exists(ASSET_DIR):
            xbmc.log(
                f"Asset directory does not exist: {ASSET_DIR}", level=xbmc.LOGERROR
            )
            return

        # Get assets
        assets: List[str] = []
        for filename in os.listdir(ASSET_DIR):
            assets.append(os.path.join(ASSET_DIR, filename))

        playlist: xbmc.PlayList = xbmc.PlayList(xbmc.PLAYLIST_VIDEO)

        # Add assets to playlist
        for asset in assets:
            list_item: xbmcgui.ListItem = xbmcgui.ListItem(os.path.basename(asset))
            video_info_tag = list_item.getVideoInfoTag()
            video_info_tag.setTitle(os.path.basename(asset))
            playlist.add(url=asset, listitem=list_item)

        playlist.shuffle()

        PlaylistPlayer.play_playlist(playlist)
