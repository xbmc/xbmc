/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <sys/types.h>
#include <squish.h>
#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "cmdlineargs.h"
#ifdef TARGET_WINDOWS
#define strncasecmp strnicmp
#endif
#include "DDSImage.h"
#include "XBTF.h"

#undef main

const char *GetFormatString(unsigned int format)
{
  switch (format)
  {
  case squish::kDxt1:
    return "DXT1 ";
  case squish::kDxt5:
    return "YCoCg";
  default:
    return "?????";
  }
}

void CompressImage(const squish::u8 *brga, int width, int height, squish::u8 *compressed, unsigned int flags, double &colorMSE, double &alphaMSE)
{
  squish::CompressImage(brga, width, height, compressed, flags | squish::kSourceBGRA);
  squish::ComputeMSE(brga, width, height, compressed, flags | squish::kSourceBGRA, colorMSE, alphaMSE);
}

void CompressToDDS(SDL_Surface* image, unsigned int format, CDDSImage &out)
{
  // Convert to ARGB
  SDL_PixelFormat argbFormat;
  memset(&argbFormat, 0, sizeof(SDL_PixelFormat));
  argbFormat.BitsPerPixel = 32;
  argbFormat.BytesPerPixel = 4;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
  argbFormat.Amask = 0xff000000;
  argbFormat.Ashift = 24;
  argbFormat.Rmask = 0x00ff0000;
  argbFormat.Rshift = 16;
  argbFormat.Gmask = 0x0000ff00;
  argbFormat.Gshift = 8;
  argbFormat.Bmask = 0x000000ff;
  argbFormat.Bshift = 0;
#else
  argbFormat.Amask = 0x000000ff;
  argbFormat.Ashift = 0;
  argbFormat.Rmask = 0x0000ff00;
  argbFormat.Rshift = 8;
  argbFormat.Gmask = 0x00ff0000;
  argbFormat.Gshift = 16;
  argbFormat.Bmask = 0xff000000;
  argbFormat.Bshift = 24;
#endif

  SDL_Surface *argbImage = SDL_ConvertSurface(image, &argbFormat, 0);

  double colorMSE, alphaMSE;
  if (format == XB_FMT_DXT1)
    CompressImage((squish::u8 *)argbImage->pixels, image->w, image->h, out.GetData(), squish::kDxt1, colorMSE, alphaMSE);
  else if (format == XB_FMT_DXT5)
    CompressImage((squish::u8 *)argbImage->pixels, image->w, image->h, out.GetData(), squish::kDxt5, colorMSE, alphaMSE);

  // print some info about the resulting image
  printf("Size: %dx%d %s in %u bytes. Quality: %5.2f\n", image->w, image->h, GetFormatString(format), out.GetSize(), colorMSE);

  SDL_FreeSurface(argbImage);
}

void Usage()
{
  puts("Usage: MakeDDS [-oN] input [output]");
  puts(" -o1 for DXT1");
  puts(" -o5 for DXT5");
}

void createDDS(const std::string& inputFile, const std::string& outputFile, unsigned int format)
{
  // Load the image
  SDL_Surface* image = IMG_Load(inputFile.c_str());
  if (!image)
  {
    printf("...unable to load image %s\n", inputFile.c_str());
    return;
  }

  CDDSImage dds(image->w, image->h, format);
  CompressToDDS(image, format, dds);

  // write to a DDS file
  dds.WriteFile(outputFile);

  SDL_FreeSurface(image);
}

int main(int argc, char* argv[])
{
  bool valid = false;
  CmdLineArgs args(argc, (const char**)argv);

  if (args.size() == 1)
  {
    Usage();
    return 1;
  }

  std::string inputFile;
  std::string outputFile;

  unsigned int format = squish::kDxt1;
  for (unsigned int i = 1; i < args.size(); ++i)
  {
    if (!stricmp(args[i], "--help") || !stricmp(args[i], "-?") || !stricmp(args[i], "?"))
    {
      Usage();
      return 1;
    }
    else if (!strncasecmp(args[i], "-o1", 3))
      format = XB_FMT_DXT1;
    else if (!strncasecmp(args[i], "-o5", 3))
      format = XB_FMT_DXT5;
    else if (!inputFile.size())
    {
      inputFile = args[i];
      valid = true;
    }
    else if (!outputFile.size())
    {
      outputFile = args[i];
      valid = true;
    }
    else
    {
      printf("Unrecognized command line flag: %s\n", args[i]);
    }
  }

  if (outputFile.empty())
  { // construct output file from input - rename to .dds
    size_t pos = inputFile.find_last_of('.');
    if (pos != std::string::npos)
    {
      outputFile = inputFile.substr(0, pos);
      outputFile += ".dds";
    }
  }

  if (!valid)
  {
    Usage();
    return 1;
  }

  createDDS(inputFile, outputFile, format);
}
