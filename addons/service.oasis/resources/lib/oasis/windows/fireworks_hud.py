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
import xbmcgui  # pylint: disable=import-error

from oasis.utils.playlist_player import PlaylistPlayer


# TODO
FIREWORKS_VIDEO: str = "/home/garrett/Videos/Fireworks/Y2Mate.is - 4K Amazing Fireworks Show with Sound! 1 Hour Holiday Mood! Relaxation Time!-eZWv_Dvyz38-1080p-1658042824825.mp4"


class FireworksHUD(xbmcgui.WindowXML):
    def onInit(self) -> None:
        self._play()

    @staticmethod
    def _play() -> None:
        playlist: xbmc.PlayList = xbmc.PlayList(xbmc.PLAYLIST_VIDEO)
        playlist.add(url=FIREWORKS_VIDEO)

        PlaylistPlayer.play_playlist(playlist)
