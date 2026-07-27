/*
 *  Copyright (C) 2004-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// clang-format off
#ifdef TARGET_WINDOWS
#include <sys/types.h>
#define __STDC_FORMAT_MACROS
#include <cinttypes>
#define platform_stricmp _stricmp
#else
#include <inttypes.h>
#define platform_stricmp strcasecmp
#endif
#include <algorithm>
#include <cerrno>
#include <dirent.h>
#include <map>

#include "guilib/TextureFormats.h"
#include "guilib/XBTF.h"
#include "guilib/XBTFReader.h"

#include "DecoderManager.h"

#include "XBTFWriter.h"
#include "md5.h"
#include "cmdlineargs.h"

#ifdef TARGET_WINDOWS
#define strncasecmp _strnicmp
#endif

#include <vector>

#include <lzo/lzo1x.h>
#include <sys/stat.h>

#define DIR_SEPARATOR '/'
// clang-format on

namespace
{

const char* GetFormatString(KD_TEX_FMT format)
{
  switch (format)
  {
    case KD_TEX_FMT_SDR_R8:
      return "R8   ";
    case KD_TEX_FMT_SDR_RG8:
      return "RG8  ";
    case KD_TEX_FMT_SDR_RGBA8:
      return "RGBA8";
    case KD_TEX_FMT_SDR_BGRA8:
      return "BGRA8";
    case KD_TEX_FMT_S3TC_RGB8:
    case KD_TEX_FMT_S3TC_RGB8_A1:
    case KD_TEX_FMT_S3TC_RGB8_A4:
    case KD_TEX_FMT_S3TC_RGBA8:
      return "S3TC ";
    case KD_TEX_FMT_RGTC_R11:
    case KD_TEX_FMT_RGTC_RG11:
      return "RGTC ";
    case KD_TEX_FMT_BPTC_RGB16F:
    case KD_TEX_FMT_BPTC_RGBA8:
      return "BPTC ";
    case KD_TEX_FMT_ETC1_RGB8:
      return "ETC1 ";
    case KD_TEX_FMT_ETC2_R11:
    case KD_TEX_FMT_ETC2_RG11:
    case KD_TEX_FMT_ETC2_RGB8:
    case KD_TEX_FMT_ETC2_RGB8_A1:
    case KD_TEX_FMT_ETC2_RGBA8:
      return "ETC2 ";
    case KD_TEX_FMT_ASTC_LDR_4x4:
    case KD_TEX_FMT_ASTC_LDR_5x4:
    case KD_TEX_FMT_ASTC_LDR_5x5:
    case KD_TEX_FMT_ASTC_LDR_6x5:
    case KD_TEX_FMT_ASTC_LDR_6x6:
    case KD_TEX_FMT_ASTC_LDR_8x5:
    case KD_TEX_FMT_ASTC_LDR_8x6:
    case KD_TEX_FMT_ASTC_LDR_8x8:
    case KD_TEX_FMT_ASTC_LDR_10x5:
    case KD_TEX_FMT_ASTC_LDR_10x6:
    case KD_TEX_FMT_ASTC_LDR_10x8:
    case KD_TEX_FMT_ASTC_LDR_10x10:
    case KD_TEX_FMT_ASTC_LDR_12x10:
    case KD_TEX_FMT_ASTC_LDR_12x12:
      return "ASTC ";
    default:
      return "?????";
  }
}

// clang-format off
void Usage()
{
  puts("Texture Packer Version 3");
  puts("");
  puts("Tool to pack XBT 3 texture files, used in Kodi Piers (v22) and newer.");
  puts("Accepts the following file formats as input:");
  puts("-PNG (preferred)");
  puts("-KTX (for compressed textures)");
  puts("-JPG");
  puts("-GIF");
  puts("");
  puts("");
  puts("Usage:");
  puts("  -help            Show this screen.");
  puts("  -input <dir>     Input directory. Default: current dir");
  puts("  -output <dir>    Output directory/filename. Default: Textures.xbt");
  puts("  -dupecheck       Enable duplicate file detection. Reduces output file size. Default: off");
  puts("  -nocompress      Disable LZO compression. Default: off");
  puts("  -astc            Substitution of *.astc.ktx files. Default: off");
  puts("  -bptc            Substitution of *.bptc.ktx files. Default: off");
  puts("  -etc1            Substitution of *.etc1.ktx files. Default: off");
  puts("  -etc2            Substitution of *.etc2.ktx files. Default: off");
  puts("  -rgtc            Substitution of *.rgtc.ktx files. Default: off");
  puts("  -s3tc            Substitution of *.s3tc.ktx files. Default: off");
  puts("");
  puts("");
  puts("Substitution of files");
  puts("");
  puts("The goal of file substitution is to create specialised texture bundles, containing a "
       "specific set of (compressed) textures. If compressed textures with the right file names "
       "are provided and substitution is enabled, the uncompressed (raw) textures get replaced by "
       "their compressed counterpart. The set should reflect capabilities of the target platform. "
       "Typical sets would be:");
  puts("-Embedded (GLES 2.0): ETC1 (Mali Utgard, VideoCore IV)");
  puts("-Embedded (GLES 3.0): ETC2, ETC1 (early Mali Midgard)");
  puts("-Emdedded (Modern): ASTC, ETC2, ETC1 (late Mali Midgard, VideoCore VI)");
  puts("-Desktop (Legacy): S3TC (NVidia Ion)");
  puts("-Desktop (Modern): BPTC, RGTC, S3TC (DX11 capable hardware)");
  puts("");
  puts("For example, if the switch \"-s3tc\" is active, files with the ending \"*.png.s3tc.ktx\" "
       "will be placed as \"*.png\" in the XBT file. This means that the content of a file with the "
       "name of \"icon.png\" will be replaced by the content of the file \"icon.png.s3tc.ktx\".");
  puts("");
  puts("Any file with such an substitution \"extension\" won't be packed directly. If multiple "
       "switches are active, the priority is as follows (decreasing): ASTC, BPTC, ETC2, RGTC, ETC1 "
       "and S3TC.");
}
// clang-format on

} // namespace

class TexturePacker
{
public:
  TexturePacker() = default;
  ~TexturePacker() = default;

  void EnableDupeCheck() { m_dupecheck = true; }

  void EnableVerboseOutput();

  int createBundle(const std::string& InputDir, const std::string& OutputFile);

  void SetFlags(unsigned int flags) { m_flags = flags; }
  void EnableTextureFamily(KD_TEX_FMT format) { m_enabledTextureFamilies.emplace_back(format); }
  void DisableCompression() { m_compress = false; }

private:
  void CreateSkeletonHeader(CXBTFWriter& xbtfWriter,
                            const std::string& fullPath,
                            const std::string& relativePath = "");

  void SubstitudeFile(CXBTFWriter& xbtfWriter, const std::string& fileName);
  unsigned int GetSubstitutionPriority(KD_TEX_FMT textureFamily);

  CXBTFFrame CreateXBTFFrame(DecodedFrame& decodedFrame, CXBTFWriter& writer) const;

  bool CheckDupe(MD5Context* ctx, unsigned int pos);

  void ConvertToSingleChannel(RGBAImage& image, uint32_t channel);
  void ConvertToDualChannel(RGBAImage& image);
  void ReduceChannels(RGBAImage& image);

  DecoderManager decoderManager;

  std::map<std::string, unsigned int> m_hashes;
  std::vector<unsigned int> m_dupes;

  bool m_dupecheck{false};
  bool m_verbose{false};
  unsigned int m_flags{0};
  bool m_compress{true};

  std::vector<KD_TEX_FMT> m_enabledTextureFamilies{};
};

void TexturePacker::EnableVerboseOutput()
{
  decoderManager.EnableVerboseOutput();
  m_verbose = true;
}

void TexturePacker::CreateSkeletonHeader(CXBTFWriter& xbtfWriter,
                                         const std::string& fullPath,
                                         const std::string& relativePath)
{
  struct dirent* dp;
  struct stat stat_p;
  DIR *dirp = opendir(fullPath.c_str());

  if (dirp)
  {
    for (errno = 0; (dp = readdir(dirp)); errno = 0)
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
          if (!tmpPath.empty())
          {
            tmpPath += "/";
          }

          CreateSkeletonHeader(xbtfWriter, fullPath + DIR_SEPARATOR + dp->d_name,
                               tmpPath + dp->d_name);
        }
        else if (decoderManager.IsSupportedGraphicsFile(dp->d_name))
        {
          std::string fileName = "";
          if (!relativePath.empty())
          {
            fileName += relativePath;
            fileName += "/";
          }

          fileName += dp->d_name;

          if (decoderManager.IsSubstitutionFile(fileName))
          {
            SubstitudeFile(xbtfWriter, fileName);
          }
          else if (xbtfWriter.Exists(fileName))
          {
            continue;
          }
          else
          {
            CXBTFFile file;
            file.SetPath(fileName);
            xbtfWriter.AddFile(file);
          }
        }
      }
    }
    if (errno)
      fprintf(stderr, "Error reading directory %s (%s)\n", fullPath.c_str(), strerror(errno));

    closedir(dirp);
  }
  else
  {
    fprintf(stderr, "Error opening %s (%s)\n", fullPath.c_str(), strerror(errno));
  }
}

void TexturePacker::SubstitudeFile(CXBTFWriter& xbtfWriter, const std::string& fileName)
{
  CXBTFFile file;
  std::string substitutedFileName = fileName;
  KD_TEX_FMT textureFamily = KD_TEX_FMT_UNKNOWN;
  decoderManager.SubstitudeFileName(textureFamily, substitutedFileName);

  if (std::find(m_enabledTextureFamilies.begin(), m_enabledTextureFamilies.end(), textureFamily) ==
      m_enabledTextureFamilies.end())
    return;

  const unsigned int priority = GetSubstitutionPriority(textureFamily);
  const bool exists = xbtfWriter.Get(substitutedFileName, file);

  if (file.GetSubstitutionPriority() >= priority)
    return;
  file.SetSubstitutionPriority(priority);

  if (exists)
  {
    file.SetRealPath(fileName);
    xbtfWriter.UpdateFile(file);
  }
  else
  {
    file.SetPath(substitutedFileName);
    file.SetRealPath(fileName);
    xbtfWriter.AddFile(file);
  }
}

unsigned int TexturePacker::GetSubstitutionPriority(KD_TEX_FMT textureFamily)
{
  switch (textureFamily)
  {
    case KD_TEX_FMT_ASTC_LDR:
      return 6;
    case KD_TEX_FMT_BPTC:
      return 5;
    case KD_TEX_FMT_ETC2:
      return 4;
    case KD_TEX_FMT_RGTC:
      return 3;
    case KD_TEX_FMT_ETC1:
      return 2;
    case KD_TEX_FMT_S3TC:
      return 1;
    case KD_TEX_FMT_SDR:
    default:
      return 0;
  }
}

CXBTFFrame TexturePacker::CreateXBTFFrame(DecodedFrame& decodedFrame, CXBTFWriter& writer) const
{
  const unsigned int delay = decodedFrame.delay;
  const unsigned int width = decodedFrame.rgbaImage.width;
  const unsigned int height = decodedFrame.rgbaImage.height;
  const unsigned int size = decodedFrame.rgbaImage.size;
  const uint32_t format = static_cast<uint32_t>(decodedFrame.rgbaImage.textureFormat) |
                          static_cast<uint32_t>(decodedFrame.rgbaImage.textureAlpha) |
                          static_cast<uint32_t>(decodedFrame.rgbaImage.textureSwizzle);
  unsigned char* data = (unsigned char*)decodedFrame.rgbaImage.pixels.data();

  CXBTFFrame frame;
  lzo_uint packedSize = size;

  if (m_compress)
  {
    // grab a temporary buffer for unpacking into
    packedSize = size + size / 16 + 64 + 3; // see simple.c in lzo

    std::vector<uint8_t> packed;
    packed.resize(packedSize);

    std::vector<uint8_t> working;
    working.resize(LZO1X_999_MEM_COMPRESS);

    if (lzo1x_999_compress(data, size, packed.data(), &packedSize, working.data()) != LZO_E_OK ||
        packedSize > size)
    {
      // compression failed, or compressed size is bigger than uncompressed, so store as uncompressed
      packedSize = size;
      writer.AppendContent(data, size);
    }
    else
    { // success
      lzo_uint optimSize = size;
      if (lzo1x_optimize(packed.data(), packedSize, data, &optimSize, NULL) != LZO_E_OK ||
          optimSize != size)
      { //optimisation failed
        packedSize = size;
        writer.AppendContent(data, size);
      }
      else
      { // success
        writer.AppendContent(packed.data(), packedSize);
      }
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
  frame.SetFormat(format);
  frame.SetDuration(delay);
  return frame;
}

bool TexturePacker::CheckDupe(MD5Context* ctx,
                              unsigned int pos)
{
  unsigned char digest[17];
  MD5Final(digest,ctx);
  digest[16] = 0;
  char hex[33];
  snprintf(hex, sizeof(hex),
           "%02X%02X%02X%02X%02X%02X%02X%02X"
           "%02X%02X%02X%02X%02X%02X%02X%02X",
           digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
           digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14],
           digest[15]);
  hex[32] = 0;
  std::map<std::string, unsigned int>::iterator it = m_hashes.find(hex);
  if (it != m_hashes.end())
  {
    m_dupes[pos] = it->second;
    return true;
  }

  m_hashes[hex] = pos;
  m_dupes[pos] = pos;

  return false;
}

void TexturePacker::ConvertToSingleChannel(RGBAImage& image, uint32_t channel)
{
  uint32_t size = (image.width * image.height);
  for (uint32_t i = 0; i < size; i++)
  {
    image.pixels[i] = image.pixels[i * 4 + channel];
  }

  image.textureFormat = KD_TEX_FMT_SDR_R8;

  image.bbp = 8;
  image.pitch = 1 * image.width;
  image.size = size;
}

void TexturePacker::ConvertToDualChannel(RGBAImage& image)
{
  uint32_t size = (image.width * image.height);
  for (uint32_t i = 0; i < size; i++)
  {
    image.pixels[i * 2] = image.pixels[i * 4];
    image.pixels[i * 2 + 1] = image.pixels[i * 4 + 3];
  }
  image.textureFormat = KD_TEX_FMT_SDR_RG8;
  image.bbp = 16;
  image.pitch = 2 * image.width;
  image.size = 2 * size;
}

void TexturePacker::ReduceChannels(RGBAImage& image)
{
  if (image.textureFormat != KD_TEX_FMT_SDR_BGRA8)
    return;

  uint32_t size = (image.width * image.height);
  uint8_t red = image.pixels[0];
  uint8_t green = image.pixels[1];
  uint8_t blue = image.pixels[2];
  uint8_t alpha = image.pixels[3];
  bool uniformRed = true;
  bool uniformGreen = true;
  bool uniformBlue = true;
  bool uniformAlpha = true;
  bool isGrey = true;
  bool isIntensity = true;

  // Checks each pixel for various properties.
  for (uint32_t i = 0; i < size; i++)
  {
    if (image.pixels[i * 4] != red)
      uniformRed = false;
    if (image.pixels[i * 4 + 1] != green)
      uniformGreen = false;
    if (image.pixels[i * 4 + 2] != blue)
      uniformBlue = false;
    if (image.pixels[i * 4 + 3] != alpha)
      uniformAlpha = false;
    if (image.pixels[i * 4] != image.pixels[i * 4 + 1] ||
        image.pixels[i * 4] != image.pixels[i * 4 + 2])
      isGrey = false;
    if (image.pixels[i * 4] != image.pixels[i * 4 + 1] ||
        image.pixels[i * 4] != image.pixels[i * 4 + 2] ||
        image.pixels[i * 4] != image.pixels[i * 4 + 3])
      isIntensity = false;
  }

  if (uniformAlpha && alpha != 0xff)
    printf("WARNING: uniform alpha detected! Consider using diffusecolor!\n");

  bool isWhite = red == 0xff && green == 0xff && blue == 0xff;
  if (uniformRed && uniformGreen && uniformBlue && !isWhite)
    printf("WARNING: uniform color detected! Consider using diffusecolor!\n");

  if (uniformAlpha && alpha == 0xff)
  {
    // we have a opaque texture, L or RGBX
    if (isGrey)
    {
      ConvertToSingleChannel(image, 1);
      image.textureSwizzle = KD_TEX_SWIZ_RRR1;
    }
    image.textureAlpha = KD_TEX_ALPHA_OPAQUE;
  }
  else if (uniformRed && uniformGreen && uniformBlue && isWhite)
  {
    // an alpha only texture
    ConvertToSingleChannel(image, 3);
    image.textureSwizzle = KD_TEX_SWIZ_111R;
  }
  else if (isIntensity)
  {
    // this is an intensity (GL_INTENSITY) texture
    ConvertToSingleChannel(image, 0);
    image.textureSwizzle = KD_TEX_SWIZ_RRRR;
  }
  else if (isGrey)
  {
    // a LA texture
    ConvertToDualChannel(image);
    image.textureSwizzle = KD_TEX_SWIZ_RRRG;
  }
  else
  {
    // BGRA
  }
}

int TexturePacker::createBundle(const std::string& InputDir, const std::string& OutputFile)
{
  CXBTFWriter writer(OutputFile);
  if (!writer.Create())
  {
    fprintf(stderr, "Error creating file\n");
    return 1;
  }

  CreateSkeletonHeader(writer, InputDir);

  std::vector<CXBTFFile> files = writer.GetFiles();
  m_dupes.resize(files.size());

  for (size_t i = 0; i < files.size(); i++)
  {
    struct MD5Context ctx;
    MD5Init(&ctx);
    CXBTFFile& file = files[i];

    std::string fullPath = InputDir;
    fullPath += file.GetRealPath();

    std::string output = file.GetPath();

    DecodedFrames frames;
    bool loaded = decoderManager.LoadFile(fullPath, frames);

    if (!loaded || frames.frameList.empty())
    {
      fprintf(stderr, "...unable to load image %s\n", file.GetPath().c_str());
      continue;
    }

    for (unsigned int j = 0; j < frames.frameList.size(); j++)
      ReduceChannels(frames.frameList[j].rgbaImage);

    if(m_verbose)
      printf("%s\n", output.c_str());

    bool skip=false;
    if (m_dupecheck)
    {
      for (unsigned int j = 0; j < frames.frameList.size(); j++)
        MD5Update(&ctx, (const uint8_t*)frames.frameList[j].rgbaImage.pixels.data(),
                  frames.frameList[j].rgbaImage.size);

      if (CheckDupe(&ctx, i))
      {
        if(m_verbose)
          printf("****  duplicate of %s\n", files[m_dupes[i]].GetPath().c_str());

        file.GetFrames().insert(file.GetFrames().end(),
                                files[m_dupes[i]].GetFrames().begin(),
                                files[m_dupes[i]].GetFrames().end());
        skip = true;
      }
    }
    else
    {
      m_dupes[i] = i;
    }

    if (!skip)
    {
      for (unsigned int j = 0; j < frames.frameList.size(); j++)
      {
        CXBTFFrame frame = CreateXBTFFrame(frames.frameList[j], writer);
        file.GetFrames().push_back(frame);
        if(m_verbose)
        {
          printf("    frame %4i (delay:%4i)                         %s%c (%d,%d @ %" PRIu64
                 " bytes)\n",
                 j, frame.GetDuration(), GetFormatString(frame.GetKDFormat()),
                 frame.HasAlpha() ? ' ' : '*', frame.GetWidth(), frame.GetHeight(),
                 frame.GetUnpackedSize());
        }
      }
    }
    file.SetLoop(0);

    writer.UpdateFile(file);
  }

  if (!writer.UpdateHeader(m_dupes))
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

  CmdLineArgs args(argc, (const char**)argv);

  if (args.size() == 1)
  {
    Usage();
    return 1;
  }

  std::string InputDir;
  std::string OutputFilename = "Textures.xbt";

  TexturePacker texturePacker;

  bool rawSDRTextures = true;

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
      texturePacker.EnableDupeCheck();
    }
    else if (!strcmp(args[i], "-verbose"))
    {
      texturePacker.EnableVerboseOutput();
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
    else if (!strcmp(args[i], "-nosdr"))
    {
      rawSDRTextures = false;
    }
    else if (!strcmp(args[i], "-etc1"))
    {
      texturePacker.EnableTextureFamily(KD_TEX_FMT_ETC1);
    }
    else if (!strcmp(args[i], "-etc2"))
    {
      texturePacker.EnableTextureFamily(KD_TEX_FMT_ETC2);
    }
    else if (!strcmp(args[i], "-astc"))
    {
      texturePacker.EnableTextureFamily(KD_TEX_FMT_ASTC_LDR);
    }
    else if (!strcmp(args[i], "-s3tc"))
    {
      texturePacker.EnableTextureFamily(KD_TEX_FMT_S3TC);
    }
    else if (!strcmp(args[i], "-rgtc"))
    {
      texturePacker.EnableTextureFamily(KD_TEX_FMT_RGTC);
    }
    else if (!strcmp(args[i], "-bptc"))
    {
      texturePacker.EnableTextureFamily(KD_TEX_FMT_BPTC);
    }
    else if (!strcmp(args[i], "-nocompress"))
    {
      texturePacker.DisableCompression();
    }
    else
    {
      fprintf(stderr, "Unrecognized command line flag: %s\n", args[i]);
    }
  }

  if (rawSDRTextures)
    texturePacker.EnableTextureFamily(KD_TEX_FMT_SDR);

  if (!valid)
  {
    Usage();
    return 1;
  }

  size_t pos = InputDir.find_last_of(DIR_SEPARATOR);
  if (pos != InputDir.length() - 1)
    InputDir += DIR_SEPARATOR;

  texturePacker.createBundle(InputDir, OutputFilename);
}
