#pragma once

/*
 *      Copyright (C) 2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "DynamicDll.h"
#include "utils/log.h"
extern "C" {
#undef strcasecmp
#include "libapetag/file_io.h"
#include "libapetag/apetaglib.h"
}

class DllLibApeTagInterface
{
public:
    virtual ~DllLibApeTagInterface() {}
    virtual int apetag_read (apetag *mem_cnt, char *filename, int flag)=0;
    virtual int apetag_read_fp (apetag *mem_cnt, ape_file * fp, char *filename, int flag)=0;
    virtual apetag *apetag_init (void)=0;
    virtual void apetag_free (apetag *mem_cnt)=0;
    virtual int apetag_save (char *filename, apetag *mem_cnt, int flag)=0;
    virtual int apefrm_add (apetag *mem_cnt, unsigned long flags, char *name, char *value)=0;
    virtual int apefrm_add_bin (apetag *mem_cnt, unsigned long flags,
                    long sizeName, char *name, long sizeValue, char *value)=0;
    virtual int apefrm_add_noreplace (apetag *mem_cnt, unsigned long flags, char *name, char *value)=0;
    virtual struct tag *apefrm_get (apetag *mem_cnt, char *name)=0;
    virtual char *apefrm_getstr (apetag *mem_cnt, char *name)=0;
    virtual void apefrm_remove_real (apetag *mem_cnt, char *name)=0;
    virtual void apefrm_remove (apetag *mem_cnt, char *name)=0;
    virtual int readtag_id3v1_fp (apetag *mem_cnt, ape_file * fp)=0;
    virtual void libapetag_print_mem_cnt (apetag *mem_cnt)=0;
};

class DllLibApeTag : public DllDynamic, DllLibApeTagInterface
{
public:
    virtual ~DllLibApeTag() {}
    virtual int apetag_read (apetag *mem_cnt, char *filename, int flag)
        { return ::apetag_read(mem_cnt, filename, flag); }
    virtual int apetag_read_fp (apetag *mem_cnt, ape_file * fp, char *filename, int flag)
        { return ::apetag_read_fp (mem_cnt, fp, filename, flag); }
    virtual apetag *apetag_init (void)
        { return ::apetag_init (); }
    virtual void apetag_free (apetag *mem_cnt)
        { return ::apetag_free (mem_cnt); }
    virtual int apetag_save (char *filename, apetag *mem_cnt, int flag)
        { return ::apetag_save (filename, mem_cnt, flag); }
    virtual int apefrm_add (apetag *mem_cnt, unsigned long flags, char *name, char *value)
        { return ::apefrm_add (mem_cnt, flags, name, value); }
    virtual int apefrm_add_bin (apetag *mem_cnt, unsigned long flags,
                    long sizeName, char *name, long sizeValue, char *value)
        { return ::apefrm_add_bin (mem_cnt, flags, sizeName, name, sizeValue, value); }
    virtual int apefrm_add_noreplace (apetag *mem_cnt, unsigned long flags, char *name, char *value)
        { return ::apefrm_add_noreplace (mem_cnt, flags, name, value); }
    virtual struct tag *apefrm_get (apetag *mem_cnt, char *name)
        { return ::apefrm_get (mem_cnt, name); }
    virtual char *apefrm_getstr (apetag *mem_cnt, char *name)
        { return ::apefrm_getstr (mem_cnt, name); }
    virtual void apefrm_remove_real (apetag *mem_cnt, char *name)
        { return ::apefrm_remove_real (mem_cnt, name); }
    virtual void apefrm_remove (apetag *mem_cnt, char *name)
        { return ::apefrm_remove (mem_cnt, name); }
    virtual int readtag_id3v1_fp (apetag *mem_cnt, ape_file * fp)
        { return ::readtag_id3v1_fp (mem_cnt, fp); }
    virtual void libapetag_print_mem_cnt (apetag *mem_cnt)
        { return ::libapetag_print_mem_cnt (mem_cnt); }

    // DLL faking.
    virtual bool ResolveExports() { return true; }
    virtual bool Load() {
        CLog::Log(LOGDEBUG, "DllLibApeTag: Using libapetag library");
        return true;
    }
    virtual void Unload() {}
};
