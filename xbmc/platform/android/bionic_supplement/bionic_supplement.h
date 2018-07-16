/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int rand_r (unsigned int *seed);
ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *stream);

#ifdef __cplusplus
}
#endif /* __cplusplus */
