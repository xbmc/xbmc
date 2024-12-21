#!/usr/bin/env python3
################################################################################
#
#  Copyright (C) 2022 Garrett Brown
#  This file is part of OASIS - https://github.com/eigendude/OASIS
#
#  SPDX-License-Identifier: Apache-2.0
#  See the file LICENSE.txt for more information.
#
################################################################################

import socket

from oasis.service import OasisService


# Get the hostname
HOSTNAME = socket.gethostname()


# Run the service
OasisService.run(HOSTNAME)
