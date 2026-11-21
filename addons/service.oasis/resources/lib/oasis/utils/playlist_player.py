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
from typing import Optional

import xbmc  # pylint: disable=import-error


class _ViewModePlayer(xbmc.Player):
    def onAVStarted(self) -> None:  # pylint: disable=invalid-name
        self._set_view_mode_zoom()

    def _set_view_mode_zoom(self) -> None:
        # We need a small sleep, otherwise the JSON-RPC call fails to set the
        # view mode. 1ms is enough on fast computers, but slower computers require
        # a larger delay, around 10ms. In any case, 100ms should be sufficient.
        xbmc.sleep(100)

        xbmc.executeJSONRPC(
            json.dumps(
                {
                    "jsonrpc": "2.0",
                    "method": "Player.SetViewMode",
                    "params": {"viewmode": "zoom"},
                    "id": 1,
                }
            )
        )


class PlaylistPlayer:
    _player: Optional[_ViewModePlayer] = None

    @classmethod
    def play_playlist(cls, playlist: xbmc.PlayList) -> None:
        cls._player = _ViewModePlayer()
        cls._player.play(playlist, windowed=True)
        xbmc.executebuiltin("PlayerControl(RepeatAll)")
