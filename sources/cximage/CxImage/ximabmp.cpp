// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
/*
 * File:	ximabmp.cpp
 * Purpose:	Platform Independent BMP Image Class Loader and Writer
 * 07/Aug/2001 <ing.davide.pizzolato@libero.it>
 * CxImage version 5.80 29/Sep/2003
 */

#include "ximabmp.h"

#if CXIMAGE_SUPPORT_BMP

#include "ximaiter.h" 

const int RLE_COMMAND     = 0;
const int RLE_ENDOFLINE   = 0;
const int RLE_ENDOFBITMAP = 1;
const int RLE_DELTA       = 2;

#if !defined(BI_RLE8)
 #define BI_RLE8  1L
#endif
#if !defined(BI_RLE4)
 #define BI_RLE4  2L
#endif

////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
bool CxImageBMP::Encode(CxFile * hFile)
{

	if (EncodeSafeCheck(hFile)) return false;

	BITMAPFILEHEADER	hdr;

	hdr.bfType = 0x4d42;   // 'BM' WINDOWS_BITMAP_SIGNATURE
	hdr.bfSize = GetSize() + 14 /*sizeof(BITMAPFILEHEADER)*/;
	hdr.bfReserved1 = hdr.bfReserved2 = 0;
	hdr.bfOffBits = 14 /*sizeof(BITMAPFILEHEADER)*/ + head.biSize + GetPaletteSize();

	 //copy attributes
	memcpy(pDib,&head,sizeof(BITMAPINFOHEADER));
    // Write the file header
	hFile->Write(&hdr,min(14,sizeof(BITMAPFILEHEADER)),1);
    // Write the DIB header and the pixels
	hFile->Write(pDib,GetSize(),1);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
bool CxImageBMP::Decode(CxFile * hFile)
{
	if (hFile == NULL) return false;

	BITMAPFILEHEADER   bf;
	DWORD off = hFile->Tell(); //<CSC>
  try {
    if (hFile->Read(&bf,min(14,sizeof(bf)),1)==0) throw "Not a BMP";
    if (bf.bfType != BFT_BITMAP) { //do we have a RC HEADER?
        bf.bfOffBits = 0L;
        hFile->Seek(off,SEEK_SET);
    }

	if (!DibReadBitmapInfo(hFile,&head)) throw "Error reading BMP info";
	DWORD dwCompression=head.biCompression;
	DWORD dwBitCount=head.biBitCount; //preserve for BI_BITFIELDS compression <Thomas Ernst>
	bool bIsOldBmp = head.biSize == sizeof(BITMAPCOREHEADER);
	head.biSize = sizeof(BITMAPINFOHEADER);

	bool bTopDownDib = head.biHeight<0; //<Flanders> check if it's a top-down bitmap
	if (bTopDownDib) head.biHeight=-head.biHeight;

	if (!Create(head.biWidth,head.biHeight,head.biBitCount,CXIMAGE_FORMAT_BMP))
		throw "Can't allocate memory";

	info.xDPI = (long) floor(head.biXPelsPerMeter * 254.0 / 10000.0 + 0.5);
	info.yDPI = (long) floor(head.biYPelsPerMeter * 254.0 / 10000.0 + 0.5);

	if (info.nEscape) throw "Cancelled"; // <vho> - cancel decoding

    RGBQUAD *pRgb = GetPalette();
    if (pRgb){
        if (bIsOldBmp){
             // convert a old color table (3 byte entries) to a new
             // color table (4 byte entries)
            hFile->Read((void*)pRgb,DibNumColors(&head) * sizeof(RGBTRIPLE),1);
            for (int i=DibNumColors(&head)-1; i>=0; i--){
                pRgb[i].rgbRed      = ((RGBTRIPLE *)pRgb)[i].rgbtRed;
                pRgb[i].rgbBlue     = ((RGBTRIPLE *)pRgb)[i].rgbtBlue;
                pRgb[i].rgbGreen    = ((RGBTRIPLE *)pRgb)[i].rgbtGreen;
                pRgb[i].rgbReserved = (BYTE)0;
            }
        } else {
            hFile->Read((void*)pRgb,DibNumColors(&head) * sizeof(RGBQUAD),1);
			//force rgbReserved=0, to avoid problems with some WinXp bitmaps
			for (unsigned int i=0; i<head.biClrUsed; i++) pRgb[i].rgbReserved=0;
        }
    }

	if (info.nEscape) throw "Cancelled"; // <vho> - cancel decoding

	switch (dwBitCount) {
		case 32 :
			if (bf.bfOffBits != 0L) hFile->Seek(off + bf.bfOffBits,SEEK_SET);
			if (dwCompression == BI_BITFIELDS || dwCompression == BI_RGB){
				long imagesize=4*head.biHeight*head.biWidth;
				BYTE* buff32=(BYTE*)malloc(imagesize);
				if (buff32){
					hFile->Read(buff32, imagesize,1); // read in the pixels
					Bitfield2RGB(buff32,0,0,0,32);
					free(buff32);
				} else throw "can't allocate memory";
			} else throw "unknown compression";
			break;
		case 24 :
			if (bf.bfOffBits != 0L) hFile->Seek(off + bf.bfOffBits,SEEK_SET);
			if (dwCompression == BI_RGB){
				hFile->Read(info.pImage, head.biSizeImage,1); // read in the pixels
			} else throw "unknown compression";
			break;
		case 16 :
		{
			DWORD bfmask[3];
			if (dwCompression == BI_BITFIELDS)
			{
				hFile->Read(bfmask, 12, 1);
			} else {
				bfmask[0]=0x7C00; bfmask[1]=0x3E0; bfmask[2]=0x1F; //RGB555
			}
			// bf.bfOffBits required after the bitfield mask <Cui Ying Jie>
			if (bf.bfOffBits != 0L) hFile->Seek(off + bf.bfOffBits,SEEK_SET);
			// read in the pixels
			hFile->Read(info.pImage, head.biHeight*((head.biWidth+1)/2)*4,1);
			// transform into RGB
			Bitfield2RGB(info.pImage,(WORD)bfmask[0],(WORD)bfmask[1],(WORD)bfmask[2],16);
			break;
		}
		case 8 :
		case 4 :
		case 1 :
		if (bf.bfOffBits != 0L) hFile->Seek(off + bf.bfOffBits,SEEK_SET);
		switch (dwCompression) {
			case BI_RGB :
				hFile->Read(info.pImage, head.biSizeImage,1); // read in the pixels
				break;
			case BI_RLE4 :
			{
				BYTE status_byte = 0;
				BYTE second_byte = 0;
				int scanline = 0;
				int bits = 0;
				BOOL low_nibble = FALSE;
				CImageIterator iter(this);

				for (BOOL bContinue = TRUE; bContinue;) {
					hFile->Read(&status_byte, sizeof(BYTE), 1);
					switch (status_byte) {
						case RLE_COMMAND :
							hFile->Read(&status_byte, sizeof(BYTE), 1);
							switch (status_byte) {
								case RLE_ENDOFLINE :
									bits = 0;
									scanline++;
									low_nibble = FALSE;
									break;
								case RLE_ENDOFBITMAP :
									bContinue=FALSE;
									break;
								case RLE_DELTA :
								{
									// read the delta values
									BYTE delta_x;
									BYTE delta_y;
									hFile->Read(&delta_x, sizeof(BYTE), 1);
									hFile->Read(&delta_y, sizeof(BYTE), 1);
									// apply them
									bits       += delta_x / 2;
									scanline   += delta_y;
									break;
								}
								default :
									hFile->Read(&second_byte, sizeof(BYTE), 1);
									BYTE *sline = iter.GetRow(scanline);
									for (int i = 0; i < status_byte; i++) {
										if (low_nibble) {
											if ((DWORD)(sline+bits) < (DWORD)(info.pImage+head.biSizeImage)){
												*(sline + bits) |=  second_byte & 0x0F;
											}
											if (i != status_byte - 1)
												hFile->Read(&second_byte, sizeof(BYTE), 1);
											bits++;
										} else {
											if ((DWORD)(sline+bits) < (DWORD)(info.pImage+head.biSizeImage)){
												*(sline + bits) = (BYTE)(second_byte & 0xF0);
											}
										}
										low_nibble = !low_nibble;
									}
									if (((status_byte / 2) & 1 )== 1)
										hFile->Read(&second_byte, sizeof(BYTE), 1);												
									break;
							};
							break;
						default :
						{
							BYTE *sline = iter.GetRow(scanline);
							hFile->Read(&second_byte, sizeof(BYTE), 1);
							for (unsigned i = 0; i < status_byte; i++) {
								if (low_nibble) {
									if ((DWORD)(sline+bits) < (DWORD)(info.pImage+head.biSizeImage)){
										*(sline + bits) |= second_byte & 0x0F;
									}
									bits++;
								} else {
									if ((DWORD)(sline+bits) < (DWORD)(info.pImage+head.biSizeImage)){
										*(sline + bits) = (BYTE)(second_byte & 0xF0);
									}
								}				
								low_nibble = !low_nibble;
							}
						}
						break;
					};
				}
				break;
			}
			case BI_RLE8 :
			{
				BYTE status_byte = 0;
				BYTE second_byte = 0;
				int scanline = 0;
				int bits = 0;
				CImageIterator iter(this);

				for (BOOL bContinue = TRUE; bContinue; ) {
					hFile->Read(&status_byte, sizeof(BYTE), 1);
					switch (status_byte) {
						case RLE_COMMAND :
							hFile->Read(&status_byte, sizeof(BYTE), 1);
							switch (status_byte) {
								case RLE_ENDOFLINE :
									bits = 0;
									scanline++;
									break;
								case RLE_ENDOFBITMAP :
									bContinue=FALSE;
									break;
								case RLE_DELTA :
								{
									// read the delta values
									BYTE delta_x;
									BYTE delta_y;
									hFile->Read(&delta_x, sizeof(BYTE), 1);
									hFile->Read(&delta_y, sizeof(BYTE), 1);
									// apply them
									bits     += delta_x;
									scanline += delta_y;
									break;
								}
								default :
									hFile->Read((void *)(iter.GetRow(scanline) + bits), sizeof(BYTE) * status_byte, 1);
									// align run length to even number of bytes 
									if ((status_byte & 1) == 1)
										hFile->Read(&second_byte, sizeof(BYTE), 1);												
									bits += status_byte;													
									break;								
							};
							break;
						default :
							BYTE *sline = iter.GetRow(scanline);
							hFile->Read(&second_byte, sizeof(BYTE), 1);
							for (unsigned i = 0; i < status_byte; i++) {
								*(sline + bits) = second_byte;
								bits++;					
							}
							break;
					};
				}
				break;
			}
			default :								
				throw "compression type not supported";
		}
	}

	if (bTopDownDib) Flip(); //<Flanders>

  } catch (char *message) {
	strncpy(info.szLastError,message,255);
	return false;
  }
    return true;
}
////////////////////////////////////////////////////////////////////////////////
/*  ReadDibBitmapInfo()
 *
 *  Will read a file in DIB format and return a global HANDLE to its
 *  BITMAPINFO.  This function will work with both "old" and "new"
 *  bitmap formats, but will always return a "new" BITMAPINFO.
 */
bool CxImageBMP::DibReadBitmapInfo(CxFile* fh, BITMAPINFOHEADER *pdib)
{
	if ((fh==NULL)||(pdib==NULL)) return false;

    if (fh->Read(pdib,sizeof(BITMAPINFOHEADER),1)==0) return false;

    BITMAPCOREHEADER   bc;

    switch (pdib->biSize) // what type of bitmap info is this?
    {
        case sizeof(BITMAPINFOHEADER):
            break;

		case 64: //sizeof(OS2_BMP_HEADER):
            fh->Seek((long)(64 - sizeof(BITMAPINFOHEADER)),SEEK_CUR);
			break;

        case sizeof(BITMAPCOREHEADER):
            bc = *(BITMAPCOREHEADER*)pdib;
            pdib->biSize               = bc.bcSize;
            pdib->biWidth              = (DWORD)bc.bcWidth;
            pdib->biHeight             = (DWORD)bc.bcHeight;
            pdib->biPlanes             =  bc.bcPlanes;
            pdib->biBitCount           =  bc.bcBitCount;
            pdib->biCompression        = BI_RGB;
            pdib->biSizeImage          = 0;
            pdib->biXPelsPerMeter      = 0;
            pdib->biYPelsPerMeter      = 0;
            pdib->biClrUsed            = 0;
            pdib->biClrImportant       = 0;

			fh->Seek((long)(sizeof(BITMAPCOREHEADER)-sizeof(BITMAPINFOHEADER)), SEEK_CUR);

            break;
        default:
			//give a last chance
			 if (pdib->biSize>(sizeof(BITMAPINFOHEADER))&&
				(pdib->biSizeImage==(unsigned long)(pdib->biHeight*((((pdib->biBitCount*pdib->biWidth)+31)/32)*4)))&&
				(pdib->biPlanes==1)&&(pdib->biCompression==BI_RGB)&&(pdib->biClrUsed==0))
			 {
	             fh->Seek((long)(pdib->biSize - sizeof(BITMAPINFOHEADER)),SEEK_CUR);
				 break;
			 }
			return false;
    }

    FixBitmapInfo(pdib);

    return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
#endif 	// CXIMAGE_SUPPORT_BMP
////////////////////////////////////////////////////////////////////////////////
