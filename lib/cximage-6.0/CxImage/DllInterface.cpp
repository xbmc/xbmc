#ifdef _DLL
#include "ximage.h"
#include "ximajpg.h"

#if defined(_LINUX) || defined(__APPLE__)
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#define strcmpi strcasecmp
#else //win32
#include <sys/types.h>
#include <sys/stat.h>
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define strcmpi _strcmpi
#endif

#define RESAMPLE_QUALITY 0
#undef USE_EXIF_THUMBS

extern "C" struct ImageInfo
{
  unsigned int width;
  unsigned int height;
  unsigned int originalwidth;
  unsigned int originalheight;
  EXIFINFO exifInfo;
  BYTE *texture;
  void *context;
  BYTE *alpha;
};

// helper functions

// determines based on file extension the type of file
DWORD GetImageType(const char *file)
{ 
  if ( !file || ( 0 == *file ) ) // ensure file is not NULL and has non-zero length
    return CXIMAGE_FORMAT_UNKNOWN;

  // try to determine extension using '.' or use entire filename
  // if '.' is absent.
  const char *ext = strrchr(file, '.');
  if (ext == NULL)
    ext = (char*)file;
  else
    ext++;

  if ( 0 == *ext ) // if filename ends in '.', we can't identify based on extension
    return CXIMAGE_FORMAT_UNKNOWN;

  if ( 0 == strcmpi(ext, "bmp") ) return CXIMAGE_FORMAT_BMP;
  else if ( 0 == strcmpi(ext, "bitmap") ) return CXIMAGE_FORMAT_BMP;
  else if ( 0 == strcmpi(ext, "gif") )    return CXIMAGE_FORMAT_GIF;
  else if ( 0 == strcmpi(ext, "jpg") )    return CXIMAGE_FORMAT_JPG;
  else if ( 0 == strcmpi(ext, "tbn") )    return CXIMAGE_FORMAT_JPG;
  else if ( 0 == strcmpi(ext, "jpeg") )   return CXIMAGE_FORMAT_JPG;
  else if ( 0 == strcmpi(ext, "png") )    return CXIMAGE_FORMAT_PNG;
  else if ( 0 == strcmpi(ext, "ico") )    return CXIMAGE_FORMAT_ICO;
  else if ( 0 == strcmpi(ext, "tif") )    return CXIMAGE_FORMAT_TIF;
  else if ( 0 == strcmpi(ext, "tiff") )   return CXIMAGE_FORMAT_TIF;
  else if ( 0 == strcmpi(ext, "tga") )    return CXIMAGE_FORMAT_TGA;
  else if ( 0 == strcmpi(ext, "pcx") )    return CXIMAGE_FORMAT_PCX;

  // RAW camera formats
  else if ( 0 == strcmpi(ext, "cr2") ) return CXIMAGE_FORMAT_RAW;  
  else if ( 0 == strcmpi(ext, "nef") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "dng") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "crw") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "orf") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "arw") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "erf") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "3fr") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "dcr") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "x3f") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "mef") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "raf") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "mrw") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "pef") ) return CXIMAGE_FORMAT_RAW;
  else if ( 0 == strcmpi(ext, "sr2") ) return CXIMAGE_FORMAT_RAW;

  // fallback to unknown
  return CXIMAGE_FORMAT_UNKNOWN;
}

int DetectFileType(const BYTE* pBuffer, int nBufSize)
{
  if (nBufSize <= 5)
    return CXIMAGE_FORMAT_UNKNOWN;
  if (pBuffer[1] == 'P' && pBuffer[2] == 'N' && pBuffer[3] == 'G')
    return CXIMAGE_FORMAT_PNG;
  if (pBuffer[0] == 'B' && pBuffer[1] == 'M')
    return CXIMAGE_FORMAT_BMP;
  if (pBuffer[0] == 0xFF && pBuffer[1] == 0xD8 && pBuffer[2] == 0xFF)
    // don't include the last APP0 byte (0xE0), as some (non-conforming) JPEG/JFIF files might have some other
    // APPn-specific data here, and we should skip over this.
    return CXIMAGE_FORMAT_JPG;
  return CXIMAGE_FORMAT_UNKNOWN;
}

int ResampleKeepAspect(CxImage &image, unsigned int width, unsigned int height)
{
  bool bResize = false;
  float fAspect = ((float)image.GetWidth()) / ((float)image.GetHeight());
  unsigned int newwidth = image.GetWidth();
  unsigned int newheight = image.GetHeight();
  if (newwidth > width)
  {
    bResize = true;
    newwidth = width;
    newheight = (DWORD)( ( (float)newwidth) / fAspect);
  }
  if (newheight > height)
  {
    bResize = true;
    newheight = height;
    newwidth = (DWORD)( fAspect * ( (float)newheight) );
  }
  if (bResize)
  {
    if (!image.Resample(newwidth, newheight, RESAMPLE_QUALITY) || !image.IsValid())
    {
      printf("PICTURE::SaveThumb: Unable to resample picture: Error:%s\n", image.GetLastError());
      return -1;
    }
  }
  return bResize ? 1 : 0;
}

#ifdef LoadImage
#undef LoadImage
#endif

#if defined(_LINUX) || defined(__APPLE__)
#define __declspec(x) 
#endif

extern "C"
{
  bool IsDir(const char* file)
  {
    struct stat holder;
    if (stat(file, &holder) == -1)
      return false;
    if (S_ISDIR(holder.st_mode))
      return true;

    return false;
  }

  __declspec(dllexport) bool ReleaseImage(ImageInfo *info) 
  {
    if (info && info->context)
    {
      delete ((CxImage *)info->context);
      return true;
    }

	  return false;
  }

  __declspec(dllexport) bool LoadImage(const char *file, unsigned int maxwidth, unsigned int maxheight, ImageInfo *info)
  {
    if (!file || !info) return false;

	if (IsDir(file))
		return false;

	  // load the image
    DWORD dwImageType = GetImageType(file);
    CxImage *image = new CxImage(dwImageType);
    if (!image) return false;

    int actualwidth = maxwidth;
    int actualheight = maxheight;
    try
    {
      if (!image->Load(file, dwImageType, actualwidth, actualheight) || !image->IsValid())
      {
#if !defined(_LINUX) && !defined(__APPLE__)
	    int nErr = GetLastError();
#else
	    int nErr = errno;
#endif
        printf("PICTURE::LoadImage: Unable to open image: %s Error:%s (%d)\n", file, image->GetLastError(),nErr);
        delete image;
        return false;
      }
    }
    catch (...)
    {
      printf("PICTURE::LoadImage: Unable to open image: %s\n", file);
      delete image;
      return false;
    }
    // ok, now resample the image down if necessary
    if (ResampleKeepAspect(*image, maxwidth, maxheight) < 0)
    {
      printf("PICTURE::LoadImage: Unable to resample picture: %s\n", file);
      delete image;
      return false;
    }

    // make sure our image is 24bit minimum
    image->IncreaseBpp(24);

    // fill in our struct
    info->width = image->GetWidth();
    info->height = image->GetHeight();
    info->originalwidth = actualwidth;
    info->originalheight = actualheight;
    memcpy(&info->exifInfo, image->GetExifInfo(), sizeof(EXIFINFO));

    // create our texture
    info->context = image;
    info->texture = image->GetBits();
    info->alpha = image->AlphaGetBits();
    return (info->texture != NULL);
  };

  __declspec(dllexport) bool LoadImageFromMemory(const BYTE *buffer, unsigned int size, const char *mime, unsigned int maxwidth, unsigned int maxheight, ImageInfo *info)
  {
    if (!buffer || !size || !mime || !info) return false;

    // load the image
    DWORD dwImageType = CXIMAGE_FORMAT_UNKNOWN;
    if (strlen(mime))
      dwImageType = GetImageType(mime);
    if (dwImageType == CXIMAGE_FORMAT_UNKNOWN)
      dwImageType = DetectFileType(buffer, size);
    if (dwImageType == CXIMAGE_FORMAT_UNKNOWN)
    {
      printf("PICTURE::LoadImageFromMemory: Unable to determine image type.");
      return false;
    }

    CxImage *image = new CxImage(dwImageType);
    if (!image)
      return false;

    int actualwidth = maxwidth;
    int actualheight = maxheight;

    try
    {
      bool success = image->Decode((BYTE*)buffer, size, dwImageType, actualwidth, actualheight);
      if (!success && dwImageType != CXIMAGE_FORMAT_UNKNOWN)
      { // try to decode with unknown imagetype
        success = image->Decode((BYTE*)buffer, size, CXIMAGE_FORMAT_UNKNOWN);
      }
      if (!success || !image->IsValid())
      {
        printf("PICTURE::LoadImageFromMemory: Unable to decode image. Error:%s\n", image->GetLastError());
        delete image;
        return false;
      }
    }
    catch (...)
    {
      printf("PICTURE::LoadImageFromMemory: Unable to decode image.");
      delete image;
      return false;
    }

    // ok, now resample the image down if necessary
    if (ResampleKeepAspect(*image, maxwidth, maxheight) < 0)
    {
      printf("PICTURE::LoadImage: Unable to resample picture\n");
      delete image;
      return false;
    }

    // make sure our image is 24bit minimum
    image->IncreaseBpp(24);

    // fill in our struct
    info->width = image->GetWidth();
    info->height = image->GetHeight();
    info->originalwidth = actualwidth;
    info->originalheight = actualheight;
    memcpy(&info->exifInfo, image->GetExifInfo(), sizeof(EXIFINFO));

    // create our texture
    info->context = image;
    info->texture = image->GetBits();
    info->alpha = image->AlphaGetBits();
    return (info->texture != NULL);
  };

  __declspec(dllexport) bool CreateThumbnailFromSurface(BYTE *buffer, unsigned int width, unsigned int height, unsigned int stride, const char *thumb)
  {
    if (!buffer || !thumb) return false;
    // creates an image, and copies the surface data into it.
    CxImage image(width, height, 24, CXIMAGE_FORMAT_PNG);
    if (!image.IsValid()) return false;
    image.AlphaCreate();
    if (!image.AlphaIsValid()) return false;
    bool fullyTransparent(true);
    bool fullyOpaque(true);
    for (unsigned int y = 0; y < height; y++)
    {
      BYTE *ptr = buffer + (y * stride);
      for (unsigned int x = 0; x < width; x++)
      {
        BYTE b = *ptr++;
        BYTE g = *ptr++;
        BYTE r = *ptr++;
        BYTE a = *ptr++;  // alpha
        if (a)
          fullyTransparent = false;
        if (a != 0xff)
          fullyOpaque = false;
        image.SetPixelColor(x, height - 1 - y, RGB(r, g, b));
        image.AlphaSet(x, height - 1 - y, a);
      }
    }
    if (fullyTransparent || fullyOpaque)
      image.AlphaDelete();
    image.SetJpegQuality(90);

    DWORD type;
    if (image.AlphaIsValid() || GetImageType(thumb) == CXIMAGE_FORMAT_PNG)
      type = CXIMAGE_FORMAT_PNG;
    else
      type = CXIMAGE_FORMAT_JPG;

    if (!image.Save(thumb, type))
    {
      printf("PICTURE::CreateThumbnailFromSurface: Unable to save thumb to %s", thumb);
      return false;
    }
    return true;
  };
}

#endif
