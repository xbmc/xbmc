/*
** dec_if.h - common decoder interface for nsvplay/nsvplayx
**
** Copyright (C) 2001-2003 Nullsoft, Inc.
**
** This software is provided 'as-is', without any express or implied warranty.
** In no event will the authors be held liable for any damages arising from the use of this software.
**
** Permission is granted to anyone to use this software for any purpose, including commercial
** applications, and to alter it and redistribute it freely, subject to the following restrictions:
**  1. The origin of this software must not be misrepresented; you must not claim that you wrote the
**     original software. If you use this software in a product, an acknowledgment in the product
**     documentation would be appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
**     being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
*/

#ifndef _NSV_DEC_IF_H_
#define _NSV_DEC_IF_H_


typedef  struct {
  unsigned char*  baseAddr;
  long      rowBytes;
} YV12_PLANE;

typedef  struct {
  YV12_PLANE  y;
  YV12_PLANE  u;
  YV12_PLANE  v;
} YV12_PLANES;

class IVideoDecoder
{
  public:
    virtual ~IVideoDecoder() { }

    // decode returns 0 on success
    // but *out can be NULL, meaning no new frame is available,
    // but all is well
    virtual int decode(int need_kf,
            void *in, int in_len,
            void **out, // out is set to a pointer to data
            unsigned int *out_type, // 'Y','V','1','2' is currently defined
            int *is_kf)=0;
    virtual void flush()=0;
};

class IAudioDecoder
{
  public:
    virtual ~IAudioDecoder() { }

    // returns -1 on error, 0 on success (done with data in 'in'), 1 on success
    // but to pass 'in' again next time around.
    virtual int decode(void *in, int in_len,
                       void *out, int *out_len, // out_len is read and written to
                       unsigned int out_fmt[8])=0; // out_fmt is written to
                                                   // ex: 'PCM ', srate, nch, bps
                                                   // or 'NONE' :)
    virtual void flush()=0;
};

class IAudioOutput
{
  public:
    virtual ~IAudioOutput() { }
    virtual int canwrite()=0; // returns bytes writeable
    virtual void write(void *buf, int len)=0;
    virtual unsigned int getpos()=0;
    virtual void flush(unsigned int newtime)=0;
    virtual int isplaying(void) { return 1; }
    virtual void pause(int pause) { }
    virtual void setvolume(int volume) { }
    virtual void setpan(int pan) { }
    virtual void getdescstr(char *buf) { *buf=0; }
};


/*
** The DLL must export one of these symbols (unmangled, i.e. using extern "C).
**
** IAudioDecoder *CreateAudioDecoder(unsigned int type, IAudioOutput **output);
** IVideoDecoder *CreateVideoDecoder(int w, int h, double framerate, unsigned int type, int *flip);
**
** the functions should return NULL if the conversion is not possible (or
** is not the correct format)
**
** The DLL must be in <program files>\common files\nsv, and must be named
** nsvdec_*.dll.
**
*/

#endif//_NSV_DEC_IF_H_
