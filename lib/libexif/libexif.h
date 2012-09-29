#ifndef HAS_LIBEXIF_H
#define HAS_LIBEXIF_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DLL
#ifdef WIN32
#define EXIF_EXPORT __declspec(dllexport)
#else
#define EXIF_EXPORT
#endif
#else
#define EXIF_EXPORT
#endif

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

#define MAX_IPTC_STRING 256

typedef struct {
  char RecordVersion[MAX_IPTC_STRING];
  char SupplementalCategories[MAX_IPTC_STRING];
  char Keywords[MAX_IPTC_STRING];
  char Caption[MAX_IPTC_STRING];
  char Author[MAX_IPTC_STRING];
  char Headline[MAX_IPTC_STRING];
  char SpecialInstructions[MAX_IPTC_STRING];
  char Category[MAX_IPTC_STRING];
  char Byline[MAX_IPTC_STRING];
  char BylineTitle[MAX_IPTC_STRING];
  char Credit[MAX_IPTC_STRING];
  char Source[MAX_IPTC_STRING];
  char CopyrightNotice[MAX_IPTC_STRING];
  char ObjectName[MAX_IPTC_STRING];
  char City[MAX_IPTC_STRING];
  char State[MAX_IPTC_STRING];
  char Country[MAX_IPTC_STRING];
  char TransmissionReference[MAX_IPTC_STRING];
  char Date[MAX_IPTC_STRING];
  char Copyright[MAX_IPTC_STRING];
  char ReferenceService[MAX_IPTC_STRING];
  char CountryCode[MAX_IPTC_STRING];
  char TimeCreated[MAX_IPTC_STRING];
  char SubLocation[MAX_IPTC_STRING];
  char ImageType[MAX_IPTC_STRING];
} IPTCInfo_t;

#define EXIF_COMMENT_CHARSET_CONVERTED -1 // Comments contains converted data
#define EXIF_COMMENT_CHARSET_UNKNOWN    0 // Exif: Unknown
#define EXIF_COMMENT_CHARSET_ASCII      2 // Exif: Ascii
#define EXIF_COMMENT_CHARSET_UNICODE    3 // Exif: Unicode (UTF-16)
#define EXIF_COMMENT_CHARSET_JIS        4 // Exif: JIS X208-1990

#define MAX_COMMENT 2000
#define MAX_DATE_COPIES 10

typedef struct {
    char  CameraMake   [32];
    char  CameraModel  [40];
    char  DateTime     [20];
    int   Height, Width;
    int   Orientation;
    int   IsColor;
    int   Process;
    int   FlashUsed;
    float FocalLength;
    float ExposureTime;
    float ApertureFNumber;
    float Distance;
    float CCDWidth;
    float ExposureBias;
    float DigitalZoomRatio;
    int   FocalLength35mmEquiv; // Exif 2.2 tag - usually not present.
    int   Whitebalance;
    int   MeteringMode;
    int   ExposureProgram;
    int   ExposureMode;
    int   ISOequivalent;
    int   LightSource;
    int   CommentsCharset;             // EXIF_COMMENT_CHARSET_*
    char  Comments[MAX_COMMENT];
    char  Description[MAX_COMMENT];

    unsigned ThumbnailOffset;          // Exif offset to thumbnail
    unsigned ThumbnailSize;            // Size of thumbnail.
    unsigned LargestExifOffset;        // Last exif data referenced (to check if thumbnail is at end)

    char  ThumbnailAtEnd;              // Exif header ends with the thumbnail
                                       // (we can only modify the thumbnail if its at the end)
    int   ThumbnailSizeOffset;

    int  DateTimeOffsets[MAX_DATE_COPIES];
    int  numDateTimeTags;

    int GpsInfoPresent;
    char GpsLat[31];
    char GpsLong[31];
    char GpsAlt[20];
} ExifInfo_t;

EXIF_EXPORT bool process_jpeg(const char *filename, ExifInfo_t *exifInfo, IPTCInfo_t *iptcInfo);

#ifdef __cplusplus
}
#endif

#endif

