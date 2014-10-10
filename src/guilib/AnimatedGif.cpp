
// ****************************************************************************
//
// WINIMAGE.CPP : Generic classes for raster images (MSWindows specialization)
//
//  Content: Member definitions for:
//  - class CAnimatedGif             : Storage class for single images
//  - class CAnimatedGifSet          : Storage class for sets of images
//
//  (Includes routines to Load and Save BMP files and to load GIF files into
// these classes).
//
//  --------------------------------------------------------------------------
//
// Copyright (c) 2000, Juan Soulie <jsoulie@cplusplus.com>
//
// Permission to use, copy, modify, distribute and sell this software or any
// part thereof and/or its documentation for any purpose is granted without fee
// provided that the above copyright notice and this permission notice appear
// in all copies.
//
// This software is provided "as is" without express or implied warranty of
// any kind. The author shall have no liability with respect to the
// infringement of copyrights or patents that any modification to the content
// of this file or this file itself may incur.
//
// ****************************************************************************

#include "AnimatedGif.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/EndianSwap.h"
#include "utils/log.h"

#ifdef TARGET_WINDOWS
extern "C" FILE *fopen_utf8(const char *_Filename, const char *_Mode);
#else
#define fopen_utf8 fopen
#endif

#pragma pack(1)
// Error processing macro (NO-OP by default):
#define ERRORMSG(PARAM) {}

#ifndef BI_RGB
 #define BI_RGB        0L
 #define BI_RLE8       1L
 #define BI_RLE4       2L
 #define BI_BITFIELDS  3L
#endif

#undef ALIGN
#define ALIGN sizeof(int)         ///< Windows GDI expects all int-aligned

// Macros to swap data endianness
#define SWAP16(X)    X=Endian_SwapLE16(X)
#define SWAP32(X)    X=Endian_SwapLE32(X)

// pre-declaration:
int LZWDecoder (char*, char*, short, int, int, int, const int);

// ****************************************************************************
// * CAnimatedGif Member definitions                                               *
// ****************************************************************************

CAnimatedGif::CAnimatedGif()
{
  Height = Width = 0;
  Raster = NULL;
  Palette = NULL;
  pbmi = NULL;
  BPP = Transparent = BytesPerRow = 0;
  xPos = yPos = Delay = Transparency = 0;
  nLoops = 1; //default=play animation 1 time
}

CAnimatedGif::~CAnimatedGif()
{
  delete [] pbmi;
  delete [] Raster;
  delete [] Palette;
}

// Init: Allocates space for raster and palette in GDI-compatible structures.
void CAnimatedGif::Init(int iWidth, int iHeight, int iBPP, int iLoops)
{
  delete[] Raster;
  Raster = NULL;

  delete[] pbmi;
  pbmi = NULL;

  delete[] Palette;
  Palette = NULL;

  // Standard members setup
  Transparent = -1;
  BytesPerRow = Width = iWidth;
  Height = iHeight;
  BPP = iBPP;
  // Animation Extra members setup:
  xPos = yPos = Delay = Transparency = 0;
  nLoops = iLoops;

  if (BPP == 24)
  {
    BytesPerRow *= 3;
    pbmi = (GUIBITMAPINFO*)new char [sizeof(GUIBITMAPINFO)];
  }
  else
  {
    pbmi = (GUIBITMAPINFO*)new char[sizeof(GUIBITMAPINFOHEADER)];
    Palette = new COLOR[256];
  }

  BytesPerRow += (ALIGN - Width % ALIGN) % ALIGN; // Align BytesPerRow
  int size = BytesPerRow * Height;

  Raster = new char [size];

  pbmi->bmiHeader.biSize = sizeof (GUIBITMAPINFOHEADER);
  pbmi->bmiHeader.biWidth = Width;
  pbmi->bmiHeader.biHeight = -Height;   // negative means up-to-bottom
  pbmi->bmiHeader.biPlanes = 1;
  pbmi->bmiHeader.biBitCount = (BPP < 8 ? 8 : BPP); // Our raster is byte-aligned
  pbmi->bmiHeader.biCompression = BI_RGB;
  pbmi->bmiHeader.biSizeImage = 0;
  pbmi->bmiHeader.biXPelsPerMeter = 11811;
  pbmi->bmiHeader.biYPelsPerMeter = 11811;
  pbmi->bmiHeader.biClrUsed = 0;
  pbmi->bmiHeader.biClrImportant = 0;
}

// operator=: copies an object's content to another
CAnimatedGif& CAnimatedGif::operator = (CAnimatedGif& rhs)
{
  Init(rhs.Width, rhs.Height, rhs.BPP); // respects virtualization
  memcpy(Raster, rhs.Raster, BytesPerRow*Height);
  memcpy(Palette, rhs.Palette, 256*sizeof(COLOR));
  return *this;
}



CAnimatedGifSet::CAnimatedGifSet()
{
  FrameHeight = FrameWidth = 0;
  nLoops = 1; //default=play animation 1 time
}

CAnimatedGifSet::~CAnimatedGifSet()
{
  Release();
}

void CAnimatedGifSet::Release()
{
  FrameWidth = 0;
  FrameHeight = 0;
  for (int i = 0; i < (int)m_vecimg.size(); ++i)
  {
    CAnimatedGif* pImage = m_vecimg[i];
    delete pImage;
  }
  m_vecimg.erase(m_vecimg.begin(), m_vecimg.end());

}

// ****************************************************************************
// * CAnimatedGifSet Member definitions                                            *
// ****************************************************************************

// AddImage: Adds an image object to the back of the img vector.
void CAnimatedGifSet::AddImage (CAnimatedGif* newimage)
{
  m_vecimg.push_back(newimage);
}

int CAnimatedGifSet::GetImageCount() const
{
  return m_vecimg.size();
}

unsigned char CAnimatedGifSet::getbyte(FILE *fd)
{
  unsigned char uchar;
  if (fread(&uchar, 1, 1, fd) == 1)
    return uchar;
  else
    return 0;
}

// ****************************************************************************
// * LoadGIF                                                                  *
// *   Load a GIF File into the CAnimatedGifSet object                             *
// *                        (c) Nov 2000, Juan Soulie <jsoulie@cplusplus.com> *
// ****************************************************************************
int CAnimatedGifSet::LoadGIF (const char * szFileName)
{
  int n;
  // Global GIF variables:
  int GlobalBPP;       // Bits per Pixel.
  COLOR * GlobalColorMap;     // Global colormap (allocate)

  struct GIFGCEtag
  {                // GRAPHIC CONTROL EXTENSION
    unsigned char BlockSize;   // Block Size: 4 bytes
    unsigned char PackedFields;  // 3.. Packed Fields. Bits detail:
    //    0: Transparent Color Flag
    //    1: User Input Flag
    //  2-4: Disposal Method
    unsigned short Delay;     // 4..5 Delay Time (1/100 seconds)
    unsigned char Transparent;  // 6.. Transparent Color Index
  }
  gifgce;

  struct GIFNetscapeTag
  {
    unsigned char comment[11];  //4...14  NETSCAPE2.0
    unsigned char SubBlockLength; //15      0x3
    unsigned char reserved;       //16      0x1
    unsigned short iIterations ;    //17..18  number of iterations (lo-hi)
  }
  gifnetscape;

  int GraphicExtensionFound = 0;

  // OPEN FILE
  FILE *fd = fopen_utf8(CSpecialProtocol::TranslatePath(szFileName).c_str(), "rb");
  if (!fd)
  {
    return 0;
  }

  // *1* READ HEADERBLOCK (6bytes) (SIGNATURE + VERSION)
  char szSignature[6];    // First 6 bytes (GIF87a or GIF89a)
  int iRead = fread(szSignature, 1, 6, fd);
  if (iRead != 6)
  {
    fclose(fd);
    return 0;
  }
  if ( memcmp(szSignature, "GIF", 2) != 0)
  {
    fclose(fd);
    return 0;
  }
  // *2* READ LOGICAL SCREEN DESCRIPTOR
  struct GIFLSDtag
  {
    unsigned short ScreenWidth;  // Logical Screen Width
    unsigned short ScreenHeight; // Logical Screen Height
    unsigned char PackedFields;  // Packed Fields. Bits detail:
    //  0-2: Size of Global Color Table
    //    3: Sort Flag
    //  4-6: Color Resolution
    //    7: Global Color Table Flag
    unsigned char Background;  // Background Color Index
    unsigned char PixelAspectRatio; // Pixel Aspect Ratio
  }
  giflsd;

  iRead = fread(&giflsd, 1, sizeof(giflsd), fd);
  if (iRead != sizeof(giflsd))
  {
    fclose(fd);
    return 0;
  }
  // endian swap
  SWAP16(giflsd.ScreenWidth);
  SWAP16(giflsd.ScreenHeight);

  GlobalBPP = (giflsd.PackedFields & 0x07) + 1;

  // fill some animation data:
  FrameWidth = giflsd.ScreenWidth;
  FrameHeight = giflsd.ScreenHeight;
  nLoops = 1; //default=play animation 1 time

  // *3* READ/GENERATE GLOBAL COLOR MAP
  GlobalColorMap = new COLOR [1 << GlobalBPP];
  if (giflsd.PackedFields & 0x80) // File has global color map?
    for (n = 0;n < 1 << GlobalBPP;n++)
    {
      GlobalColorMap[n].r = getbyte(fd);
      GlobalColorMap[n].g = getbyte(fd);
      GlobalColorMap[n].b = getbyte(fd);
      GlobalColorMap[n].x = 0;
    }

  else // GIF standard says to provide an internal default Palette:
    for (n = 0;n < 256;n++)
    {
      GlobalColorMap[n].r = GlobalColorMap[n].g = GlobalColorMap[n].b = n;
      GlobalColorMap[n].x = 0;
    }

  // *4* NOW WE HAVE 3 POSSIBILITIES:
  //  4a) Get and Extension Block (Blocks with additional information)
  //  4b) Get an Image Separator (Introductor to an image)
  //  4c) Get the trailer Char (End of GIF File)
  do
  {
    int charGot = getbyte(fd);

    if (charGot == 0x21)  // *A* EXTENSION BLOCK
    {
      unsigned char extensionType = getbyte(fd);
      switch (extensionType)
      {
      case 0xF9:    // Graphic Control Extension
        {
          if (fread((char*)&gifgce, 1, sizeof(gifgce), fd) == sizeof(gifgce))
            SWAP16(gifgce.Delay);
          GraphicExtensionFound++;
          getbyte(fd); // Block Terminator (always 0)
        }
        break;

      case 0xFE:    // Comment Extension: Ignored
        {
          while (int nBlockLength = getbyte(fd))
            for (n = 0;n < nBlockLength;n++) getbyte(fd);
        }
        break;

      case 0x01:    // PlainText Extension: Ignored
        {
          while (int nBlockLength = getbyte(fd))
            for (n = 0;n < nBlockLength;n++) getbyte(fd);
        }
        break;

      case 0xFF:    // Application Extension: Ignored
        {
          int nBlockLength = getbyte(fd);
          if (nBlockLength == 0x0b)
          {
            struct GIFNetscapeTag tag;
            if (fread((char*)&tag, 1, sizeof(gifnetscape), fd) == sizeof(gifnetscape))
            {
              SWAP16(tag.iIterations);
              nLoops = tag.iIterations;
            }
            else
              nLoops = 0;

            if (nLoops) nLoops++;
            getbyte(fd);
          }
          else
          {
            do
            {
              for (n = 0;n < nBlockLength;n++) getbyte(fd);
            }
            while ((nBlockLength = getbyte(fd)) != 0);
          }
        }
        break;

       default:    // Unknown Extension: Ignored
        {
          // read (and ignore) data sub-blocks
          while (int nBlockLength = getbyte(fd))
            for (n = 0;n < nBlockLength;n++) getbyte(fd);
        }
        break;
      }
    }
    else if (charGot == 0x2c)
    { // *B* IMAGE (0x2c Image Separator)
      // Create a new Image Object:
      CAnimatedGif* NextImage = new CAnimatedGif();

      // Read Image Descriptor
      struct GIFIDtag
      {
        unsigned short xPos;     // Image Left Position
        unsigned short yPos;     // Image Top Position
        unsigned short Width;     // Image Width
        unsigned short Height;    // Image Height
        unsigned char PackedFields;  // Packed Fields. Bits detail:
        //  0-2: Size of Local Color Table
        //  3-4: (Reserved)
        //    5: Sort Flag
        //    6: Interlace Flag
        //    7: Local Color Table Flag
      }
      gifid;

      memset(&gifid, 0, sizeof(gifid));

      int LocalColorMap = 0;
      if (fread((char*)&gifid, 1, sizeof(gifid), fd) == sizeof(gifid))
      {
        SWAP16(gifid.xPos);
        SWAP16(gifid.yPos);
        SWAP16(gifid.Width);
        SWAP16(gifid.Height);

        LocalColorMap = (gifid.PackedFields & 0x08) ? 1 : 0;
      }

      NextImage->Init(gifid.Width, gifid.Height, LocalColorMap ? (gifid.PackedFields&7) + 1 : GlobalBPP);

      /* verify that all the image is inside the screen dimensions */
      if (gifid.xPos + gifid.Width > giflsd.ScreenWidth || gifid.yPos + gifid.Height > giflsd.ScreenHeight)
        return 0;

      // Fill NextImage Data
      NextImage->xPos = gifid.xPos;
      NextImage->yPos = gifid.yPos;
      if (GraphicExtensionFound)
      {
        NextImage->Transparent = (gifgce.PackedFields & 0x01) ? gifgce.Transparent : -1;
        NextImage->Transparency = (gifgce.PackedFields & 0x1c) > 1 ? 1 : 0;
        NextImage->Delay = gifgce.Delay * 10;
      }

      if (NextImage->Transparent != -1)
        memset(NextImage->Raster, NextImage->Transparent, NextImage->BytesPerRow * NextImage->Height);
      else
        memset(NextImage->Raster, giflsd.Background, NextImage->BytesPerRow * NextImage->Height);

      // Read Color Map (if descriptor says so)
      size_t palSize = sizeof(COLOR)*(1 << NextImage->BPP);
      bool isPalRead = false;
      if (LocalColorMap && fread((char*)NextImage->Palette, 1, palSize, fd) == palSize)
        isPalRead = true;

      // Copy global, if no palette
      if (!isPalRead)
        memcpy(NextImage->Palette, GlobalColorMap, palSize);

      short firstbyte = getbyte(fd); // 1st byte of img block (CodeSize)

      // Calculate compressed image block size
      // to fix: this allocates an extra byte per block
      long ImgStart, ImgEnd;
      ImgEnd = ImgStart = ftell(fd);
      while ((n = getbyte(fd)) !=  0) fseek (fd, ImgEnd += n + 1, SEEK_SET );
      fseek (fd, ImgStart, SEEK_SET);

      // Allocate Space for Compressed Image
      char * pCompressedImage = new char [ImgEnd - ImgStart + 4];

      // Read and store Compressed Image
      char * pTemp = pCompressedImage;
      while (int nBlockLength = getbyte(fd))
      {
        if (fread(pTemp, 1, nBlockLength, fd) != (size_t)nBlockLength)
        {
        // Error?
        }
        pTemp += nBlockLength;
      }

      // Call LZW/GIF decompressor
      n = LZWDecoder(
            (char*) pCompressedImage,
            (char*) NextImage->Raster,
            firstbyte, NextImage->BytesPerRow, //NextImage->AlignedWidth,
            gifid.Width, gifid.Height,
            ((gifid.PackedFields & 0x40) ? 1 : 0) //Interlaced?
          );

      if (n)
        AddImage(NextImage);
      else
      {
        delete NextImage;
        CLog::Log(LOGERROR, "CAnimatedGifSet::LoadGIF: gif file corrupt: %s", szFileName);
        ERRORMSG("GIF File Corrupt");
      }

      // Some cleanup
      delete[] pCompressedImage;
      GraphicExtensionFound = 0;
    }
    else if (charGot == 0x3b)
    {
      // *C* TRAILER: End of GIF Info
      break; // Ok. Standard End.
    }

  }
  while ( !feof(fd) );

  delete[] GlobalColorMap;
  fclose(fd);
  if ( GetImageCount() == 0) ERRORMSG("Premature End Of File");
  return GetImageCount();
}

// ****************************************************************************
// * LZWDecoder (C/C++)                                                       *
// * Codec to perform LZW (GIF Variant) decompression.                        *
// *                         (c) Nov2000, Juan Soulie <jsoulie@cplusplus.com> *
// ****************************************************************************
//
// Parameter description:
//  - bufIn: Input buffer containing a "de-blocked" GIF/LZW compressed image.
//  - bufOut: Output buffer where result will be stored.
//  - InitCodeSize: Initial CodeSize to be Used
//    (GIF files include this as the first byte in a picture block)
//  - AlignedWidth : Width of a row in memory (including alignment if needed)
//  - Width, Height: Physical dimensions of image.
//  - Interlace: 1 for Interlaced GIFs.
//
int LZWDecoder (char * bufIn, char * bufOut,
                short InitCodeSize, int AlignedWidth,
                int Width, int Height, const int Interlace)
{
  if (InitCodeSize < 1 || InitCodeSize >= LZW_MAXBITS)
    return 0;
  int n;
  int row = 0, col = 0;    // used to point output if Interlaced
  int nPixels, maxPixels; // Output pixel counter

  short CodeSize;      // Current CodeSize (size in bits of codes)
  short ClearCode;     // Clear code : resets decompressor
  short EndCode;      // End code : marks end of information

  long whichBit;      // Index of next bit in bufIn
  long LongCode;      // Temp. var. from which Code is retrieved
  short Code;        // Code extracted
  short PrevCode;      // Previous Code
  short OutCode;      // Code to output

  // Translation Table:
  short Prefix[LZW_SIZETABLE] = {};    // Prefix: index of another Code
  unsigned char Suffix[LZW_SIZETABLE] = {};    // Suffix: terminating character
  short FirstEntry;     // Index of first free entry in table
  short NextEntry;     // Index of next free entry in table

  unsigned char OutStack[LZW_SIZETABLE + 1];   // Output buffer
  int OutIndex;      // Characters in OutStack

  int RowOffset;     // Offset in output buffer for current row

  // Set up values that depend on InitCodeSize Parameter.
  CodeSize = InitCodeSize + 1;
  ClearCode = (1 << InitCodeSize);
  EndCode = ClearCode + 1;
  NextEntry = FirstEntry = ClearCode + 2;

  whichBit = 0;
  nPixels = 0;
  maxPixels = Width * Height;
  RowOffset = 0;
  PrevCode = 0;

  while (nPixels < maxPixels)
  {
    OutIndex = 0;       // Reset Output Stack

    // GET NEXT CODE FROM bufIn:
    // LZW compression uses code items longer than a single byte.
    // For GIF Files, code sizes are variable between 9 and 12 bits
    // That's why we must read data (Code) this way:
    LongCode = *((long*)(bufIn + whichBit / 8));     // Get some bytes from bufIn
    SWAP32(LongCode);
    LongCode >>= (whichBit&7);            // Discard too low bits
    Code = (short)((LongCode & ((1 << CodeSize) - 1) )); // Discard too high bits
    whichBit += CodeSize;              // Increase Bit Offset

    // SWITCH, DIFFERENT POSIBILITIES FOR CODE:
    if (Code == EndCode)     // END CODE
      break;           // Exit LZW Decompression loop

    if (Code == ClearCode)
    {
      // CLEAR CODE:
      CodeSize = InitCodeSize + 1; // Reset CodeSize
      NextEntry = FirstEntry;   // Reset Translation Table
      PrevCode = Code;       // Prevent next to be added to table.
      continue;          // restart, to get another code
    }
    if (Code < NextEntry)     // CODE IS IN TABLE
      OutCode = Code;       // Set code to output.

    else
    {               // CODE IS NOT IN TABLE:
      OutIndex++;         // Keep "first" character of previous output.
      OutCode = PrevCode;     // Set PrevCode to be output
    }

    // EXPAND OutCode IN OutStack
    // - Elements up to FirstEntry are Raw-Codes and are not expanded
    // - Table Prefices contain indexes to other codes
    // - Table Suffices contain the raw codes to be output
    while (OutCode >= FirstEntry)
    {
      if (OutIndex > LZW_SIZETABLE || OutCode >= LZW_SIZETABLE)
        return 0;
      OutStack[OutIndex++] = Suffix[OutCode]; // Add suffix to Output Stack
      OutCode = Prefix[OutCode];       // Loop with preffix
    }

    // NOW OutCode IS A RAW CODE, ADD IT TO OUTPUT STACK.
    if (OutIndex > LZW_SIZETABLE)
      return 0;
    OutStack[OutIndex++] = (unsigned char) OutCode;

    // ADD NEW ENTRY TO TABLE (PrevCode + OutCode)
    // (EXCEPT IF PREVIOUS CODE WAS A CLEARCODE)
    if (PrevCode != ClearCode)
    {
      // Prevent Translation table overflow:
      if (NextEntry >= LZW_SIZETABLE)
        return 0;

      Prefix[NextEntry] = PrevCode;
      Suffix[NextEntry] = (unsigned char) OutCode;
      NextEntry++;

      // INCREASE CodeSize IF NextEntry IS INVALID WITH CURRENT CodeSize
      if (NextEntry >= (1 << CodeSize))
      {
        if (CodeSize < LZW_MAXBITS) CodeSize++;
        else
        {
          ;
        }    // Do nothing. Maybe next is Clear Code.
      }
    }

    PrevCode = Code;

    // Avoid the possibility of overflow on 'bufOut'.
    if (nPixels + OutIndex > maxPixels) OutIndex = maxPixels - nPixels;

    // OUTPUT OutStack (LAST-IN FIRST-OUT ORDER)
    for (n = OutIndex - 1; n >= 0; n--)
    {
      if (col == Width)      // Check if new row.
      {
        if (Interlace)
        {
          // If interlaced::
          if ((row&7) == 0) {row += 8; if (row >= Height) row = 4;}
        else if ((row&3) == 0) {row += 8; if (row >= Height) row = 2;}
        else if ((row&1) == 0) {row += 4; if (row >= Height) row = 1;}
          else row += 2;
        }
        else       // If not interlaced:
          row++;

        RowOffset = row * AlignedWidth;  // Set new row offset
        col = 0;
      }
      bufOut[RowOffset + col] = OutStack[n]; // Write output
      col++; nPixels++;     // Increase counters.
    }

  } // while (main decompressor loop)

  return whichBit;
}

// Refer to WINIMAGE.TXT for copyright and patent notices on GIF and LZW.

#pragma pack()
