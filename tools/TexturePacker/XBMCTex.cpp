/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#ifdef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif
//#include <string>
#include <cerrno>
//#include <cstring>
#include <dirent.h>
#include <map>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#undef main

#include "guilib/XBTF.h"
#include "XBTFWriter.h"
#include "md5.h"
#include "SDL_anigif.h"
#include "cmdlineargs.h"
#include "libsquish/squish.h"

#ifdef _WIN32
#define strncasecmp strnicmp
#endif

#ifdef USE_LZO_PACKING
#ifdef _WIN32
#include "../../lib/win32/liblzo/LZO1X.H"
#else
#include <lzo/lzo1x.h>
#endif
#endif

using namespace std;

#define FLAGS_USE_LZO     1
#define FLAGS_ALLOW_YCOCG 2
#define FLAGS_USE_DXT     4

#define DIR_SEPARATOR "/"
#define DIR_SEPARATOR_CHAR '/'

int NP2( unsigned x )
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}

const char *GetFormatString(unsigned int format)
{
  switch (format)
  {
  case XB_FMT_DXT1:
    return "DXT1 ";
  case XB_FMT_DXT3:
    return "DXT3 ";
  case XB_FMT_DXT5:
    return "DXT5 ";
  case XB_FMT_DXT5_YCoCg:
    return "YCoCg";
  case XB_FMT_A8R8G8B8:
    return "ARGB ";
  case XB_FMT_A8:
    return "A8   ";
  default:
    return "?????";
  }
}

// returns true for png, bmp, tga, jpg and dds files, otherwise returns false
bool IsGraphicsFile(char *strFileName)
{
  size_t n = strlen(strFileName);
  if (n < 4)
    return false;

  if (strncasecmp(&strFileName[n-4], ".png", 4) &&
      strncasecmp(&strFileName[n-4], ".bmp", 4) &&
      strncasecmp(&strFileName[n-4], ".tga", 4) &&
      strncasecmp(&strFileName[n-4], ".gif", 4) &&
      strncasecmp(&strFileName[n-4], ".tbn", 4) &&
      strncasecmp(&strFileName[n-4], ".jpg", 4))
    return false;

  return true;
}

// returns true for png, bmp, tga, jpg and dds files, otherwise returns false
bool IsGIF(const char *strFileName)
{
  size_t n = strlen(strFileName);
  if (n < 4)
    return false;

  if (strncasecmp(&strFileName[n-4], ".gif", 4))
    return false;

  return true;
}

void CreateSkeletonHeaderImpl(CXBTF& xbtf, std::string fullPath, std::string relativePath)
{
  struct dirent* dp;
  struct stat stat_p;
  DIR *dirp = opendir(fullPath.c_str());

  if (dirp)
  {
    while ((dp = readdir(dirp)) != NULL)
    {
      if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) 
      {
        continue;
      }

      //stat to check for dir type (reiserfs fix)
      std::string fileN = fullPath + "/" + dp->d_name;
      if (stat(fileN.c_str(), &stat_p) == 0)
      {
        if (dp->d_type == DT_DIR || stat_p.st_mode & S_IFDIR)
        {
          std::string tmpPath = relativePath;
          if (tmpPath.size() > 0)
          {
            tmpPath += "/";
          }

          CreateSkeletonHeaderImpl(xbtf, fullPath + DIR_SEPARATOR + dp->d_name, tmpPath + dp->d_name);
        }
        else if (IsGraphicsFile(dp->d_name))
        {
          std::string fileName = "";
          if (relativePath.size() > 0)
          {
            fileName += relativePath;
            fileName += "/";
          }

          fileName += dp->d_name;

          CXBTFFile file;
          file.SetPath(fileName);
          xbtf.GetFiles().push_back(file);
        }
      }
    }

    closedir(dirp);
  }
  else
  {
    printf("Error opening %s (%s)\n", fullPath.c_str(), strerror(errno));
  }
}

void CreateSkeletonHeader(CXBTF& xbtf, std::string fullPath)
{
  std::string temp;
  CreateSkeletonHeaderImpl(xbtf, fullPath, temp);
}

CXBTFFrame appendContent(CXBTFWriter &writer, int width, int height, unsigned char *data, unsigned int size, unsigned int format, bool hasAlpha, unsigned int flags)
{
  CXBTFFrame frame;
#ifdef USE_LZO_PACKING
  lzo_uint packedSize = size;

  if ((flags & FLAGS_USE_LZO) == FLAGS_USE_LZO)
  {
    // grab a temporary buffer for unpacking into
    unsigned char *packed  = new unsigned char[size + size / 16 + 64 + 3]; // see simple.c in lzo
    unsigned char *working = new unsigned char[LZO1X_999_MEM_COMPRESS];
    if (packed && working)
    {
      if (lzo1x_999_compress(data, size, packed, &packedSize, working) != LZO_E_OK || packedSize > size)
      {
        // compression failed, or compressed size is bigger than uncompressed, so store as uncompressed
        packedSize = size;
        writer.AppendContent(data, size);
      }
      else
      { // success
        lzo_uint optimSize = size;
        lzo1x_optimize(packed, packedSize, data, &optimSize, NULL);
        writer.AppendContent(packed, packedSize);
      }
      delete[] working;
      delete[] packed;
    }
  }
  else
#else
  unsigned int packedSize = size;
#endif
  {
    writer.AppendContent(data, size);
  }
  frame.SetPackedSize(packedSize);
  frame.SetUnpackedSize(size);
  frame.SetWidth(width);
  frame.SetHeight(height);
  frame.SetFormat(hasAlpha ? format : format | XB_FMT_OPAQUE);
  frame.SetDuration(0);
  return frame;
}

void CompressImage(const squish::u8 *brga, int width, int height, squish::u8 *compressed, unsigned int flags, double &colorMSE, double &alphaMSE)
{
  squish::CompressImage(brga, width, height, compressed, flags | squish::kSourceBGRA);
  squish::ComputeMSE(brga, width, height, compressed, flags | squish::kSourceBGRA, colorMSE, alphaMSE);
}

bool HasAlpha(unsigned char *argb, unsigned int width, unsigned int height)
{
  unsigned char *p = argb + 3; // offset of alpha
  for (unsigned int i = 0; i < 4*width*height; i += 4)
  {
    if (p[i] != 0xff)
      return true;
  }
  return false;
}

CXBTFFrame createXBTFFrame(SDL_Surface* image, CXBTFWriter& writer, double maxMSE, unsigned int flags)
{
  // Convert to ARGB
  SDL_PixelFormat argbFormat;
  memset(&argbFormat, 0, sizeof(SDL_PixelFormat));
  argbFormat.BitsPerPixel = 32;
  argbFormat.BytesPerPixel = 4;

  // For DXT5 we need RGBA
#if defined(HOST_BIGENDIAN)
  argbFormat.Amask = 0x000000ff;
  argbFormat.Ashift = 0;
  argbFormat.Rmask = 0x0000ff00;
  argbFormat.Rshift = 8;
  argbFormat.Gmask = 0x00ff0000;
  argbFormat.Gshift = 16;
  argbFormat.Bmask = 0xff000000;
  argbFormat.Bshift = 24;
#else
  argbFormat.Amask = 0xff000000;
  argbFormat.Ashift = 24;
  argbFormat.Rmask = 0x00ff0000;
  argbFormat.Rshift = 16;
  argbFormat.Gmask = 0x0000ff00;
  argbFormat.Gshift = 8;
  argbFormat.Bmask = 0x000000ff;
  argbFormat.Bshift = 0;
#endif

  int width, height;
  unsigned int format = 0;
  SDL_Surface *argbImage = SDL_ConvertSurface(image, &argbFormat, 0);
  unsigned char* argb = (unsigned char*)argbImage->pixels;
  unsigned int compressedSize = 0;
  unsigned char* compressed = NULL;
  
  width  = image->w;
  height = image->h;
  bool hasAlpha = HasAlpha(argb, width, height);
  
  if (flags & FLAGS_USE_DXT)
  {
    double colorMSE, alphaMSE;
    compressedSize = squish::GetStorageRequirements(width, height, squish::kDxt5);
    compressed = new unsigned char[compressedSize];
    // first try DXT1, which is only 4bits/pixel
    CompressImage(argb, width, height, compressed, squish::kDxt1, colorMSE, alphaMSE);
    if (colorMSE < maxMSE && alphaMSE < maxMSE)
    { // success - use it
      compressedSize = squish::GetStorageRequirements(width, height, squish::kDxt1);
      format = XB_FMT_DXT1;
    }
    /* 
    if (!format && alphaMSE == 0 && (flags & FLAGS_ALLOW_YCOCG) == FLAGS_ALLOW_YCOCG)
    { 
      // no alpha channel, so DXT5YCoCg is going to be the best DXT5 format
      CompressImage(argb, width, height, compressed, squish::kDxt5 | squish::kUseYCoCg, colorMSE, alphaMSE);
      if (colorMSE < maxMSE && alphaMSE < maxMSE)
      { // success - use it
        compressedSize = squish::GetStorageRequirements(width, height, squish::kDxt5);
        format = XB_FMT_DXT5_YCoCg;
      }
    }
    */
    if (!format)
    { // try DXT3 and DXT5 - use whichever is better (color is the same, but alpha will be different)
      CompressImage(argb, width, height, compressed, squish::kDxt3, colorMSE, alphaMSE);
      if (colorMSE < maxMSE)
      { // color is fine, test DXT5 as well
        double dxt5MSE;
        squish::u8* compressed2 = new squish::u8[squish::GetStorageRequirements(width, height, squish::kDxt5)];
        CompressImage(argb, width, height, compressed2, squish::kDxt5, colorMSE, dxt5MSE);
        if (alphaMSE < maxMSE && alphaMSE < dxt5MSE)
        { // DXT3 passes and is best
          compressedSize = squish::GetStorageRequirements(width, height, squish::kDxt3);
          format = XB_FMT_DXT3;
        }
        else if (dxt5MSE < maxMSE)
        { // DXT5 passes
          compressedSize = squish::GetStorageRequirements(width, height, squish::kDxt5);
          memcpy(compressed, compressed2, compressedSize);
          format = XB_FMT_DXT5;
        }
        delete[] compressed2;
      }
    }
  }

  CXBTFFrame frame; 
  if (format)
  {
    frame = appendContent(writer, width, height, compressed, compressedSize, format, hasAlpha, flags);
    if (compressedSize)
      delete[] compressed;
  }
  else
  {
    // none of the compressed stuff works for us, so we use 32bit texture
    format = XB_FMT_A8R8G8B8;
    frame = appendContent(writer, width, height, argb, (width * height * 4), format, hasAlpha, flags);
  }

  SDL_FreeSurface(argbImage);
  return frame;
}

void Usage()
{
  puts("Usage:");
  puts("  -help            Show this screen.");
  puts("  -input <dir>     Input directory. Default: current dir");
  puts("  -output <dir>    Output directory/filename. Default: Textures.xpr");
  puts("  -dupecheck       Enable duplicate file detection. Reduces output file size. Default: on");
  puts("  -use_lzo         Use lz0 packing.     Default: on");
  puts("  -use_dxt         Use DXT compression. Default: on");
  puts("  -use_none        Use No  compression. Default: off");
}

static bool checkDupe(struct MD5Context* ctx,
                      map<string,unsigned int>& hashes,
                      vector<unsigned int>& dupes, unsigned int pos)
{
  unsigned char digest[17];
  MD5Final(digest,ctx);
  digest[16] = 0;
  char hex[33];
  sprintf(hex, "%02X%02X%02X%02X%02X%02X%02X%02X"\
      "%02X%02X%02X%02X%02X%02X%02X%02X", digest[0], digest[1], digest[2],
      digest[3], digest[4], digest[5], digest[6], digest[7], digest[8],
      digest[9], digest[10], digest[11], digest[12], digest[13], digest[14],
      digest[15]);
  hex[32] = 0;
  map<string,unsigned int>::iterator it = hashes.find(hex);
  if (it != hashes.end())
  {
    dupes[pos] = it->second; 
    return true;
  }

  hashes.insert(make_pair(hex,pos));
  dupes[pos] = pos;

  return false;
}

int createBundle(const std::string& InputDir, const std::string& OutputFile, double maxMSE, unsigned int flags, bool dupecheck)
{
  map<string,unsigned int> hashes;
  vector<unsigned int> dupes;
  CXBTF xbtf;
  CreateSkeletonHeader(xbtf, InputDir);
  dupes.resize(xbtf.GetFiles().size());
  if (!dupecheck)
  {
    for (unsigned int i=0;i<dupes.size();++i)
      dupes[i] = i;
  }

  CXBTFWriter writer(xbtf, OutputFile);
  if (!writer.Create())
  {
    printf("Error creating file\n");
    return 1;
  }

  std::vector<CXBTFFile>& files = xbtf.GetFiles();
  for (size_t i = 0; i < files.size(); i++)
  {
    struct MD5Context ctx;
    MD5Init(&ctx);
    CXBTFFile& file = files[i];

    std::string fullPath = InputDir;
    fullPath += file.GetPath();

    std::string output = file.GetPath();
    output = output.substr(0, 40);
    while (output.size() < 46)
      output += ' ';
    if (!IsGIF(fullPath.c_str()))
    {
      // Load the image
      SDL_Surface* image = IMG_Load(fullPath.c_str());
      if (!image)
      {
        printf("...unable to load image %s\n", file.GetPath());
        continue;
      }

      bool skip=false;
      printf("%s", output.c_str());
      if (dupecheck)
      {
        MD5Update(&ctx,(const uint8_t*)image->pixels,image->h*image->pitch);
        if (checkDupe(&ctx,hashes,dupes,i))
        {
          printf("****  duplicate of %s\n", files[dupes[i]].GetPath());
          file.GetFrames().insert(file.GetFrames().end(),
            files[dupes[i]].GetFrames().begin(), files[dupes[i]].GetFrames().end());
          skip = true;
        }
      }

      if (!skip)
      {
        CXBTFFrame frame = createXBTFFrame(image, writer, maxMSE, flags);

        printf("%s%c (%d,%d @ %"PRIu64" bytes)\n", GetFormatString(frame.GetFormat()), frame.HasAlpha() ? ' ' : '*',
          frame.GetWidth(), frame.GetHeight(), frame.GetUnpackedSize());

        file.SetLoop(0);
        file.GetFrames().push_back(frame);
      }
      SDL_FreeSurface(image);
    }
    else
    {
      int gnAG = AG_LoadGIF(fullPath.c_str(), NULL, 0);
      AG_Frame* gpAG = new AG_Frame[gnAG];
      AG_LoadGIF(fullPath.c_str(), gpAG, gnAG);

      printf("%s\n", output.c_str());
      bool skip=false;
      if (dupecheck)
      {
        for (int j = 0; j < gnAG; j++)
          MD5Update(&ctx,
            (const uint8_t*)gpAG[j].surface->pixels,
            gpAG[j].surface->h * gpAG[j].surface->pitch);

        if (checkDupe(&ctx,hashes,dupes,i))
        {
          printf("****  duplicate of %s\n", files[dupes[i]].GetPath());
          file.GetFrames().insert(file.GetFrames().end(),
            files[dupes[i]].GetFrames().begin(), files[dupes[i]].GetFrames().end());
          skip = true;
        }
      }

      if (!skip)
      {
        for (int j = 0; j < gnAG; j++)
        {
          printf("    frame %4i                                ", j);
          CXBTFFrame frame = createXBTFFrame(gpAG[j].surface, writer, maxMSE, flags);
          frame.SetDuration(gpAG[j].delay);
          file.GetFrames().push_back(frame);
          printf("%s%c (%d,%d @ %"PRIu64" bytes)\n", GetFormatString(frame.GetFormat()), frame.HasAlpha() ? ' ' : '*',
            frame.GetWidth(), frame.GetHeight(), frame.GetUnpackedSize());
        }
      }
      AG_FreeSurfaces(gpAG, gnAG);
      delete [] gpAG;

      file.SetLoop(0);
    }
  }

  if (!writer.UpdateHeader(dupes))
  {
    printf("Error writing header to file\n");
    return 1;
  }

  if (!writer.Close())
  {
    printf("Error closing file\n");
    return 1;
  }

  return 0;
}

int main(int argc, char* argv[])
{
#ifdef USE_LZO_PACKING
  if (lzo_init() != LZO_E_OK)
    return 1;
#endif
  bool valid = false;
  unsigned int flags = 0;
  bool dupecheck = false;
  CmdLineArgs args(argc, (const char**)argv);

  // setup some defaults, dxt with lzo post packing,
  flags = FLAGS_USE_DXT;
#ifdef USE_LZO_PACKING
  flags |= FLAGS_USE_LZO;
#endif

  if (args.size() == 1)
  {
    Usage();
    return 1;
  }

  std::string InputDir;
  std::string OutputFilename = "Textures.xbt";

  for (unsigned int i = 1; i < args.size(); ++i)
  {
    if (!stricmp(args[i], "-help") || !stricmp(args[i], "-h") || !stricmp(args[i], "-?"))
    {
      Usage();
      return 1;
    }
    else if (!stricmp(args[i], "-input") || !stricmp(args[i], "-i"))
    {
      InputDir = args[++i];
      valid = true;
    }
    else if (!strcmp(args[i], "-dupecheck"))
    {
      dupecheck = true;
    }
    else if (!stricmp(args[i], "-output") || !stricmp(args[i], "-o"))
    {
      OutputFilename = args[++i];
      valid = true;
#ifdef _LINUX
      char *c = NULL;
      while ((c = (char *)strchr(OutputFilename.c_str(), '\\')) != NULL) *c = '/';
#endif
    }
    else if (!stricmp(args[i], "-use_none"))
    {
      flags &= ~FLAGS_USE_DXT;
    }
    else if (!stricmp(args[i], "-use_dxt"))
    {
      flags |= FLAGS_USE_DXT;
    }
#ifdef USE_LZO_PACKING
    else if (!stricmp(args[i], "-use_lzo"))
    {
      flags |= FLAGS_USE_LZO;
    }
#endif
    else
    {
      printf("Unrecognized command line flag: %s\n", args[i]);
    }
  }

  if (!valid)
  {
    Usage();
    return 1;
  }

  size_t pos = InputDir.find_last_of(DIR_SEPARATOR);
  if (pos != InputDir.length() - 1)
    InputDir += DIR_SEPARATOR;

  double maxMSE = 1.5;    // HQ only please
  createBundle(InputDir, OutputFilename, maxMSE, flags, dupecheck);
}
