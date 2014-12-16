/*
 *  ppc_drawings.h
 *  Goom
 *
 *  Created by Guillaume Borios on Sun Dec 28 2003.
 *  Copyright (c) 2003 iOS. All rights reserved.
 *
 */

/* Generic PowerPC Code */
void ppc_brightness_generic(Pixel *src, Pixel *dest, int size, int coeff);

/* G4 Specific PowerPC Code (Possible use of Altivec and Data Streams) */
void ppc_brightness_G4(Pixel *src, Pixel *dest, int size, int coeff);

/* G5 Specific PowerPC Code (Possible use of Altivec) */
void ppc_brightness_G5(Pixel *src, Pixel *dest, int size, int coeff);

