// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
// xImaCodec.cpp : Encode Decode functions
/* 07/08/2001 v1.00 - ing.davide.pizzolato@libero.it
 * CxImage version 5.80 29/Sep/2003
 */

#include "ximage.h"

#ifdef _LINUX
#include <X11/Xmd.h>
#endif

#if CXIMAGE_SUPPORT_JPG
#include "ximajpg.h"
#endif

#if CXIMAGE_SUPPORT_GIF
#include "ximagif.h"
#endif

#if CXIMAGE_SUPPORT_PNG
#include "ximapng.h"
#endif

#if CXIMAGE_SUPPORT_MNG
#include "ximamng.h"
#endif

#if CXIMAGE_SUPPORT_BMP
#include "ximabmp.h"
#endif

#if CXIMAGE_SUPPORT_ICO
#include "ximaico.h"
#endif

#if CXIMAGE_SUPPORT_TIF
#include "ximatif.h"
#endif

#if CXIMAGE_SUPPORT_TGA
#include "ximatga.h"
#endif

#if CXIMAGE_SUPPORT_PCX
#include "ximapcx.h"
#endif

#if CXIMAGE_SUPPORT_WBMP
#include "ximawbmp.h"
#endif

#if CXIMAGE_SUPPORT_WMF
#include "ximawmf.h" // <vho> - WMF/EMF support
#endif

#if CXIMAGE_SUPPORT_J2K
#include "ximaj2k.h"
#endif

#if CXIMAGE_SUPPORT_JBG
#include "ximajbg.h"
#endif

#if CXIMAGE_SUPPORT_JASPER
#include "ximajas.h"
#endif

////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
bool CxImage::EncodeSafeCheck(CxFile *hFile)
{
	if (hFile==NULL) {
		strcpy(info.szLastError,CXIMAGE_ERR_NOFILE);
		return true;
	}

	if (pDib==NULL){
		strcpy(info.szLastError,CXIMAGE_ERR_NOIMAGE);
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Save(const char * filename, DWORD imagetype)
{
	FILE* hFile;	//file handle to write the image
	if ((hFile=fopen(filename,"wb"))==NULL)  return false;
	bool bOK = Encode(hFile,imagetype);
	fclose(hFile);
	return bOK;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Encode(FILE *hFile, DWORD imagetype)
{
	CxIOFile file(hFile);
	return Encode(&file,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Encode(BYTE * &buffer, long &size, DWORD imagetype)
{
	if (buffer!=NULL) return false; //the buffer must be empty
	CxMemFile file;
	file.Open();
	if(Encode(&file,imagetype)){
		buffer=file.GetBuffer();
		size=file.Size();
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Encode(CxFile *hFile, DWORD imagetype)
{

#if CXIMAGE_SUPPORT_BMP

	if (imagetype==CXIMAGE_FORMAT_BMP){
		CxImageBMP newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_ICO
	if (imagetype==CXIMAGE_FORMAT_ICO){
		CxImageICO newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_TIF
	if (imagetype==CXIMAGE_FORMAT_TIF){
		CxImageTIF newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_JPG
	if (imagetype==CXIMAGE_FORMAT_JPG){
		CxImageJPG newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_GIF
	if (imagetype==CXIMAGE_FORMAT_GIF){
		CxImageGIF newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_PNG
	if (imagetype==CXIMAGE_FORMAT_PNG){
		CxImagePNG newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_MNG
	if (imagetype==CXIMAGE_FORMAT_MNG){
		CxImageMNG newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_TGA
	if (imagetype==CXIMAGE_FORMAT_TGA){
		CxImageTGA newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_PCX
	if (imagetype==CXIMAGE_FORMAT_PCX){
		CxImagePCX newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_WBMP
	if (imagetype==CXIMAGE_FORMAT_WBMP){
		CxImageWBMP newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_WMF && CXIMAGE_SUPPORT_WINDOWS // <vho> - WMF/EMF support
	if (imagetype==CXIMAGE_FORMAT_WMF){
		CxImageWMF newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_J2K
	if (imagetype==CXIMAGE_FORMAT_J2K){
		CxImageJ2K newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_JBG
	if (imagetype==CXIMAGE_FORMAT_JBG){
		CxImageJBG newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_JASPER
	if (
 #if	CXIMAGE_SUPPORT_JP2
		imagetype==CXIMAGE_FORMAT_JP2 || 
 #endif
 #if	CXIMAGE_SUPPORT_JPC
		imagetype==CXIMAGE_FORMAT_JPC || 
 #endif
 #if	CXIMAGE_SUPPORT_PGX
		imagetype==CXIMAGE_FORMAT_PGX || 
 #endif
 #if	CXIMAGE_SUPPORT_PNM
		imagetype==CXIMAGE_FORMAT_PNM || 
 #endif
 #if	CXIMAGE_SUPPORT_RAS
		imagetype==CXIMAGE_FORMAT_RAS || 
 #endif
		 false ){
		CxImageJAS newima;
		newima.Ghost(this);
		if (newima.Encode(hFile,imagetype)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif

	strcpy(info.szLastError,"Encode: Unknown format");
	return false;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Encode(FILE * hFile, CxImage ** pImages, int pagecount, DWORD imagetype)
{
	CxIOFile file(hFile);
	return Encode(&file, pImages, pagecount,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Encode(CxFile * hFile, CxImage ** pImages, int pagecount, DWORD imagetype)
{
#if CXIMAGE_SUPPORT_TIF
	if (imagetype==CXIMAGE_FORMAT_TIF){
		CxImageTIF newima;
		newima.Ghost(this);
		if (newima.Encode(hFile,pImages,pagecount)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_GIF
	if (imagetype==CXIMAGE_FORMAT_GIF){
		CxImageGIF newima;
		newima.Ghost(this);
		if (newima.Encode(hFile,pImages,pagecount)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
	strcpy(info.szLastError,"Multipage Encode, Unsupported operation for this format");
	return false;
}

////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
#ifdef XBMC
bool CxImage::Load(const char * filename, DWORD imagetype, int &iWidth, int &iHeight)
#else
bool CxImage::Load(const char * filename, DWORD imagetype)
#endif
{
	printf("CxImage::Load - loading <%s> of type <%d>\n", filename, imagetype);

	/*FILE* hFile;	//file handle to read the image
	if ((hFile=fopen(filename,"rb"))==NULL)  return false;
	bool bOK = Decode(hFile,imagetype);
	fclose(hFile);*/

	/* automatic file type recognition */
	bool bOK = false;
	if ( imagetype > 0 && imagetype < CMAX_IMAGE_FORMATS ){
		FILE* hFile;	//file handle to read the image
		if ((hFile=fopen(filename,"rb"))==NULL)  {
			printf("failed to open file. err: %d\n",GetLastError());
			return false;
		}
#ifdef XBMC
		int iWidthSave = iWidth;
		int iHeightSave = iHeight;
#endif
    try
    {
#ifdef XBMC
      bOK = Decode(hFile,imagetype,iWidth,iHeight);
		  if (imagetype != CXIMAGE_FORMAT_JPG)
		  {
			  iWidth = GetWidth();
			  iHeight = GetHeight();
		  }
#else
		  bOK = Decode(hFile,imagetype);
#endif
    }
    catch (...)
    {
      printf("failed to open image - possibly the wrong extension?");
		}
    fclose(hFile);
		if (bOK) return bOK;
#ifdef XBMC
		iWidth = iWidthSave;
		iHeight = iHeightSave;
#endif
	}

	char sxzError[256];
	strcpy(sxzError,info.szLastError); //save the first error

	// if failed, try automatic recognition of the file...
	FILE* hFile;
	if ((hFile=fopen(filename,"rb"))==NULL)  return false;
  try
  { // Silently catch exceptions, because Decode of some image formats
    // could throw and we need to perform cleanup so that we can close 
    // our open file.
#ifdef XBMC
	  bOK = Decode(hFile,CXIMAGE_FORMAT_UNKNOWN,iWidth,iHeight);
#else
	  bOK = Decode(hFile,CXIMAGE_FORMAT_UNKNOWN);
#endif
  }
  catch (...)
  {
    printf("failed to decode file using auto recognition");
  }
  fclose(hFile);
#ifdef XBMC
	if (imagetype != CXIMAGE_FORMAT_JPG)
	{
		iWidth = GetWidth();
		iHeight = GetHeight();
	}
#endif
	if (!bOK && imagetype > 0) strcpy(info.szLastError,sxzError); //restore the first error

	return bOK;
}
////////////////////////////////////////////////////////////////////////////////
// File name constructor
// > filename: file name
// > imagetype: specify the image format (CXIMAGE_FORMAT_BMP,...)
CxImage::CxImage(const char * filename, DWORD imagetype)
{
	Startup(imagetype);
	Load(filename,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
// Stream constructor
// § the file is always closed by the constructor.
// > stream: file with "rb" access
// > imagetype: specify the image format (CXIMAGE_FORMAT_BMP,...)
CxImage::CxImage(FILE * stream, DWORD imagetype)
{
	Startup(imagetype);
	Decode(stream,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
CxImage::CxImage(CxFile * stream, DWORD imagetype)
{
	Startup(imagetype);
	Decode(stream,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
CxImage::CxImage(BYTE * buffer, DWORD size, DWORD imagetype)
{
	Startup(imagetype);
	CxMemFile stream(buffer,size);
	Decode(&stream,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Decode(BYTE * buffer, DWORD size, DWORD imagetype)
{
	CxMemFile file(buffer,size);
	return Decode(&file,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
#ifdef XBMC
bool CxImage::Decode(FILE *hFile, DWORD imagetype, int &iWidth, int &iHeight)
{
	CxIOFile file(hFile);
	return Decode(&file,imagetype,iWidth,iHeight);
}
#else
bool CxImage::Decode(FILE *hFile, DWORD imagetype)
{
	CxIOFile file(hFile);
	return Decode(&file,imagetype);
}
#endif
////////////////////////////////////////////////////////////////////////////////
#ifdef XBMC
bool CxImage::Decode(CxFile *hFile, DWORD imagetype, int &iWidth, int &iHeight)
#else
bool CxImage::Decode(CxFile *hFile, DWORD imagetype)
#endif
{

	if (imagetype==CXIMAGE_FORMAT_UNKNOWN){
		DWORD pos = hFile->Tell();
#if CXIMAGE_SUPPORT_BMP
		{ CxImageBMP newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#if CXIMAGE_SUPPORT_JPG
#ifdef XBMC
		{ CxImageJPG newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile, iWidth, iHeight)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#else
		{ CxImageJPG newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#endif
#if CXIMAGE_SUPPORT_ICO
		{ CxImageICO newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#if CXIMAGE_SUPPORT_GIF
		{ CxImageGIF newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#if CXIMAGE_SUPPORT_PNG
		{ CxImagePNG newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#if CXIMAGE_SUPPORT_TIF
		{ CxImageTIF newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#if CXIMAGE_SUPPORT_MNG
		{ CxImageMNG newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#if CXIMAGE_SUPPORT_TGA
		{ CxImageTGA newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#if CXIMAGE_SUPPORT_PCX
		{ CxImagePCX newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#if CXIMAGE_SUPPORT_WBMP
		{ CxImageWBMP newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#if CXIMAGE_SUPPORT_WMF && CXIMAGE_SUPPORT_WINDOWS
		{ CxImageWMF newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#if CXIMAGE_SUPPORT_J2K
		{ CxImageJ2K newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#if CXIMAGE_SUPPORT_JBG
		{ CxImageJBG newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
#if CXIMAGE_SUPPORT_JASPER
		{ CxImageJAS newima; try { newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); } catch (...) { hFile->Seek(pos,SEEK_SET); } }
#endif
	}

#if CXIMAGE_SUPPORT_BMP
	if (imagetype==CXIMAGE_FORMAT_BMP){
		CxImageBMP newima;
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_JPG
	if (imagetype==CXIMAGE_FORMAT_JPG){
		CxImageJPG newima;
#ifdef XBMC
		if (newima.Decode(hFile, iWidth, iHeight)){
#else
		if (newima.Decode(hFile)){
#endif
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_ICO
	if (imagetype==CXIMAGE_FORMAT_ICO){
		CxImageICO newima;
		newima.SetFrame(GetFrame()); //<REC>,29/01/02: handles multipage
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			info.nNumFrames = newima.info.nNumFrames;
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_GIF
	if (imagetype==CXIMAGE_FORMAT_GIF){
		CxImageGIF newima;
		newima.SetFrame(GetFrame()); //<REC>,29/01/02: handles multipage
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			info.nNumFrames = newima.info.nNumFrames;
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_PNG
	if (imagetype==CXIMAGE_FORMAT_PNG){
		CxImagePNG newima;
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_TIF
	if (imagetype==CXIMAGE_FORMAT_TIF){
		CxImageTIF newima;
		newima.SetFrame(GetFrame()); //<REC>,29/01/02: handles multipage
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			info.nNumFrames = newima.info.nNumFrames;
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_MNG
	if (imagetype==CXIMAGE_FORMAT_MNG){
		CxImageMNG newima;
		newima.SetFrame(GetFrame()); //<REC>,29/01/02: handles multipage
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			info.nNumFrames = newima.info.nNumFrames;
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_TGA
	if (imagetype==CXIMAGE_FORMAT_TGA){
		CxImageTGA newima;
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_PCX
	if (imagetype==CXIMAGE_FORMAT_PCX){
		CxImagePCX newima;
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_WBMP
	if (imagetype==CXIMAGE_FORMAT_WBMP){
		CxImageWBMP newima;
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_WMF && CXIMAGE_SUPPORT_WINDOWS // vho - WMF support
	if (imagetype == CXIMAGE_FORMAT_WMF){
		CxImageWMF newima;
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_J2K
	if (imagetype==CXIMAGE_FORMAT_J2K){
		CxImageJ2K newima;
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_JBG
	if (imagetype==CXIMAGE_FORMAT_JBG){
		CxImageJBG newima;
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_JASPER
	if (
 #if	CXIMAGE_SUPPORT_JP2
		imagetype==CXIMAGE_FORMAT_JP2 || 
 #endif
 #if	CXIMAGE_SUPPORT_JPC
		imagetype==CXIMAGE_FORMAT_JPC || 
 #endif
 #if	CXIMAGE_SUPPORT_PGX
		imagetype==CXIMAGE_FORMAT_PGX || 
 #endif
 #if	CXIMAGE_SUPPORT_PNM
		imagetype==CXIMAGE_FORMAT_PNM || 
 #endif
 #if	CXIMAGE_SUPPORT_RAS
		imagetype==CXIMAGE_FORMAT_RAS || 
 #endif
		 false ){
		CxImageJAS newima;
		if (newima.Decode(hFile,imagetype)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif

	strcpy(info.szLastError,"Decode: Unknown or wrong format");
	return false;
}

#ifdef XBMC
bool CxImage::GetExifThumbnail(const char *filename, const char *outname)
{
  CxImageJPG image;
  return image.GetExifThumbnail(filename, outname);
}
#endif
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
