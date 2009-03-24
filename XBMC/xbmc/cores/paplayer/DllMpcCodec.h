#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#if (defined HAVE_CONFIG_H)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_LIBRARIES) || (defined USE_EXTERNAL_LIBMPCDEC)
  #include <mpcdec/config_types.h>
  #include <mpcdec/reader.h>
  #include <mpcdec/streaminfo.h>
#else
  #include "MPCCodec/include/mpcdec/config_types.h"
  #include "MPCCodec/include/mpcdec/reader.h"
  #include "MPCCodec/include/mpcdec/streaminfo.h"
#endif
#include "MPCCodec/src/xbmc_interface.h"
#include "DynamicDll.h"

// stuff from dll we need
#define FRAMELEN 1152

class DllMPCCodecInterface
{
public:
    virtual ~DllMPCCodecInterface() {}
    virtual bool Open(mpc_decoder **decoder, mpc_reader *reader, mpc_streaminfo *info, double *timeinseconds)=0;
    virtual void Close(mpc_decoder *decoder)=0;
    virtual int Read(mpc_decoder *decoder, float *buffer, int size)=0;
    virtual int Seek(mpc_decoder *decoder, double timeinseconds)=0;
};

#if (defined USE_EXTERNAL_LIBRARIES) || (defined USE_EXTERNAL_LIBVORBIS)

class DllMPCCodec : public DllDynamic, DllMPCCodecInterface
{
public:
    virtual ~DllMPCCodec() {};
    virtual bool Open(mpc_decoder **decoder, mpc_reader *reader,
        mpc_streaminfo *info, double *timeinseconds)
        { return ::Open(decoder, reader, info, timeinseconds); }
    virtual void Close(mpc_decoder *decoder)
        { return ::Close(decoder); }
    virtual int Read(mpc_decoder *decoder, float *buffer, int size)
        { return ::Read(decoder, buffer, size); }
    virtual int Seek(mpc_decoder *decoder, double timeinseconds)
        { return ::Seek(decoder, timeinseconds); }

    // DLL faking.
    virtual bool ResolveExports() { return true; }
    virtual bool Load() { return true; }
    virtual void Unload() {}
};

#else

class DllMPCCodec : public DllDynamic, DllMPCCodecInterface
{
  DECLARE_DLL_WRAPPER(DllMPCCodec, DLL_PATH_MPC_CODEC)
  DEFINE_METHOD4(bool, Open, (mpc_decoder **p1, mpc_reader *p2, mpc_streaminfo *p3, double *p4))
  DEFINE_METHOD1(void, Close, (mpc_decoder *p1))
  DEFINE_METHOD3(int, Read, (mpc_decoder *p1, float *p2, int p3))
  DEFINE_METHOD2(int, Seek, (mpc_decoder *p1, double p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(Open)
    RESOLVE_METHOD(Close)
    RESOLVE_METHOD(Read)
    RESOLVE_METHOD(Seek)
  END_METHOD_RESOLVE()
};

#endif
