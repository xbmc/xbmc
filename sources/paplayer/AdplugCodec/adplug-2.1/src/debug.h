/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2002 Simon Peter <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * debug.h - AdPlug Debug Logger
 * Copyright (c) 2002 Riven the Mage <riven@ok.ru>
 * Copyright (c) 2002 Simon Peter <dn.tlp@gmx.net>
 *
 * NOTES:
 * This debug logger is used throughout AdPlug to log debug output to stderr
 * (the default) or to a user-specified logfile.
 *
 * To use it, AdPlug has to be compiled with debug logging support enabled.
 * This is done by defining the DEBUG macro with every source-file. The
 * LogFile() function can be used to specify a logfile to write to.
 */

#ifndef H_DEBUG
#define H_DEBUG

extern "C"
{
        void AdPlug_LogFile(const char *filename);
        void AdPlug_LogWrite(const char *fmt, ...);
}

#endif
