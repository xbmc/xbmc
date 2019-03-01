#pragma once

#include "libexif.h"

class CExifParse
{
  public:
   ~CExifParse(void) = default;
    bool Process(const unsigned char* const Data, const unsigned short length, ExifInfo_t *info);
    static int Get16(const void* const Short, const bool motorolaOrder=true);
    static int Get32(const void* const Long,  const bool motorolaOrder=true);

  private:
    ExifInfo_t *m_ExifInfo = nullptr;
    double m_FocalPlaneXRes = 0.0;
    double m_FocalPlaneUnits = 0.0;
    unsigned m_LargestExifOffset = 0;          // Last exif data referenced (to check if thumbnail is at end)
    int m_ExifImageWidth = 0;
    bool m_MotorolaOrder = false;
    bool m_DateFound = false;

//    void    LocaliseDate        (void);
//    void    GetExposureTime     (const float exposureTime);
    double ConvertAnyFormat(const void* const ValuePtr, int Format);
    void ProcessDir(const unsigned char* const DirStart,
                    const unsigned char* const OffsetBase,
                    const unsigned ExifLength, int NestingLevel);
    void ProcessGpsInfo(const unsigned char* const DirStart,
                        int ByteCountUnused,
                        const unsigned char* const OffsetBase,
                        unsigned ExifLength);
    void GetLatLong(const unsigned int Format,
                    const unsigned char* ValuePtr,
                    const int ComponentSize,
                    char *latlongString);
};

