################################################################################
#
#  Copyright (C) 2022 Garrett Brown
#  This file is part of OASIS - https://github.com/eigendude/OASIS
#
#  SPDX-License-Identifier: Apache-2.0
#  See the file LICENSE.txt for more information.
#
################################################################################

import json
import os
from typing import Any
from typing import Dict
from urllib.parse import urlparse

import xbmc  # pylint: disable=import-error
import xbmcgui  # pylint: disable=import-error

from oasis.utils.playlist_player import PlaylistPlayer


# TODO
ASSET_DIR: str = "/home/garrett/Videos/ATV-4K-SDR"


class StationHUD(xbmcgui.WindowXML):
    def onInit(self) -> None:
        self._play()

    @staticmethod
    def _play() -> None:
        """Service entry-point."""
        assets: Dict[
            str, Dict[str, str]
        ] = {}  # Asset category -> asset label -> filename

        with open(os.path.join(ASSET_DIR, "entries.json")) as f:
            data: Dict[str, Any] = json.load(f)

            # Get categories
            categories: Dict[str, str] = {}
            for category in data["categories"]:
                category_id: str = category["id"]
                name: str = category["localizedNameKey"]
                categories[category_id] = name
                assets[name] = {}

            # Get assets
            for asset in data["assets"]:
                label: str = asset["accessibilityLabel"]
                url: str = asset["url-4K-SDR"]
                asset_category: str = categories[asset["categories"][0]]
                filename: str = os.path.basename(urlparse(url).path)
                assets[asset_category][label] = filename

        # space_category = 'AerialCategorySpace'
        # space_assets = assets[space_category]

        playlist: xbmc.PlayList = xbmc.PlayList(xbmc.PLAYLIST_VIDEO)

        for category_name, category_assets in assets.items():
            for label, filename in category_assets.items():
                asset_url: str = os.path.join(ASSET_DIR, filename)
                if not os.path.exists(asset_url):
                    continue
                list_item: xbmcgui.ListItem = xbmcgui.ListItem(label)
                list_item.setInfo("video", {"Title": label})
                playlist.add(url=asset_url, listitem=list_item)

        playlist.shuffle()

        PlaylistPlayer.play_playlist(playlist)
