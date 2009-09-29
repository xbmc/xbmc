/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include <sys/types.h>
#include <dirent.h>
#include <squish.h>
#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "XBTF.h"
#include "XBTFWriter.h"
#include "SDL_anigif.h"
#include "cmdlineargs.h"
#ifdef _WIN32
#define strncasecmp strnicmp
#endif

#define DIR_SEPARATOR "/"
#define DIR_SEPARATOR_CHAR '/'

#undef main

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
  DIR *dirp = opendir(fullPath.c_str());
  
  while ((dp = readdir(dirp)) != NULL)
  {
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) 
    {
      continue;
    }
    
    if (dp->d_type == DT_DIR)
    {
      std::string tmpPath = relativePath;
      if (tmpPath.size() > 0)
      {
        tmpPath += "/";
      }
      
      CreateSkeletonHeaderImpl(xbtf, fullPath + DIR_SEPARATOR + dp->d_name, 
          tmpPath + dp->d_name);
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
      file.SetFormat(XB_FMT_DXT5);
      xbtf.GetFiles().push_back(file);
    }
  }
  
  closedir(dirp);
}

void CreateSkeletonHeader(CXBTF& xbtf, std::string fullPath)
{
  std::string temp;
  CreateSkeletonHeaderImpl(xbtf, fullPath, temp); 
}

CXBTFFrame createXBTFFrame(SDL_Surface* image, CXBTFWriter& writer, unsigned int format)
{
  // Convert to RGBA
  SDL_PixelFormat rgbaFormat;
  memset(&rgbaFormat, 0, sizeof(SDL_PixelFormat));
  rgbaFormat.BitsPerPixel = 32;
  rgbaFormat.BytesPerPixel = 4;
 
  // For DXT5 we need RGBA
#if SDL_BYTEORDER == SDL_LIL_ENDIAN    
  rgbaFormat.Rmask = 0x000000ff;
  rgbaFormat.Rshift = 0;
  rgbaFormat.Gmask = 0x0000ff00;
  rgbaFormat.Gshift = 8;
  rgbaFormat.Bmask = 0x00ff0000;
  rgbaFormat.Bshift = 16;
  rgbaFormat.Amask = 0xff000000;
  rgbaFormat.Ashift = 24;
#else    
  rgbaFormat.Amask = 0x000000ff;
  rgbaFormat.Ashift = 0;
  rgbaFormat.Rmask = 0x0000ff00;
  rgbaFormat.Rshift = 8;
  rgbaFormat.Gmask = 0x00ff0000;
  rgbaFormat.Gshift = 16;
  rgbaFormat.Bmask = 0xff000000;
  rgbaFormat.Bshift = 24;
#endif

  
  SDL_Surface *rgbaImage = SDL_ConvertSurface(image, &rgbaFormat, 0);

  int compressedSize = 0;
  CXBTFFrame frame;
  
  // Compress to DXT5
  compressedSize = squish::GetStorageRequirements(image->w, rgbaImage->h, squish::kDxt5);
  squish::u8* compressed = new squish::u8[compressedSize];    
  squish::CompressImage((const squish::u8*) rgbaImage->pixels, image->w, rgbaImage->h, compressed, squish::kDxt5);    
  frame.SetPackedSize(compressedSize);
  frame.SetUnpackedSize(compressedSize);
  
  // Write the texture to a temporary file
  writer.AppendContent((unsigned char*) compressed, compressedSize);
  
  delete [] compressed;
  
  SDL_FreeSurface(rgbaImage);    
    
  // Update the header
  frame.SetWidth(image->w);
  frame.SetHeight(image->h);
  frame.SetX(0);
  frame.SetY(0);
  frame.SetDuration(0);
  
  return frame;
}

void Usage()
{
  puts("Usage:");
  puts("  -help            Show this screen.");
  puts("  -input <dir>     Input directory. Default: current dir");
  puts("  -output <dir>    Output directory/filename. Default: Textures.xpr");
}

int createBundle(const std::string& InputDir, const std::string& OutputFile)
{
  CXBTF xbtf;
  CreateSkeletonHeader(xbtf, InputDir);
    
  CXBTFWriter writer(xbtf, OutputFile);
  if (!writer.Create())
  {
    printf("Error creating file\n");
    return 1;
  }
  
  std::vector<CXBTFFile>& files = xbtf.GetFiles();
  for (size_t i = 0; i < files.size(); i++)
  {
    CXBTFFile& file = files[i];
    
    int format = file.GetFormat();
    
    printf("Converting %s to DXT5 ", file.GetPath());
    
    std::string fullPath = InputDir;
    fullPath += file.GetPath();
    
    if (!IsGIF(fullPath.c_str()))
    {          
      // Load the image
      SDL_Surface* image = IMG_Load(fullPath.c_str());
      if (!image)
      {
        printf("...unable to load image\n");
        continue;
      }
      
      CXBTFFrame frame = createXBTFFrame(image, writer, format);
          
      file.SetLoop(0);    
      file.GetFrames().push_back(frame); 
      
      printf("(%dx%d)\n", image->w, image->h);
      
      SDL_FreeSurface(image);        
    }
    else
    {
      int gnAG = AG_LoadGIF(fullPath.c_str(), NULL, 0);  
      AG_Frame* gpAG = new AG_Frame[gnAG];
      AG_LoadGIF(fullPath.c_str(), gpAG, gnAG);
      
      printf("(%dx%d) -- %d frames\n", gpAG[0].surface->w, gpAG[0].surface->h, gnAG);
      
      for (int j = 0; j < gnAG; j++)
      {
        CXBTFFrame frame = createXBTFFrame(gpAG[j].surface, writer, format);
        frame.SetX(gpAG[j].x);
        frame.SetX(gpAG[j].y);
        frame.SetDuration(gpAG[j].delay);
        file.GetFrames().push_back(frame);  
      }

      AG_FreeSurfaces(gpAG, gnAG);
      delete [] gpAG;
      
      file.SetLoop(0);          
    }      
  }
    
  if (!writer.UpdateHeader())
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
  bool valid = false;  
  CmdLineArgs args(argc, (const char**)argv);

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
    else if (!stricmp(args[i], "-output") || !stricmp(args[i], "-o"))
    {
      OutputFilename = args[++i];
      valid = true;
#ifdef _LINUX
      char *c = NULL;
      while ((c = (char *)strchr(OutputFilename.c_str(), '\\')) != NULL) *c = '/';
#endif
    }
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
  if (pos != InputDir.length()-1) 
  {
    InputDir += DIR_SEPARATOR;
  }
  
  createBundle(InputDir, OutputFilename);
}
