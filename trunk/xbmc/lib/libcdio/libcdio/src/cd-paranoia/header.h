/******************************************************************
 * CopyPolicy: GNU Public License 2 applies
 * Copyright (C) 1998 Monty xiphmont@mit.edu
 ******************************************************************/

/** \file header.h 
 *  \brief header for WAV, AIFF and AIFC header-writing routines.
 */

/** Writes WAV headers */
extern void WriteWav(int f,long int i_bytes);

/** Writes AIFC headers */
extern void WriteAifc(int f,long int i_bytes);

/** Writes AIFF headers */
extern void WriteAiff(int f,long int_bytes);
