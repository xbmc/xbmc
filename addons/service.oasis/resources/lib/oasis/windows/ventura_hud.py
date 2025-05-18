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
from urllib.parse import urlparse

import xbmc  # pylint: disable=import-error
import xbmcgui  # pylint: disable=import-error

from oasis.utils.playlist_player import PlaylistPlayer


# TODO
ASSET_DIR: str = "/home/garrett/Videos/Ventura"


class VenturaHUD(xbmcgui.WindowXML):
    def onInit(self) -> None:
        """ Service entry-point. """
        self._play()

    @staticmethod
    def _play() -> None:
        # Get assets
        assets: List[str] = []
        for filename in os.listdir(ASSET_DIR):
            assets.append(os.path.join(ASSET_DIR, filename))

        playlist: xbmc.PlayList = xbmc.PlayList(xbmc.PLAYLIST_VIDEO)

        # Add assets to playlist
        for asset in assets:
            list_item: xbmcgui.ListItem = xbmcgui.ListItem(os.path.basename(asset))
            list_item.setInfo("video", {"Title": os.path.basename(asset)})
            playlist.add(url=asset, listitem=list_item)

        playlist.shuffle()

        PlaylistPlayer.play_playlist(playlist)
