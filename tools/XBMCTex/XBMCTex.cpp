/*
 *      Copyright (C) 2004-2009 Team XBMC
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

// XBMCTex.cpp : Defines the entry point for the console application.
//

#include "AnimatedGif.h"
#include "Bundler.h"
#include <stdio.h>
#include <algorithm>
#include "cmdlineargs.h"
#include "Surface.h"
#include "EndianSwap.h"

#ifdef _LINUX
#ifndef __APPLE__
#include <linux/limits.h>
#endif
#include <string.h>
#include "XFileUtils.h"
#include "PlatformDefs.h"
#include "xwinapi.h"
#define WIN32_FIND_DATAA WIN32_FIND_DATA
#else
#define XBMC_FILE_SEP '\\'
#endif

// Debug macros
#if defined(_DEBUG) && defined(_MSC_VER) 
#include <crtdbg.h>
#define TRACE0(f)                 _RPT0(_CRT_WARN, f)
#define TRACE1(f, a)              _RPT1(_CRT_WARN, f, a)
#define TRACE2(f, a, b)           _RPT2(_CRT_WARN, f, a, b)
#define TRACE3(f, a, b, c)        _RPT3(_CRT_WARN, f, a, b, c)
#define TRACE4(f, a, b, c, d)     _RPT4(_CRT_WARN, f, a, b, c, d)
#define TRACE5(f, a, b, c, d, e)  _RPT_BASE((_CRT_WARN, NULL, 0, NULL, f, a, b, c, d, e))

#else

#define TRACE0(f)
#define TRACE1(f, a)
#define TRACE2(f, a, b)
#define TRACE3(f, a, b, c)
#define TRACE4(f, a, b, c, d)
#define TRACE5(f, a, b, c, d, e)
#endif

extern "C" void SHA1(const BYTE* buf, DWORD len, BYTE hash[20]);

CBundler Bundler;

UINT UncompressedSize;
UINT CompressedSize;
UINT TotalSrcPixels;
UINT TotalDstPixels;

#pragma pack(push,1)
struct RGBCOLOUR
{
	BYTE b;
	BYTE g;
	BYTE r;
	BYTE a;
};
#pragma pack(pop)

void PrintImageInfo(const CSurface::ImageInfo& info)
{
	printf("%4dx%-4d ", info.width, info.height);
	fputs("->", stdout);
}

void PrintAnimInfo(const CAnimatedGifSet& Anim)
{
	printf("%4dx%-4d (%5df)->", Anim.FrameWidth, Anim.FrameHeight, Anim.GetImageCount());
}

#define CheckHR(hr) if (FAILED(hr)) { printf("ERROR: %08x\n", hr); if (pCompSurf) pCompSurf->Release(); if (pDstSurf) pDstSurf->Release(); return false; }

struct XPRFile_t
{
  // the following are pointers into our headerBuf
	DWORD* flags;
	struct AnimInfo_t {
		DWORD nLoops;
		DWORD RealSize;
	} *AnimInfo;
	D3DPalette* D3DPal;
	struct Texture_t {
		D3DTexture D3DTex;
		DWORD RealSize;
	} *Texture;

  int nImages;
};

BYTE *headerBuf = NULL;
DWORD headerSize = 0;

BYTE *imageData = NULL;
DWORD imageSize = 0;

static XPRFile_t XPRFile;

enum XPR_FLAGS
{
	XPRFLAG_PALETTE = 0x00000001,
	XPRFLAG_ANIM =    0x00000002
};

#undef CheckHR

void CommitXPR(const char* Filename)
{
	if (!XPRFile.nImages)
		return;
        
	// Conversion for big-endian systems
	// flags is used/updated in different places
	// so swap it only before to call AddFile
	*XPRFile.flags = Endian_SwapLE32(*XPRFile.flags);

	const void* Buffers[2] = { headerBuf, imageData };
	DWORD Sizes[2] = { headerSize, imageSize };
	if (!Bundler.AddFile(Filename, 2, Buffers, Sizes))
		printf("ERROR: Unable to compress data (out of memory?)\n");

  // free our image memory
  free(imageData);
  imageData = NULL;
  imageSize = 0;
}

void WriteXPRHeader(DWORD* pal, int nImages, DWORD nLoops = 0)
{
  // compute how large our header requires
  headerSize = sizeof(DWORD);
  if (nImages > 1) // need AnimInfo header
    headerSize += sizeof(XPRFile_t::AnimInfo_t);
  if (pal) // need D3DPal header
    headerSize += sizeof(D3DPalette);
  headerSize += nImages * sizeof(XPRFile_t::Texture_t);

  // align to 128 byte boundary
	headerSize = (headerSize + 127) & ~127;

  // allocate space for our header
  headerBuf = (BYTE *)realloc(headerBuf, headerSize);
  memset(headerBuf, 0, headerSize);

  // setup our header
  unsigned int offset = 0;
  XPRFile.flags = (DWORD *)&headerBuf[offset];
  offset += sizeof(DWORD);
  if (nImages > 1)
  {
    XPRFile.AnimInfo = (XPRFile_t::AnimInfo_t *)&headerBuf[offset];
    offset += sizeof(XPRFile_t::AnimInfo_t);
  }
  else
    XPRFile.AnimInfo = NULL;
  if (pal)
  {
    XPRFile.D3DPal = (D3DPalette *)&headerBuf[offset];
    offset += sizeof(D3DPalette);
  }
  else
    XPRFile.D3DPal = NULL;
	XPRFile.Texture = (XPRFile_t::Texture_t *)&headerBuf[offset];
	XPRFile.nImages = 0;

  *XPRFile.flags = nImages << 16;

	if (nImages > 1)
	{
		*XPRFile.flags |= XPRFLAG_ANIM;
		XPRFile.AnimInfo->nLoops = nLoops;
	}

	if (pal)
	{
		// commit memory for palette
    imageData = (BYTE*)realloc(imageData, 1024);

		*XPRFile.flags |= XPRFLAG_PALETTE;
		XPRFile.D3DPal->Common = Endian_SwapLE32(1 | (3 << 16));
		XPRFile.D3DPal->Data = 0;
		XPRFile.D3DPal->Lock = 0;
		memcpy(imageData, pal, 1024);
		imageSize += 1024;
	}
}

void AppendXPRImage(CSurface &surface, XB_D3DFORMAT fmt)
{
	UINT Size = ((surface.Pitch() * surface.Height()) + 127) & ~127; // must be 128-byte aligned for any following images

  // reallocate enough data for our image
  imageData = (BYTE*)realloc(imageData, imageSize + Size);
  memset(imageData + imageSize, 0, Size);

  CSurfaceRect rect;
  if (!surface.Lock(&rect))
    return;

	if (IsSwizzledFormat(fmt))
	{
		// Swizzle for xbox
		SwizzleRect(rect.pBits, 0, imageData + imageSize, surface.Width(), surface.Height(), surface.BPP());
	}
	else
	{
		// copy
		BYTE* src = rect.pBits;
		BYTE* dst = imageData + imageSize;
		for (UINT y = 0; y < surface.Height(); ++y)
		{
			memcpy(dst, src, surface.Pitch());
			src += rect.Pitch;
			dst += surface.Pitch();
		}
	}

	surface.Unlock();

	SetTextureHeader(surface.Width(), surface.Height(), 1, 0, fmt, 
		&XPRFile.Texture[XPRFile.nImages].D3DTex, imageSize, surface.Pitch());
	if (!(*XPRFile.flags & XPRFLAG_ANIM))
                XPRFile.Texture[XPRFile.nImages].RealSize = Endian_SwapLE32((surface.Info().width & 0xffff) | ((surface.Info().height & 0xffff) << 16));
	++XPRFile.nImages;

	imageSize += Size;
	CompressedSize += Size;
}

void AppendXPRImageLink(int iLinkedImage)
{
	memcpy(&XPRFile.Texture[XPRFile.nImages].D3DTex, &XPRFile.Texture[iLinkedImage].D3DTex, sizeof(D3DTexture));
	++XPRFile.nImages;
}

void WriteXPR(const char* Filename, CSurface &surface, XB_D3DFORMAT fmt, DWORD* pal)
{
	WriteXPRHeader(pal, 1);
	AppendXPRImage(surface, fmt);
	CommitXPR(Filename);
}

#undef CheckHR
#define CheckHR(hr) if (FAILED(hr)) { printf("ERROR: %08x\n", hr); if (pDstSurf) pDstSurf->Release(); return false; }

// Converts to P8 format is colours <= 256
bool ConvertP8(CSurface &source, CSurface &dest, DWORD* pal)
{
  // note: This routine assumes the source is 32 bpp
  if (source.BPP() != 4)
  {
    printf("ERROR: ConvertP8 called on a source that's not 32bpp\n");
    return false;
  }

	// convert to p8
  if (!dest.Create(source.Info().width, source.Info().height, CSurface::FMT_PALETTED))
    return false;

  CSurfaceRect sr, dr;
  if (!dest.Lock(&dr) || !source.Lock(&sr))
    return false;

	DWORD* src = (DWORD*)sr.pBits;
	BYTE* dst = (BYTE*)dr.pBits;
	int n = 0, i;
	for (UINT y = 0; y < source.Info().height; ++y)
	{
		for (UINT x = 0; x < source.Info().width; ++x)
		{
      for (i = 0; i < n; ++i)
			{
				if (pal[i] == *src)
					break;
			}
			if (i == n)
			{
				if (n >= 256)
				{
					TRACE0(" Too many colours for P8\n");
					source.Unlock();
          dest.Unlock();
					return false;
				}
				pal[n++] = *src;
			}
			*dst++ = i;
			++src;
		}
		for (UINT x = source.Info().width; x < dest.Width(); ++x)
		{
			*dst++ = 0; // we don't care about the colour outside of our real image
			++src;
    }
	}
  for (UINT y = source.Info().height; y < dest.Height(); ++y)
  {
		for (UINT x = 0; x < dest.Width(); ++x)
		{
			*dst++ = 0; // we don't care about the colour outside of our real image
			++src;
    }
  }
  for (int i = n; i < 256; i++)
    pal[i] = 0;

	TRACE1(" Colours Used: %d\n", n);

	dest.Unlock();
	source.Unlock();

	return true;
}

// Converts any fully transparent pixels to transparent black to make textures better compressable
void FixTransparency(CSurface &surface)
{
  CSurfaceRect rect;

  if (!surface.Lock(&rect))
    return;

	DWORD* pix = (DWORD*)rect.pBits;
	for (UINT y = 0; y < surface.Width(); ++y)
	{
		for (UINT x = 0; x < surface.Height(); ++x)
		{
			if (!(*pix & 0xff000000))
				*pix = 0;
			++pix;
		}
	}

  surface.Unlock();
}


#undef CheckHR
#define CheckHR(hr) if (FAILED(hr)) { printf("ERROR: %08x\n", hr); return; }

void ConvertFile(const char* Dir, const char* Filename)
{
  CSurface surface;
	char OutFilename[52];
	if (Dir)
		_snprintf(OutFilename, 52, "%s\\%s", Dir, Filename);
	else
		_snprintf(OutFilename, 52, "%s", Filename);
	OutFilename[51] = 0;

	printf("%s: ", OutFilename);
	TRACE1("%s:\n", OutFilename);
	int n = strlen(OutFilename);
	if (n < 40)
		printf("%*c", 40-n, ' ');

  CSurface srcSurface;
  if (!srcSurface.CreateFromFile(Filename, CSurface::FMT_ARGB))
  {
    printf("Error creating surface size %u by %u\n", srcSurface.Width(), srcSurface.Height());
    return;
  }

  // fix up the transparency (allows better compression)
	FixTransparency(srcSurface);

	// Use a paletted texture if possible as it's lossless + only 4 bytes per pixel (guaranteed smaller)
#ifdef _XBOX
	CSurface tempSurface;
	DWORD pal[256];
  if (ConvertP8(srcSurface, tempSurface, pal))
  {
	  float Waste = 100.f * (float)(srcSurface.Width() * srcSurface.Height() - srcSurface.Info().width * srcSurface.Info().height) / (float)(srcSurface.Width() * srcSurface.Height());
		printf("P8       %4dx%-4d (%5.2f%% waste)\n", srcSurface.Width(), srcSurface.Height(), Waste);
		TRACE0(" Selected Format: P8\n");

		WriteXPR(OutFilename, tempSurface, XB_D3DFMT_P8, pal);
		return;
  }
#endif
  // we are going to use a 32bit texture, so work out what type to use
  // test linear format versus non-linear format
  // Linear format requires 64 pixel aligned width, whereas
  // Non-linear format requires power of 2 width and height
  bool useLinearFormat(false);
#ifdef _XBOX
  UINT linearWidth = (srcSurface.Info().width + 0x3f) & ~0x3f;
  if (linearWidth * srcSurface.Info().height < srcSurface.Width() * srcSurface.Height())
#endif
    useLinearFormat = true;

	// Use A8R8G8B8
  if (useLinearFormat)
  {
    // create the linear version as well
    // correct sizing information
    UncompressedSize += srcSurface.Width() * srcSurface.Height() * 4;
	  TotalSrcPixels += srcSurface.Info().width * srcSurface.Info().height;
    TotalDstPixels += srcSurface.Width() * srcSurface.Height();
	  float Waste = 100.f * (float)(srcSurface.Width() - srcSurface.Info().width) / (float)(srcSurface.Width());

    CSurface linearSurface;
    if (!linearSurface.CreateFromFile(Filename, CSurface::FMT_LIN_ARGB))
      return;
   
    printf("LIN_A8R8G8B8 %4dx%-4d (%5.2f%% waste)\n", srcSurface.Width(), srcSurface.Height(), Waste);
		TRACE0(" Selected Format: LIN_A8R8G8B8\n");
    WriteXPR(OutFilename, linearSurface, XB_D3DFMT_LIN_A8R8G8B8, NULL);
  }
  else
  {
    UncompressedSize += srcSurface.Width() * srcSurface.Height() * 4;
	  TotalSrcPixels += srcSurface.Info().width * srcSurface.Info().height;
    TotalDstPixels += srcSurface.Width() * srcSurface.Height();

	  float Waste = 100.f * (float)(srcSurface.Width() * srcSurface.Height() - srcSurface.Info().width * srcSurface.Info().height) / (float)(srcSurface.Width() * srcSurface.Height());
		printf("A8R8G8B8 %4dx%-4d (%5.2f%% waste)\n", srcSurface.Width(), srcSurface.Height(), Waste);
		TRACE0(" Selected Format: A8R8G8B8\n");
    WriteXPR(OutFilename, srcSurface, XB_D3DFMT_A8R8G8B8, NULL);
  }
}

// only works for gifs or other 256-colour anims
void ConvertAnim(const char* Dir, const char* Filename)
{
	char OutFilename[52];
	if (Dir)
		_snprintf(OutFilename, 52, "%s\\%s", Dir, Filename);
	else
		_snprintf(OutFilename, 52, "%s", Filename);
	OutFilename[51] = 0;

	printf("%s: ", OutFilename);
	TRACE1("%s:\n", OutFilename);
	int n = strlen(OutFilename);
	if (n < 40)
		printf("%*c", 40-n, ' ');

	// Load up the file
	CAnimatedGifSet Anim;
	int nImages = Anim.LoadGIF(Filename);
	if (!nImages)
	{
		puts("ERROR: Unable to load gif (file corrupt?)");
		return;
	}
	if (nImages > 65535)
	{
		printf("ERROR: Too many frames in gif (%d > 65535)\n", nImages);
		return;
	}

	PrintAnimInfo(Anim);

	UINT Width = PadPow2(Anim.FrameWidth);
	UINT Height = PadPow2(Anim.FrameHeight);

	PALETTEENTRY pal[256];
	memcpy(pal, Anim.m_vecimg[0]->Palette, 256 * sizeof(PALETTEENTRY));
	for (int i = 0; i < 256; i++)
		pal[i].peFlags = 0xff; // alpha
	if (Anim.m_vecimg[0]->Transparency && Anim.m_vecimg[0]->Transparent >= 0)
		memset(&pal[Anim.m_vecimg[0]->Transparent], 0, sizeof(PALETTEENTRY));

	// setup xpr header
	WriteXPRHeader((DWORD*)pal, nImages);
	if (nImages > 1)
	{
		XPRFile.AnimInfo->RealSize = Endian_SwapLE32((Anim.FrameWidth & 0xffff) | ((Anim.FrameHeight & 0xffff) << 16));
		XPRFile.AnimInfo->nLoops = Endian_SwapLE32(Anim.nLoops);
	}

	int nActualImages = 0;

	TotalSrcPixels += Anim.FrameWidth * Anim.FrameHeight * nImages;
	TotalDstPixels += Width * Height * nImages;
	float Waste = 100.f * (float)(Width * Height - Anim.FrameWidth * Anim.FrameHeight) / (float)(Width * Height);

	// alloc hash buffer
	BYTE (*HashBuf)[20] = new BYTE[nImages][20];

	for (int i = 0; i < nImages; ++i)
	{
		printf("%3d%%\b\b\b\b", 100 * i / nImages);

		UncompressedSize += Width * Height;
		CAnimatedGif* pGif = Anim.m_vecimg[i];

		if (nImages > 1)
			XPRFile.Texture[i].RealSize = Endian_SwapLE32(pGif->Delay);

		// generate sha1 hash
		SHA1((BYTE*)pGif->Raster, pGif->BytesPerRow * pGif->Height, HashBuf[i]);

		// duplicate scan
		int j;
		for (j = 0; j < i; ++j)
		{
			if (!memcmp(HashBuf[j], HashBuf[i], 20))
			{
				// duplicate image!
				TRACE2(" %03d: Duplicate of %03d\n", i, j);
				AppendXPRImageLink(j);
				break;
			}
		}
		if (j < i)
			continue;

		++nActualImages;

    // P8 for animgifs
    CSurface surface;
    if (!surface.Create(Anim.FrameWidth, Anim.FrameHeight, CSurface::FMT_PALETTED))
      return;

    CSurfaceRect rect;
    if (!surface.Lock(&rect))
      return;

		memcpy(rect.pBits, pGif->Raster, pGif->Height * rect.Pitch);
		memset(rect.pBits + pGif->Height * rect.Pitch, pGif->Transparent, (Height - pGif->Height) * rect.Pitch);

    surface.Unlock();

		TRACE1(" %03d: Selected Format: P8\n", i);
		AppendXPRImage(surface, XB_D3DFMT_P8);
	}

	delete [] HashBuf;
	
	printf("(%5df) %4dx%-4d (%5.2f%% waste)\n", nActualImages, Width, Height, Waste);

	CommitXPR(OutFilename);
}

// returns true for png, bmp, tga, jpg and dds files, otherwise returns false
bool IsGraphicsFile(char *strFileName)
{
	int n = (int)strlen(strFileName);
	if (n<4)
		return false;
	if (strnicmp(&strFileName[n-4], ".png", 4) &&
		strnicmp(&strFileName[n-4], ".bmp", 4) &&
		strnicmp(&strFileName[n-4], ".tga", 4) &&
		strnicmp(&strFileName[n-4], ".jpg", 4))
		return false;
	return true;
}

// returns true if it's a ".gif" otherwise returns false
bool IsGraphicsAnim(char *strFileName)
{
	int n = (int)strlen(strFileName);
	if (n<4 || strnicmp(&strFileName[n-4], ".gif", 4))
		return false;
	return true;
}

void ConvertDirectory(const char *strFullPath, char *strRelativePath)
{
	// Set our current directory
	if (strFullPath)
		SetCurrentDirectory(strFullPath);
	// Get our current pathname
	char strCurrentPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, strCurrentPath);

	// Now run through our directory, and find all subdirs
	WIN32_FIND_DATAA FindData;
	char Filename[4] = "*.*";
	HANDLE hFind = FindFirstFile(Filename, &FindData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Check if we've found a subdir
			if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// ignore any directory starting with a '.'
				if (strnicmp(FindData.cFileName,".",1))
				{
					char strNewFullPath[MAX_PATH];
					char strNewRelativePath[MAX_PATH];
					sprintf(strNewFullPath, "%s%c%s", strCurrentPath, XBMC_FILE_SEP, FindData.cFileName);
					if (strRelativePath)
						sprintf(strNewRelativePath, "%s%c%s", strRelativePath, XBMC_FILE_SEP, FindData.cFileName);
					else
						sprintf(strNewRelativePath, "%s", FindData.cFileName);
					// Recurse into the new directory
					ConvertDirectory(strNewFullPath, strNewRelativePath);
					// Restore our current directory
					SetCurrentDirectory(strCurrentPath);
				}
			}
			else
			{	// just files - check if it's an allowed graphics file
				if (IsGraphicsFile(FindData.cFileName))
				{	// got a graphics file
					ConvertFile(strRelativePath,FindData.cFileName);
				}
				if (IsGraphicsAnim(FindData.cFileName))
				{	// got a .gif anim
					ConvertAnim(strRelativePath,FindData.cFileName);
				}
			}
		}
		while (FindNextFile(hFind, &FindData));
		FindClose(hFind);
	}
}

void Usage()
{
	puts("Usage:");
	puts("  -help            Show this screen.");
	puts("  -input <dir>     Input directory. Default: current dir");
	puts("  -output <dir>    Output directory/filename. Default: Textures.xpr");
	puts("  -quality <qual>  Quality setting (min, low, normal, high, max). Default: normal");
  puts("  -noprotect       XPR contents viewable at full quality in skin editor");
}

int main(int argc, char* argv[])
{
	int NoProtect = 0;
	bool valid = false;

	CmdLineArgs args(argc, (const char**)argv);

	if (args.size() == 1)
	{
		Usage();
		return 1;
	}

	const char* InputDir = NULL;
	const char* OutputFilename = "Textures.xpr";

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
      while ((c = (char *)strchr(OutputFilename, '\\')) != NULL) *c = '/';
#endif
		}
    else if (!stricmp(args[i], "-noprotect") || !stricmp(args[i], "-p"))
    {
      NoProtect = 1;
			valid = true;
    }
		else
		{
			printf("Unrecognised command line flag: %s\n", args[i]);
		}
	}

	if (!valid)
	{
		Usage();
		return 1;
	}

	// Initialize the graphics device
  if (!g_device.Create())
    return 1;

	char HomeDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, HomeDir);

	Bundler.StartBundle();

	// Scan the input directory (or current dir if false) for media files
	ConvertDirectory(InputDir, NULL);

  free(headerBuf);

	SetCurrentDirectory(HomeDir);
	DWORD attr = GetFileAttributes(OutputFilename);
	if (attr != (DWORD)-1 && (attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		SetCurrentDirectory(OutputFilename);
		OutputFilename = "Textures.xpr";
	}

	printf("\nWriting bundle: %s", OutputFilename);
  int BundleSize = Bundler.WriteBundle(OutputFilename, NoProtect);
	if (BundleSize == -1)
	{
		printf("\nERROR: %08x\n", GetLastError());
		return 1;
	}

	printf("\nUncompressed texture size: %6dkB\nCompressed texture size: %8dkB\nBundle size:             %8dkB\n\nWasted Pixels: %u/%u (%5.2f%%)\n",
		(UncompressedSize + 1023) / 1024, (((CompressedSize + 1023) / 1024) + 3) & ~3, (BundleSize + 1023) / 1024,
		TotalDstPixels - TotalSrcPixels, TotalDstPixels, 100.f * (float)(TotalDstPixels - TotalSrcPixels) / (float)TotalDstPixels);

	return 0;
}

