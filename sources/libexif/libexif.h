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

#define MAX_IPTC_STRING 256

typedef struct {
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
} IPTCInfo_t;

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
    char  Comments[MAX_COMMENT];

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

