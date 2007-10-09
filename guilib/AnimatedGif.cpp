#include "include.h"

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
// Copyright © 2000, Juan Soulie <jsoulie@cplusplus.com>
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


#pragma pack(1) 
// Error processing macro (NO-OP by default):
#define ERRORMSG(PARAM) {}

#ifndef BI_RGB
 #define BI_RGB        0L
 #define BI_RLE8       1L
 #define BI_RLE4       2L
 #define BI_BITFIELDS  3L
#endif 
// pre-declaration:
int LZWDecoder (char*, char*, short, int, int, int, const int);

// ****************************************************************************
// * CAnimatedGif Member definitions                                               *
// ****************************************************************************

CAnimatedGif::CAnimatedGif()
{
  Raster = NULL;
  Palette = NULL;
  pbmi = NULL;
  nLoops = 1; //default=play animation 1 time
}

CAnimatedGif::~CAnimatedGif()
{
  delete [] pbmi;
  delete [] Raster;
}

#ifdef _XBOX 
// Round a number to the nearest power of 2 rounding up
// runs pretty quickly - the only expensive op is the bsr
// alternive would be to dec the source, round down and double the result
// which is slightly faster but rounds 1 to 2
DWORD __forceinline __stdcall PadPow2(DWORD x)
{
  __asm {
    mov edx, x    // put the value in edx
    xor ecx, ecx  // clear ecx - if x is 0 bsr doesn't alter it
    bsr ecx, edx  // find MSB position
    mov eax, 1    // shift 1 by result effectively
    shl eax, cl   // doing a round down to power of 2
    cmp eax, edx  // check if x was already a power of two
    adc ecx, 0    // if it wasn't then CF is set so add to ecx
    mov eax, 1    // shift 1 by result again, this does a round
    shl eax, cl   // up as a result of adding CF to ecx
  }
  // return result in eax
}
#endif

// Init: Allocates space for raster and palette in GDI-compatible structures.
void CAnimatedGif::Init(int iWidth, int iHeight, int iBPP, int iLoops)
{
  if (Raster)
  {
    delete[] Raster;
    Raster = NULL;
  }

  if (pbmi)
  {
    delete[] pbmi;
    pbmi = NULL;
  }
  // Standard members setup
  Transparent = -1;
#ifdef _XBOX
  BytesPerRow = PadPow2(Width = iWidth);
#else
  BytesPerRow = Width = iWidth;
#endif
  Height = iHeight;
  BPP = iBPP;
  // Animation Extra members setup:
  xPos = xPos = Delay = 0;
  nLoops = iLoops;

  if (BPP == 24)
  {
    BytesPerRow *= 3;
    pbmi = (GUIBITMAPINFO*)new char [sizeof(GUIBITMAPINFO)];
  }
  else
  {
    pbmi = (GUIBITMAPINFO*)new char[sizeof(GUIBITMAPINFOHEADER) + (1 << BPP) * sizeof(COLOR)];
    Palette = (COLOR*)((char*)pbmi + sizeof(GUIBITMAPINFOHEADER));
  }

#ifndef _XBOX // Not needed as already fixed to power of two
  BytesPerRow += (ALIGN - Width % ALIGN) % ALIGN; // Align BytesPerRow
  int size = BytesPerRow * Height;
#else
  // align to multiple of 4096 for XGSwizzleRect
  int size = BytesPerRow * Height;
  size += (4096 - size % 4096) % 4096;  // align size
#endif

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

#if !defined(_XBOX) && !defined(_LINUX)
// GDIPaint: Paint the raster image onto a DC
int CAnimatedGif::GDIPaint (HDC hdc, int x, int y)
{
  return SetDIBitsToDevice (hdc, x, y, Width, Height, 0, 0, 0, Height, (LPVOID)Raster, pbmi, 0);
}
#endif

// operator=: copies an object's content to another
CAnimatedGif& CAnimatedGif::operator = (CAnimatedGif& rhs)
{
  Init(rhs.Width, rhs.Height, rhs.BPP); // respects virtualization
  memcpy(Raster, rhs.Raster, BytesPerRow*Height);
  memcpy((char*)Palette, (char*)rhs.Palette, (1 << BPP)*sizeof(*Palette));
  return *this;
}



CAnimatedGifSet::CAnimatedGifSet()
{
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
  fread(&uchar, 1, 1, fd);
  return uchar;
}

#ifndef _LINUX
extern "C" void dllprintf( const char *format, ... );
#else
#define dllprintf printf
#endif

// ****************************************************************************
// * LoadGIF                                                                  *
// *   Load a GIF File into the CAnimatedGifSet object                             *
// *                        (c) Nov 2000, Juan Soulie <jsoulie@cplusplus.com> *
// ****************************************************************************
int CAnimatedGifSet::LoadGIF (const char * szFileName)
{
  try
  {
    int n;
    //  dllprintf("load:%s",szFileName);
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
    FILE *fd = fopen(szFileName, "rb");
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
    //dllprintf("header:%s", szSignature);
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
      }

    else // GIF standard says to provide an internal default Palette:
      for (n = 0;n < 256;n++)
        GlobalColorMap[n].r = GlobalColorMap[n].g = GlobalColorMap[n].b = n;

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
        //      dllprintf("Extension Block:%x",extensionType);
        switch (extensionType)
        {

        case 0xF9:    // Graphic Control Extension
          {
            fread((char*)&gifgce, 1, sizeof(gifgce), fd);
            //dllprintf("got Graphic Control Extension:%i/%i fields:%x",gifgce.BlockSize,sizeof(gifgce),gifgce.PackedFields);
            GraphicExtensionFound++;
            getbyte(fd); // Block Terminator (always 0)
          }
          break;

        case 0xFE:    // Comment Extension: Ignored
          {
            //            OutputDebugString("got Comment Extension\n");
            while (int nBlockLength = getbyte(fd))
              for (n = 0;n < nBlockLength;n++) getbyte(fd);
          }
          break;

        case 0x01:    // PlainText Extension: Ignored
          {
            //          OutputDebugString("got PlainText Extension\n");
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
              fread((char*)&tag, 1, sizeof(gifnetscape), fd);
              nLoops = tag.iIterations;
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
            dllprintf("got unknown extension:%x", extensionType);
            // read (and ignore) data sub-blocks
            while (int nBlockLength = getbyte(fd))
              for (n = 0;n < nBlockLength;n++) getbyte(fd);
          }
          break;
        }
      }
      else if (charGot == 0x2c)
      { // *B* IMAGE (0x2c Image Separator)
        //      OutputDebugString("image seperator\n");
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

        fread((char*)&gifid, 1, sizeof(gifid), fd);

        int LocalColorMap = (gifid.PackedFields & 0x08) ? 1 : 0;

        NextImage->Init (gifid.Width, gifid.Height, LocalColorMap ? (gifid.PackedFields&7) + 1 : GlobalBPP);

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

        if (LocalColorMap)  // Read Color Map (if descriptor says so)
          fread((char*)NextImage->Palette, 1, sizeof(COLOR)*(1 << NextImage->BPP), fd);

        else     // Otherwise copy Global
          memcpy(NextImage->Palette, GlobalColorMap, sizeof(COLOR)*(1 << NextImage->BPP));

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
          fread(pTemp, 1, nBlockLength, fd);
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
          ERRORMSG("GIF File Corrupt");
        }

        // Some cleanup
        delete[] pCompressedImage;
        GraphicExtensionFound = 0;
      }
      else if (charGot == 0x3b)
      {
        //      OutputDebugString("end of gif marker\n");
        // *C* TRAILER: End of GIF Info
        break; // Ok. Standard End.
      }
      else
      {
        dllprintf("unknown marker:%x\n", charGot);
      }

    }
    while ( !feof(fd) );

    delete[] GlobalColorMap;
    fclose(fd);
    if ( GetImageCount() == 0) ERRORMSG("Premature End Of File");
    return GetImageCount();
  }
  catch (...)
  {
    OutputDebugString("Exception in CAnimatedGifSet::Load(");
    OutputDebugString(szFileName);
    OutputDebugString(")\n");
  }

  return 0;
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
  short Prefix[4096];    // Prefix: index of another Code
  unsigned char Suffix[4096];    // Suffix: terminating character
  short FirstEntry;     // Index of first free entry in table
  short NextEntry;     // Index of next free entry in table

  unsigned char OutStack[4097];   // Output buffer
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

  while (nPixels < maxPixels)
  {
    OutIndex = 0;       // Reset Output Stack

    // GET NEXT CODE FROM bufIn:
    // LZW compression uses code items longer than a single byte.
    // For GIF Files, code sizes are variable between 9 and 12 bits
    // That's why we must read data (Code) this way:
    LongCode = *((long*)(bufIn + whichBit / 8));     // Get some bytes from bufIn
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
      if (OutIndex > 4096)
        return 0;
      OutStack[OutIndex++] = Suffix[OutCode]; // Add suffix to Output Stack
      OutCode = Prefix[OutCode];       // Loop with preffix
    }

    // NOW OutCode IS A RAW CODE, ADD IT TO OUTPUT STACK.
    if (OutIndex > 4096)
      return 0;
    OutStack[OutIndex++] = (unsigned char) OutCode;

    // ADD NEW ENTRY TO TABLE (PrevCode + OutCode)
    // (EXCEPT IF PREVIOUS CODE WAS A CLEARCODE)
    if (PrevCode != ClearCode)
    {
      Prefix[NextEntry] = PrevCode;
      Suffix[NextEntry] = (unsigned char) OutCode;
      NextEntry++;

      // Prevent Translation table overflow:
      if (NextEntry >= 4096)
        return 0;

      // INCREASE CodeSize IF NextEntry IS INVALID WITH CURRENT CodeSize
      if (NextEntry >= (1 << CodeSize))
      {
        if (CodeSize < 12) CodeSize++;
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
