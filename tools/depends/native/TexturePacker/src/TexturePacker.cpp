/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifdef TARGET_WINDOWS
#include <sys/types.h>
#include <sys/stat.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#define platform_stricmp _stricmp
#else
#define platform_stricmp stricmp
#endif
#include <cerrno>
#include <dirent.h>
#include <map>

#include "guilib/XBTF.h"
#include "guilib/XBTFReader.h"

#include "DecoderManager.h"

#include "XBTFWriter.h"
#include "md5.h"
#include "cmdlineargs.h"

#ifdef TARGET_WINDOWS
#define strncasecmp _strnicmp
#endif

#include <lzo/lzo1x.h>

using namespace std;

#define FLAGS_USE_LZO     1

#define DIR_SEPARATOR "/"

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

void CreateSkeletonHeaderImpl(CXBTFWriter& xbtfWriter, std::string fullPath, std::string relativePath)
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

          CreateSkeletonHeaderImpl(xbtfWriter, fullPath + DIR_SEPARATOR + dp->d_name, tmpPath + dp->d_name);
        }
        else if (DecoderManager::IsSupportedGraphicsFile(dp->d_name))
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
          xbtfWriter.AddFile(file);
        }
      }
    }

    closedir(dirp);
  }
  else
  {
    fprintf(stderr, "Error opening %s (%s)\n", fullPath.c_str(), strerror(errno));
  }
}

void CreateSkeletonHeader(CXBTFWriter& xbtfWriter, std::string fullPath)
{
  std::string temp;
  CreateSkeletonHeaderImpl(xbtfWriter, fullPath, temp);
}

CXBTFFrame appendContent(CXBTFWriter &writer, int width, int height, unsigned char *data, unsigned int size, unsigned int format, bool hasAlpha, unsigned int flags)
{
  CXBTFFrame frame;
  lzo_uint packedSize = size;

  if ((flags & FLAGS_USE_LZO) == FLAGS_USE_LZO)
  {
    // grab a temporary buffer for unpacking into
    packedSize = size + size / 16 + 64 + 3; // see simple.c in lzo
    unsigned char *packed  = new unsigned char[packedSize];
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
        if (lzo1x_optimize(packed, packedSize, data, &optimSize, NULL) != LZO_E_OK || optimSize != size)
        { //optimisation failed
          packedSize = size;
          writer.AppendContent(data, size);
        }
        else
        { // success
          writer.AppendContent(packed, packedSize);
        }
      }
      delete[] working;
      delete[] packed;
    }
  }
  else
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

CXBTFFrame createXBTFFrame(RGBAImage &image, CXBTFWriter& writer, double maxMSE, unsigned int flags)
{

  int width, height;
  unsigned int format = 0;
  unsigned char* argb = (unsigned char*)image.pixels;
  
  width  = image.width;
  height = image.height;
  bool hasAlpha = HasAlpha(argb, width, height);

  CXBTFFrame frame; 
  format = XB_FMT_A8R8G8B8;
  frame = appendContent(writer, width, height, argb, (width * height * 4), format, hasAlpha, flags);

  return frame;
}

void Usage()
{
  puts("Usage:");
  puts("  -help            Show this screen.");
  puts("  -input <dir>     Input directory. Default: current dir");
  puts("  -output <dir>    Output directory/filename. Default: Textures.xbt");
  puts("  -dupecheck       Enable duplicate file detection. Reduces output file size. Default: off");
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
  CXBTFWriter writer(OutputFile);
  if (!writer.Create())
  {
    fprintf(stderr, "Error creating file\n");
    return 1;
  }

  map<string,unsigned int> hashes;
  vector<unsigned int> dupes;
  CreateSkeletonHeader(writer, InputDir);

  std::vector<CXBTFFile> files = writer.GetFiles();
  dupes.resize(files.size());
  if (!dupecheck)
  {
    for (unsigned int i=0;i<dupes.size();++i)
      dupes[i] = i;
  }

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

    DecodedFrames frames;
    bool loaded = DecoderManager::LoadFile(fullPath, frames);

    if (!loaded)
    {
      fprintf(stderr, "...unable to load image %s\n", file.GetPath().c_str());
      continue;
    }

    printf("%s\n", output.c_str());
    bool skip=false;
    if (dupecheck)
    {
      for (unsigned int j = 0; j < frames.frameList.size(); j++)
        MD5Update(&ctx,
          (const uint8_t*)frames.frameList[j].rgbaImage.pixels,
          frames.frameList[j].rgbaImage.height * frames.frameList[j].rgbaImage.pitch);

      if (checkDupe(&ctx,hashes,dupes,i))
      {
        printf("****  duplicate of %s\n", files[dupes[i]].GetPath().c_str());
        file.GetFrames().insert(file.GetFrames().end(),
                                files[dupes[i]].GetFrames().begin(),
                                files[dupes[i]].GetFrames().end());
        skip = true;
      }
    }

    if (!skip)
    {
      for (unsigned int j = 0; j < frames.frameList.size(); j++)
      {
        printf("    frame %4i (delay:%4i)                         ", j, frames.frameList[j].delay);
        CXBTFFrame frame = createXBTFFrame(frames.frameList[j].rgbaImage, writer, maxMSE, flags);
        frame.SetDuration(frames.frameList[j].delay);
        file.GetFrames().push_back(frame);
        printf("%s%c (%d,%d @ %" PRIu64 " bytes)\n", GetFormatString(frame.GetFormat()), frame.HasAlpha() ? ' ' : '*',
          frame.GetWidth(), frame.GetHeight(), frame.GetUnpackedSize());
      }
    }
    DecoderManager::FreeDecodedFrames(frames);
    file.SetLoop(0);

    writer.UpdateFile(file);
  }

  if (!writer.UpdateHeader(dupes))
  {
    fprintf(stderr, "Error writing header to file\n");
    return 1;
  }

  if (!writer.Close())
  {
    fprintf(stderr, "Error closing file\n");
    return 1;
  }

  return 0;
}

int main(int argc, char* argv[])
{
  if (lzo_init() != LZO_E_OK)
    return 1;
  bool valid = false;
  unsigned int flags = 0;
  bool dupecheck = false;
  CmdLineArgs args(argc, (const char**)argv);

  // setup some defaults, lzo packing,
  flags = FLAGS_USE_LZO;

  if (args.size() == 1)
  {
    Usage();
    return 1;
  }

  std::string InputDir;
  std::string OutputFilename = "Textures.xbt";

  for (unsigned int i = 1; i < args.size(); ++i)
  {
    if (!platform_stricmp(args[i], "-help") || !platform_stricmp(args[i], "-h") || !platform_stricmp(args[i], "-?"))
    {
      Usage();
      return 1;
    }
    else if (!platform_stricmp(args[i], "-input") || !platform_stricmp(args[i], "-i"))
    {
      InputDir = args[++i];
      valid = true;
    }
    else if (!strcmp(args[i], "-dupecheck"))
    {
      dupecheck = true;
    }
    else if (!platform_stricmp(args[i], "-output") || !platform_stricmp(args[i], "-o"))
    {
      OutputFilename = args[++i];
      valid = true;
#ifdef TARGET_POSIX
      char *c = NULL;
      while ((c = (char *)strchr(OutputFilename.c_str(), '\\')) != NULL) *c = '/';
#endif
    }
    else
    {
      fprintf(stderr, "Unrecognized command line flag: %s\n", args[i]);
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
  DecoderManager::InstantiateDecoders();
  createBundle(InputDir, OutputFilename, maxMSE, flags, dupecheck);
  DecoderManager::FreeDecoders();
}
