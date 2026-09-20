# SPDX-License-Identifier: GPL-3.0-or-later

import sys
from urllib.parse import parse_qsl

import xbmcplugin

from lib.scraper import run_action

HANDLE = int(sys.argv[1])
PARAMS = dict(parse_qsl(sys.argv[2].lstrip('?')))


def main():
    action = PARAMS.get('action')
    if action:
        run_action(HANDLE, action, PARAMS)
    else:
        xbmcplugin.endOfDirectory(HANDLE)


if __name__ == '__main__':
    main()
