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

import xbmc  # pylint: disable=import-error


class PlaylistPlayer:
    @staticmethod
    def play_playlist(playlist: xbmc.PlayList) -> None:
        xbmc.Player().play(playlist, windowed=True)
        xbmc.executebuiltin("PlayerControl(RepeatAll)")

        # We need a small sleep, otherwise the JSON-RPC call fails to set the
        # view mode. 1ms is enough on fast computers, but slower computers
        # require a larger delay, around 10ms. In any case, 100ms should be
        # sufficient.
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
