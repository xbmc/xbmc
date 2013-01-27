/*
 * Copyright (C) 2001, 2002 Billy Biggs <vektor@dumbterm.net>,
 *                          Håkan Hjort <d95hjort@dtek.chalmers.se>
 *
 * This file is part of libdvdread.
 *
 * libdvdread is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdread is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdread; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LIBDVDREAD_NAV_PRINT_H
#define LIBDVDREAD_NAV_PRINT_H

#include "nav_types.h"

/**
 * Pretty printing of the NAV packets, PCI and DSI structs.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prints information contained in the PCI to stdout.
 *
 * @param pci Pointer to the PCI data structure to be printed.
 */
void navPrint_PCI(pci_t *);

/**
 * Prints information contained in the DSI to stdout.
 *
 * @param dsi Pointer to the DSI data structure to be printed.
 */
void navPrint_DSI(dsi_t *);

#ifdef __cplusplus
};
#endif
#endif /* LIBDVDREAD_NAV_PRINT_H */
