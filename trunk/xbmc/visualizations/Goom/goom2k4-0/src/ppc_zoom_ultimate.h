/*
 *  ppc_zoom_ultimate.h
 *  Goom
 *
 *  Created by Guillaume Borios on Sun Dec 28 2003.
 *  Copyright (c) 2003 iOS. All rights reserved.
 *
 */

/* Generic PowerPC Code */
void ppc_zoom_generic (int sizeX, int sizeY, Pixel *src, Pixel *dest, int *brutS, int *brutD, int buffratio, int precalCoef[16][16]);

/* G4 Specific PowerPC Code (Possible use of Altivec and Data Streams) */
void ppc_zoom_G4 (int sizeX, int sizeY, Pixel *src, Pixel *dest, int *brutS, int *brutD, int buffratio, int precalCoef[16][16]);