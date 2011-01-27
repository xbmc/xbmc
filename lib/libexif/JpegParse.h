#pragma once

#include "IptcParse.h"
#include "ExifParse.h"
#include "stdio.h"

//--------------------------------------------------------------------------
// JPEG markers consist of one or more 0xFF bytes, followed by a marker
// code byte (which is not an FF).  Here are the marker codes of interest
// in this application.
//--------------------------------------------------------------------------

#define M_SOF0  0xC0            // Start Of Frame N
#define M_SOF1  0xC1            // N indicates which compression process
#define M_SOF2  0xC2            // Only SOF0-SOF2 are now in common use
#define M_SOF3  0xC3
#define M_SOF5  0xC5            // NB: codes C4 and CC are NOT SOF markers
#define M_SOF6  0xC6
#define M_SOF7  0xC7
#define M_SOF9  0xC9
#define M_SOF10 0xCA
#define M_SOF11 0xCB
#define M_SOF13 0xCD
#define M_SOF14 0xCE
#define M_SOF15 0xCF
#define M_SOI   0xD8            // Start Of Image (beginning of datastream)
#define M_EOI   0xD9            // End Of Image (end of datastream)
#define M_SOS   0xDA            // Start Of Scan (begins compressed data)
#define M_JFIF  0xE0            // Jfif marker
#define M_EXIF  0xE1            // Exif marker
#define M_COM   0xFE            // COMment
#define M_DQT   0xDB
#define M_DHT   0xC4
#define M_DRI   0xDD
#define M_IPTC  0xED            // IPTC marker


class CJpegParse
{
  public:
    CJpegParse   ();
   ~CJpegParse   (void)  {}
    bool         Process (const char *picFileName);
    const ExifInfo_t * GetExifInfo() const { return &m_ExifInfo; };
    const IPTCInfo_t * GetIptcInfo() const { return &m_IPTCInfo; };

  private:
    bool ExtractInfo    (FILE *infile);
    bool GetSection     (FILE *infile, const unsigned short sectionLength);
    void ReleaseSection (void);
    void ProcessSOFn    (void);

    unsigned char*  m_SectionBuffer;
    ExifInfo_t      m_ExifInfo;
    IPTCInfo_t      m_IPTCInfo;
};

