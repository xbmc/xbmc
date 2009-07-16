/*
*      Copyright (C) 2005-2008 Team XBMC
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

#include "include.h"
#include "TextureManagerSDL.h"
#include "AnimatedGif.h"
#include "GraphicContext.h"
#include "Surface.h"
#include "../xbmc/Picture.h"
#include "utils/SingleLock.h"
#include "StringUtils.h"
#include "utils/CharsetConverter.h"
#include "../xbmc/Util.h"
#include "../xbmc/FileSystem/File.h"
#include "../xbmc/FileSystem/Directory.h"
#include "../xbmc/FileSystem/SpecialProtocol.h"

#ifdef HAS_SDL
#define MAX_PICTURE_WIDTH  2048
#define MAX_PICTURE_HEIGHT 2048
#endif

using namespace std;

extern "C" void dllprintf( const char *format, ... );

DWORD PadPow2(DWORD x) 
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return ++x;
}

CGUITextureManagerGL g_TextureManager;

/************************************************************************/
/*    CSDLTexture                                                       */
/************************************************************************/
CGLTexture::CGLTexture(void* surface, bool load, bool freeSurface) 
: CBaseTexture(surface, load, freeSurface)
{
	Update(surface, load, freeSurface);
}


CGLTexture::~CGLTexture()
{
	g_graphicsContext.BeginPaint();
	if (glIsTexture(id)) {
		glDeleteTextures(1, &((GLuint)id));
	}
	g_graphicsContext.EndPaint();

	if (m_pixels)
		delete [] m_pixels;

	m_pixels=NULL;

	id = 0;
}

void CGLTexture::LoadToGPU()
{
	if (!m_pixels) {
		// nothing to load - probably same image (no change)
		return;
	}

	g_graphicsContext.BeginPaint();
	if (!m_loadedToGPU) {
		// Have OpenGL generate a texture object handle for us
		// this happens only one time - the first time the texture is loaded
		glGenTextures(1, &id);
	}

	// Bind the texture object
	glBindTexture(GL_TEXTURE_2D, id);

	// Set the texture's stretching properties
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	static GLint maxSize = g_graphicsContext.GetMaxTextureSize();
	{
		if (textureHeight>maxSize)
		{
			CLog::Log(LOGERROR, "GL: Image height %d too big to fit into single texture unit, truncating to %d", textureHeight, (int) maxSize);
			textureHeight = maxSize;
		}
		if (textureWidth>maxSize)
		{
			CLog::Log(LOGERROR, "GL: Image width %d too big to fit into single texture unit, truncating to %d", textureWidth, (int) maxSize);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, textureWidth);
			textureWidth = maxSize;
		}
	}
	//CLog::Log(LOGNOTICE, "Texture width x height: %d x %d", textureWidth, textureHeight);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, textureWidth, textureHeight, 0,
		GL_BGRA, GL_UNSIGNED_BYTE, m_pixels);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	VerifyGLState();

	g_graphicsContext.EndPaint();
	delete [] m_pixels;
	m_pixels = NULL;

	m_loadedToGPU = true;           
}

void CGLTexture::Update(int w, int h, int pitch, const unsigned char *pixels, bool loadToGPU) 
{
	static int vmaj=0;
	int vmin,tpitch;

	if (m_pixels)
		delete [] m_pixels;

	imageWidth = w;
	imageHeight = h;


	if ((vmaj==0) && g_graphicsContext.getScreenSurface())
	{
		g_graphicsContext.getScreenSurface()->GetGLVersion(vmaj, vmin);    
	}
	if (vmaj>=2 && GLEW_ARB_texture_non_power_of_two)
	{
		textureWidth = imageWidth;
		textureHeight = imageHeight;
	}
	else
	{
		textureWidth = PadPow2(imageWidth);
		textureHeight = PadPow2(imageHeight);
	}

	// Resize texture to POT
	const unsigned char *src = pixels;
	tpitch = min(pitch,textureWidth*4);
	m_pixels = new unsigned char[textureWidth * textureHeight * 4];
	unsigned char* resized = m_pixels;

	for (int y = 0; y < h; y++)
	{
		memcpy(resized, src, tpitch); // make sure pitch is not bigger than our width
		src += pitch;

		// repeat last column to simulate clamp_to_edge
		for(int i = tpitch; i < textureWidth*4; i+=4)
			memcpy(resized+i, src-4, 4);

		resized += (textureWidth * 4);
	}

	// repeat last row to simulate clamp_to_edge
	for(int y = h; y < textureHeight; y++) 
	{
		memcpy(resized, src - tpitch, tpitch);

		// repeat last column to simulate clamp_to_edge
		for(int i = tpitch; i < textureWidth*4; i+=4) 
			memcpy(resized+i, src-4, 4);

		resized += (textureWidth * 4);
	}
	if (loadToGPU)
		LoadToGPU();
}

void CGLTexture::Update(void *surface, bool loadToGPU, bool freeSurface) 
{
	SDL_Surface* sdl_surface = (SDL_Surface *)surface;

	SDL_LockSurface(sdl_surface);
	Update(sdl_surface->w, sdl_surface->h, sdl_surface->pitch, (unsigned char *)sdl_surface->pixels, loadToGPU);
	SDL_UnlockSurface(sdl_surface);

	if (freeSurface)
		SDL_FreeSurface(sdl_surface);
}

/************************************************************************/
/*    CTextureArraySDL                                                  */
/************************************************************************/
CTextureArrayGL::CTextureArrayGL()
{
	Reset();
};

CTextureArrayGL::CTextureArrayGL(int width, int height, int loops, bool texCoordsArePixels)
: CTextureArray(width, height, loops, texCoordsArePixels)
{
};

CTextureArrayGL::~CTextureArrayGL()
{

}

void CTextureArrayGL::Add(void *texture, int delay)
{
	if (!texture)
		return;

	m_textures.push_back(texture);
	m_delays.push_back(delay ? delay * 2 : 100);

	CGLTexture* SDLTexture = (CGLTexture *)texture;

	m_texWidth = SDLTexture->textureWidth;
	m_texHeight = SDLTexture->textureHeight;
	m_texCoordsArePixels = false;
}

void CTextureArrayGL::Set(void *texture, int width, int height)
{
	assert(!m_textures.size()); // don't try and set a texture if we already have one!
	m_width = width;
	m_height = height;
	Add(texture, 100);
}

void CTextureArrayGL::Free()
{
	CSingleLock lock(g_graphicsContext);
	for (unsigned int i = 0; i < m_textures.size(); i++)
	{
		delete m_textures[i];
	}

	m_textures.clear();
	m_delays.clear();

	Reset();
}



CTextureMapGL::CTextureMapGL(const CStdString& textureName, int width, int height, int loops)
: CTextureMap(textureName, width, height, loops)
{
	m_texture = new CTextureArrayGL(width, height, loops);
}

CTextureMapGL::~CTextureMapGL()
{
	FreeTexture();
}


void CTextureMapGL::Add(void* pTexture, int delay)
{
	CGLTexture *glTexture = new CGLTexture(pTexture, false);
	m_texture->Add(glTexture, delay);

	if (glTexture)
		m_memUsage += sizeof(CGLTexture) + (glTexture->textureWidth * glTexture->textureHeight * 4); 
}

/************************************************************************/
/*    CGUITextureManagerSDL                                             */
/************************************************************************/
CGUITextureManagerGL::CGUITextureManagerGL(void) : CGUITextureManager()
{
#if defined(_WIN32)
	// Hack for SDL library that keeps loading and unloading these
	LoadLibraryEx("zlib1.dll", NULL, 0);
	LoadLibraryEx("libpng12-0.dll", NULL, 0);
	LoadLibraryEx("jpeg.dll", NULL, 0);
#endif
}

CGUITextureManagerGL::~CGUITextureManagerGL(void)
{
	Cleanup();
}

int CGUITextureManagerGL::Load(const CStdString& strTextureName, bool checkBundleOnly /*= false */)
{
	CStdString strPath;
	int bundle = -1;
	int size = 0;
	if (!HasTexture(strTextureName, &strPath, &bundle, &size))
		return 0;

	if (size) // we found the texture
		return size;

	if (checkBundleOnly && bundle == -1)
		return 0;

	//Lock here, we will do stuff that could break rendering
	CSingleLock lock(g_graphicsContext);

	XBMC::TexturePtr pTexture;
	XBMC::PalettePtr pPal = NULL;

#ifdef _DEBUG
	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
#endif

	D3DXIMAGE_INFO info;

	if (strPath.Right(4).ToLower() == ".gif")
	{
		CTextureMapGL* pMap;

		if (bundle >= 0)
		{

			XBMC::TexturePtr *pTextures;
			int nLoops = 0;
			int* Delay;
			int nImages = m_TexBundle[bundle].LoadAnim(strTextureName, &info, &pTextures, &pPal, nLoops, &Delay);
			if (!nImages)
			{
				CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
				return 0;
			}

			pMap = new CTextureMapGL(strTextureName, info.Width, info.Height, nLoops);
			for (int iImage = 0; iImage < nImages; ++iImage)
			{
				pMap->Add(pTextures[iImage], Delay[iImage]);
				SDL_FreeSurface(pTextures[iImage]);
			}

			delete [] pTextures;
			delete [] Delay;
		}
		else
		{
			CAnimatedGifSet AnimatedGifSet;
			int iImages = AnimatedGifSet.LoadGIF(strPath.c_str());
			if (iImages == 0)
			{
				if (!strnicmp(strPath.c_str(), "special://home/skin/", 20) && !strnicmp(strPath.c_str(), "special://xbmc/skin/", 20))
					CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
				return 0;
			}
			int iWidth = AnimatedGifSet.FrameWidth;
			int iHeight = AnimatedGifSet.FrameHeight;

			int iPaletteSize = (1 << AnimatedGifSet.m_vecimg[0]->BPP);
			pMap = new CTextureMapGL(strTextureName, iWidth, iHeight, AnimatedGifSet.nLoops);

			for (int iImage = 0; iImage < iImages; iImage++)
			{
				int w = iWidth;
				int h = iHeight;

				pTexture = SDL_CreateRGBSurface(SDL_HWSURFACE, w, h, 32, RMASK, GMASK, BMASK, AMASK);
				if (pTexture)

				{
					CAnimatedGif* pImage = AnimatedGifSet.m_vecimg[iImage];

					if (SDL_LockSurface(pTexture) != -1)          
					{
						COLOR *palette = AnimatedGifSet.m_vecimg[0]->Palette;
						// set the alpha values to fully opaque
						for (int i = 0; i < iPaletteSize; i++)
							palette[i].x = 0xff;
						// and set the transparent colour
						if (AnimatedGifSet.m_vecimg[0]->Transparency && AnimatedGifSet.m_vecimg[0]->Transparent >= 0)
							palette[AnimatedGifSet.m_vecimg[0]->Transparent].x = 0;

						for (int y = 0; y < pImage->Height; y++)
						{

							BYTE *dest = (BYTE *)pTexture->pixels + (y * w * 4);
							BYTE *source = (BYTE *)pImage->Raster + y * pImage->BytesPerRow;
							for (int x = 0; x < pImage->Width; x++)
							{
								COLOR col = palette[*source++];
								*dest++ = col.b;
								*dest++ = col.g;
								*dest++ = col.r;
								*dest++ = col.x;
							}
						}

						SDL_UnlockSurface(pTexture);
						pMap->Add(pTexture, pImage->Delay);
						SDL_FreeSurface(pTexture);
					}
				}
			} // of for (int iImage=0; iImage < iImages; iImage++)
		}

#ifdef _DEBUG
		LARGE_INTEGER end, freq;
		QueryPerformanceCounter(&end);
		QueryPerformanceFrequency(&freq);
		char temp[200];
		sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, (bundle >= 0) ? " (bundled)" : "");
		OutputDebugString(temp);
#endif

		m_vecTextures.push_back(pMap);
		return 1;
	} // of if (strPath.Right(4).ToLower()==".gif")

	if (bundle >= 0)
	{
		if (FAILED(m_TexBundle[bundle].LoadTexture(strTextureName, &info, &pTexture, &pPal)))  
		{
			CLog::Log(LOGERROR, "Texture manager unable to load bundled file: %s", strTextureName.c_str());
			return 0;
		}
	}
	else
	{
		// normal picture
		// convert from utf8
		CStdString texturePath;
		g_charsetConverter.utf8ToStringCharset(strPath, texturePath);

		CPicture pic;
		SDL_Surface *original = pic.Load(texturePath, MAX_PICTURE_WIDTH, MAX_PICTURE_HEIGHT);
		if (!original)
		{
			CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
			return 0;
		}
		// make sure the texture format is correct
		SDL_PixelFormat format;
		format.palette = 0; format.colorkey = 0; format.alpha = 0;
		format.BitsPerPixel = 32; format.BytesPerPixel = 4;
		format.Amask = AMASK; format.Ashift = PIXEL_ASHIFT;
		format.Rmask = RMASK; format.Rshift = PIXEL_RSHIFT;
		format.Gmask = GMASK; format.Gshift = PIXEL_GSHIFT;
		format.Bmask = BMASK; format.Bshift = PIXEL_BSHIFT;
		pTexture = SDL_ConvertSurface(original, &format, SDL_SWSURFACE);

		SDL_FreeSurface(original);
		if (!pTexture)
		{
			CLog::Log(LOGERROR, "Texture manager unable to load file: %s", strPath.c_str());
			return 0;
		}
		info.Width = pTexture->w;
		info.Height = pTexture->h;
	}

	CTextureMapGL* pMap = new CTextureMapGL(strTextureName, info.Width, info.Height, 0);
	pMap->Add(pTexture, 100);
	m_vecTextures.push_back(pMap);

	SDL_FreeSurface(pTexture);
    

#ifdef _DEBUG_TEXTURES
	LARGE_INTEGER end, freq;
	QueryPerformanceCounter(&end);
	QueryPerformanceFrequency(&freq);
	char temp[200];
	sprintf(temp, "Load %s: %.1fms%s\n", strPath.c_str(), 1000.f * (end.QuadPart - start.QuadPart) / freq.QuadPart, (bundle >= 0) ? " (bundled)" : "");
	OutputDebugString(temp);
#endif

	return 1;
}

static CTextureArrayGL emptyTexture;

const CTextureArrayGL* CGUITextureManagerGL::GetTexture(const CStdString& strTextureName)
{
	const CTextureArray* tmp;
	tmp = CGUITextureManager::GetTexture(strTextureName);

	if(tmp == NULL)
		return (CTextureArrayGL *)&emptyTexture;
	else
		return (const CTextureArrayGL*)tmp;
}