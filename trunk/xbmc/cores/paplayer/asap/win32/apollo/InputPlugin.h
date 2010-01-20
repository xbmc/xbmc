#ifndef __INPUTPLUGIN_H__
#define __INPUTPLUGIN_H__

#include <windows.h>

typedef struct
{
    // General

    char suggestedTitle[256];
    int fileSize;
    BOOL seekable;
    BOOL hasEqualizer;

    // Audio

    int playingTime;    // ms (-1 = stream)
    int bitRate;        // b/s (negative values mean average)
    int sampleRate;     // Hz
    int numChannels;    // 1 = mono, 2 = stereo
    int bitResolution;  // 8, 16, 24 or 32
    char fileTypeDescription[256];

    // The following fields are only valid if the upper
    // 16 bits of fileSize contain the value 0xfeed when 
    // passed to the GetInfo function. The lower 16
    // bits of this field are considered as the
    // version of the extension to this structure.

    // These fields are valid for extension versions >= 1

    // The plug-in must the extension version it supports here
    BOOL extensionVersion; 
    __int64 fileSize64Bits;

} TrackInfo;

// __cdecl is a Microsoft specific keyword and means
// that the C calling convention will be used for the
// member function. If you are using a compiler other
// than MS Visual C++ you may have to remove or change
// this keyword.

class CInputDecoder
{
public:
    virtual int __cdecl GetNextAudioChunk(void **buffer) =0;
    virtual int __cdecl SetPosition(int position) =0;

    virtual void __cdecl SetEqualizer(BOOL equalize) =0;
    virtual void __cdecl AdjustEqualizer(int equalizerValues[16]) =0;

    virtual void __cdecl Close() =0;
};

class CInputPlugin
{
public:
    virtual char *__cdecl GetDescription() =0;

    virtual CInputDecoder *__cdecl Open(char *filename,int audioDataOffset) =0;
    virtual BOOL __cdecl GetInfo(char *filename,int audioDataOffset,TrackInfo *trackInfo) =0;
    virtual void __cdecl AdditionalInfo(char *filename,int audioDataOffset) =0;
    virtual BOOL __cdecl IsSuitableFile(char *filename,int audioDataOffset) =0;
    virtual char *__cdecl GetExtensions() =0;

    virtual void __cdecl Config() =0;
    virtual void __cdecl About() =0;

    virtual void __cdecl Close() =0;
};

#endif//__INPUTPLUGIN_H__
