/* 
 *  This source file is part of Drempels, a program that falls
 *  under the Gnu Public License (GPL) open-source license.  
 *  Use and distribution of this code is regulated by law under 
 *  this license.  For license details, visit:
 *    http://www.gnu.org/copyleft/gpl.html
 * 
 *  The Drempels open-source project is accessible at 
 *  sourceforge.net; the direct URL is:
 *    http://sourceforge.net/projects/drempels/
 *  
 *  Drempels was originally created by Ryan M. Geiss in 2001
 *  and was open-sourced in February 2005.  The original
 *  Drempels homepage is available here:
 *    http://www.geisswerks.com/drempels/
 *
 */

#include <xtl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <io.h>
#include <string.h>
#include "texmgr.h"
#include <assert.h>

extern LPDIRECT3DDEVICE8       g_pd3dDevice;

/*

#include "setjmp.h"

extern "C" {                    // NOTE: NEED TO EXTERN "C" JPEGLIB.H 
    #include "jpeg/jpeglib.h"   // or you'll get "unresolved external symbol" errors are link time!
}

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/ * "public" fields * /
  jmp_buf setjmp_buffer;	/ * for return to caller * /
};*/


//typedef struct my_error_mgr * my_error_ptr;
 



//#define max(a,b)    (((a) > (b)) ? (a) : (b))
//#define min(a,b)    (((a) > (b)) ? (b) : (a))



texmgr::texmgr()
{
	for (int i=0; i<NUM_TEX; i++)
	{
		tex[i] = NULL;
		orig_tex[i] = NULL;
	}

	iNumFiles = 0;
	files = NULL;
}

texmgr::~texmgr()
{
	for (int i=0; i<NUM_TEX; i++)
	{
		if (orig_tex[i])
		{
			delete orig_tex[i];
			tex[i] = NULL;
			orig_tex[i] = NULL;
		}
	}

	while (files)
	{
		td_filenode *pNode = files->next;
		if (files->szFilename) delete files->szFilename;
		delete files;
		files = pNode;
	}
}


char* texmgr::GetRandomFilename()
{
	if (!files) return NULL;
	if (iNumFiles == 0) return NULL;

	int iFileNum = rand() % iNumFiles;
	int i;
	td_filenode *pNode = files;
	
	for (i = 0; i < iFileNum; i++)
	{
		if (!pNode) return NULL;
		pNode = pNode->next;
	}

	if (!pNode) return NULL;
	
	return pNode->szFilename;
}


bool texmgr::EnumTgaAndBmpFiles(char *szFileDir)
{
	struct _finddata_t c_file;
	long hFile, hFile2;

	char *szMask = new char[strlen(szFileDir) + 32];
	char *szPath = new char[strlen(szFileDir) + 32];

	strcpy(szMask, szFileDir);
	if (szMask[strlen(szMask)-1] != '\\')	
		strcat(szMask, "\\*.tga");
	else
		strcat(szMask, "*.tga");

	strcpy(szPath, szFileDir);
	if (szPath[strlen(szPath)-1] != '\\') 
	{
		strcat(szPath, "\\");
	}

	// clean up old list
	while (files)
	{
		td_filenode *pNode = files->next;
		if (files->szFilename) delete files->szFilename;
		delete files;
		files = pNode;
	}
	iNumFiles = 0;

	/* Find first .TGA file */
	if( (hFile = _findfirst(szMask, &c_file )) != -1L )
	{
		// start new list
        if (files == NULL)
        {
		    files = new td_filenode;
		    files->szFilename = new char[strlen(c_file.name) + strlen(szPath) + 1];
		    files->next = NULL;
		    strcpy(files->szFilename, szPath);
		    strcat(files->szFilename, c_file.name);
		    iNumFiles++;
        }
        else
        {
            td_filenode *temp = new td_filenode;
            temp->next = files;
			files = temp;
			files->szFilename = new char[strlen(c_file.name) + strlen(szPath) + 1];
			strcpy(files->szFilename, szPath);
			strcat(files->szFilename, c_file.name);
			iNumFiles++;
        }

		/* Find the rest of the .BMP files */
		while( _findnext( hFile, &c_file ) == 0 )
		{
            td_filenode *temp = new td_filenode;
            temp->next = files;
			files = temp;
			files->szFilename = new char[strlen(c_file.name) + strlen(szPath) + 1];
			strcpy(files->szFilename, szPath);
			strcat(files->szFilename, c_file.name);
			iNumFiles++;
		}

		_findclose( hFile );
	}

	strcpy(szMask, szFileDir);
	if (szMask[strlen(szMask)-1] != '\\')	
		strcat(szMask, "\\*.bmp");
	else
		strcat(szMask, "*.bmp");

	/* Find first .BMP file */
	if( (hFile = _findfirst(szMask, &c_file )) != -1L )
	{
        td_filenode *pNode = files;

		// start new list
        if (files == NULL)
        {
		    files = new td_filenode;
		    files->szFilename = new char[strlen(c_file.name) + strlen(szPath) + 1];
		    files->next = NULL;
		    strcpy(files->szFilename, szPath);
		    strcat(files->szFilename, c_file.name);
		    iNumFiles++;
        }
        else
        {
            td_filenode *temp = new td_filenode;
            temp->next = files;
			files = temp;
			files->szFilename = new char[strlen(c_file.name) + strlen(szPath) + 1];
			strcpy(files->szFilename, szPath);
			strcat(files->szFilename, c_file.name);
			iNumFiles++;
        }

		/* Find the rest of the .BMP files */
		while( _findnext( hFile, &c_file ) == 0 )
		{
            td_filenode *temp = new td_filenode;
            temp->next = files;
			files = temp;
			files->szFilename = new char[strlen(c_file.name) + strlen(szPath) + 1];
			strcpy(files->szFilename, szPath);
			strcat(files->szFilename, c_file.name);
			iNumFiles++;
		}

		_findclose( hFile );
	}

	strcpy(szMask, szFileDir);
	if (szMask[strlen(szMask)-1] != '\\')	
		strcat(szMask, "\\*.jpg");
	else
		strcat(szMask, "*.jpg");

	/* Find first .JPG file */
	if( (hFile = _findfirst(szMask, &c_file )) != -1L )
	{
        td_filenode *pNode = files;

		// start new list
        if (files == NULL)
        {
		    files = new td_filenode;
		    files->szFilename = new char[strlen(c_file.name) + strlen(szPath) + 1];
		    files->next = NULL;
		    strcpy(files->szFilename, szPath);
		    strcat(files->szFilename, c_file.name);
		    iNumFiles++;
        }
        else
        {
            td_filenode *temp = new td_filenode;
            temp->next = files;
			files = temp;
			files->szFilename = new char[strlen(c_file.name) + strlen(szPath) + 1];
			strcpy(files->szFilename, szPath);
			strcat(files->szFilename, c_file.name);
			iNumFiles++;
        }

		/* Find the rest of the .JPG files */
		while( _findnext( hFile, &c_file ) == 0 )
		{
            td_filenode *temp = new td_filenode;
            temp->next = files;
			files = temp;
			files->szFilename = new char[strlen(c_file.name) + strlen(szPath) + 1];
			strcpy(files->szFilename, szPath);
			strcat(files->szFilename, c_file.name);
			iNumFiles++;
		}

		_findclose( hFile );
	}

    // no files found case:
	delete szMask;
	delete szPath;
	return (iNumFiles != 0);    // false if no files found
}


void texmgr::SwapTex(int s1, int s2)
{
	unsigned char *pOrig = orig_tex[s1];
	unsigned char *pTex  = tex[s1];
	unsigned int iW      = texW[s1];
	unsigned int iH      = texH[s1];

	orig_tex[s1] 	= orig_tex[s2];
	tex[s1] 		= tex[s2];
	texW[s1] 		= texW[s2];
	texH[s1] 		= texH[s2];

	orig_tex[s2]	= pOrig;
	tex[s2]			= pTex;
	texW[s2]		= iW;
	texH[s2]		= iH;
}

void texmgr::BlendTex(int src1, int src2, int dest, float t, bool bMMX)
{
	if (tex[src1] && 
		tex[src2] && 
		(texW[src1] == texW[src2]) &&
		(texH[src1] == texH[src2]))
	{
		int end = texW[src1]*texH[src1];
		int m1 = (int)((1.0f-t)*255);
		int m2 = (int)(t*255);
		
		if (tex[dest] && 
			(texW[src1] != texW[dest] || texH[src1] != texH[dest]))
		{
			delete orig_tex[dest];
			orig_tex[dest] = NULL;
			tex[dest] = NULL;
		}

		if (!tex[dest])
		{
			orig_tex[dest] = new unsigned char[ texW[src1]*texH[src1]*4 + 16 ];
			tex[dest] = orig_tex[dest];
			if (((unsigned long)(orig_tex[dest])) % 8 != 0)
			{
				//align tex buffer to 8-byte boundary
				tex[dest] = (unsigned char *)((((unsigned long)(orig_tex[dest]))/8 + 1) * 8);
			}
			texW[dest] = texW[src1];
			texH[dest] = texH[src1];
		}

		unsigned char *s1 = (unsigned char *)tex[src1];
		unsigned char *s2 = (unsigned char *)tex[src2];
		unsigned char *d  = (unsigned char *)tex[dest];

		if (bMMX)
		{
			unsigned short mult1[4] = { m1, m1, m1, m1 };
			unsigned short mult2[4] = { m2, m2, m2, m2 };

			__asm
			{
				mov  eax, s1
				mov  ebx, s2
				mov  edi, d
				mov  ecx, end
				movq mm6, mult1
				movq mm7, mult2

				TexMixLoop:
					pxor mm0, mm0
					pxor mm1, mm1
					punpcklbw mm0, [eax]           //  00 aa 00 bb 00 gg 00 rr    
					punpcklbw mm1, [ebx]           //  00 aa 00 bb 00 gg 00 rr    
					 psrlw mm0,8
					 psrlw mm1,8
					pmullw    mm0, mm6
					pmullw    mm1, mm7
					paddusw   mm0, mm1
					psrlw     mm0, 8

					packuswb  mm0, mm0             ;    a'   b'   g'   r'   a'   b'   g'   r'
					 add       eax, 4
					 add       ebx, 4
					movd      dword ptr [edi], mm0 ; store to the destination array
					
					add       edi, 4
					dec       ecx
					jnz       TexMixLoop

				EMMS
			}
		}
		else
		{
			for (int i=0; i<end; i++)
			{
				*(d++) = (((*s1++) * m1) + ((*s2++) * m2)) >> 8;
				*(d++) = (((*s1++) * m1) + ((*s2++) * m2)) >> 8;
				*(d++) = (((*s1++) * m1) + ((*s2++) * m2)) >> 8;
				*(d++) = (((*s1++) * m1) + ((*s2++) * m2)) >> 8;
			}
		}
	}
}

bool texmgr::LoadTex256(char *szFilename, int iSlot, bool bResize, bool bAutoBlend, int iBlendPercent)
{
//  char txt[255];
//  sprintf(txt, "Drempels: Load texture %s\n", szFilename);
//  OutputDebugString(txt);
  if (iSlot < 0) return false;
  if (iSlot >= NUM_TEX) return false;

  // Allocate memory for image if not already allocated
  if (orig_tex[iSlot] == NULL)
  {
    orig_tex[iSlot] = new unsigned char[ 256*256*4 + 16 ];
    tex[iSlot]      = orig_tex[iSlot];
    if (((unsigned long)(orig_tex[iSlot])) % 8 != 0)
    {
      //align tex buffer to 8-byte boundary
      tex[iSlot] = (unsigned char *)((((unsigned long)(orig_tex[iSlot]))/8 + 1) * 8);
    }
  }
  /*

  // If it's a JPG, call that routine instead
  char *pExtension = strrchr(szFilename, '.');
  if (strcmpi(pExtension, ".jpg") == 0)
  {
  return LoadJpg256(szFilename, iSlot, bResize, bAutoBlend, iBlendPercent);
  }

  // Otherwise, load TGA or BMP file
  FILE *infile;
  if ((infile = fopen(szFilename, "rb")) != NULL)
  {
  unsigned char a,b,c,d,e,f,g,h;
  int W;
  int H;
  int bitdepth;
  int nPadBytesPerScanline = 0;

  fscanf(infile, "%c%c%c%c%c%c%c%c", &a,&b,&c,&d,&e,&f,&g,&h);
  if (a=='B' && b=='M')
  {
  // bmp file header
  fscanf(infile, "%c%c%c%c%c%c%c%c", &a,&b,&c,&d,&e,&f,&g,&h);
  fscanf(infile, "%c%c%c%c%c%c%c%c", &a,&b,&c,&d,&e,&f,&g,&h);
  W = c+d*256;
  H = g+h*256;
  fscanf(infile, "%c%c%c%c%c%c%c%c", &a,&b,&c,&d,&e,&f,&g,&h);
  bitdepth = e;
  fscanf(infile, "%c%c%c%c%c%c%c%c", &a,&b,&c,&d,&e,&f,&g,&h);
  fscanf(infile, "%c%c%c%c%c%c%c%c", &a,&b,&c,&d,&e,&f,&g,&h);
  fscanf(infile, "%c%c%c%c%c%c", &a,&b,&c,&d,&e,&f);

  // SPECIAL CASE: if a BMP file's width is 1 pixel shy of a multiple of 4,
  // the BMP is saved with the large width and junk in the last column,
  // EVEN THOUGH the header says it's the original size!

  int nBytesPerScanline = W*3; 

  if (nBytesPerScanline % 4)
  {
  nPadBytesPerScanline = 4 - (nBytesPerScanline % 4);
  }
  }
  else
  {
  // tga file header
  fscanf(infile, "%c%c%c%c", &a,&b,&c,&d);
  fscanf(infile, "%c%c%c%c%c%c", &a,&b,&c,&d,&e,&f);
  W = a+b*256;
  H = c+d*256;
  bitdepth  = e;
  }

  if (bitdepth != 24 || W >= 2048 || H >= 2048) 
  {
  fclose(infile);
  return false;
  }

  if (bResize || W < 256 || H < 256)
  {
  // read into dynamically-allocated buffer, then resize it

  unsigned char *temp_orig = new unsigned char[ W*H*4 + 16 ];
  unsigned char *temp_aligned = temp_orig;
  if (((unsigned long)(temp_orig)) % 8 != 0)
  {
  //align tex buffer to 8-byte boundary
  temp_aligned = (unsigned char *)((((unsigned long)(temp_orig))/8 + 1) * 8);
  }

  unsigned int tex_offset = 0;
  unsigned char buf[2048*3];
  int bufpos, y;

  // read in texture
  for (y=0; y<H; y++)
  {
  assert(tex_offset == y*W*4);
  int n = fread(buf, sizeof(unsigned char), W*3, infile);
  //assert (n == W*3);
  for (bufpos=0; bufpos<W; bufpos++)
  {
  temp_aligned[tex_offset  ] = buf[(bufpos*3)  ];//bufpos;
  temp_aligned[tex_offset+1] = buf[(bufpos*3)+1];//y;
  temp_aligned[tex_offset+2] = buf[(bufpos*3)+2];
  tex_offset += 4;
  }

  if (nPadBytesPerScanline > 0)
  {
  fread(buf, sizeof(unsigned char), nPadBytesPerScanline, infile);
  }
  }
  assert(tex_offset == W*H*4);

  // now resize into the 256x256 buffer, and clean up the big temp_ buffer.
  int dest_offset = 0;

  int src_y, dy, dy_inv;
  int src_x, dx, dx_inv;
  int src_offset;
  int c;
  int src_scanline_offset, src_offset_UL;
  bool bLarge = ((W+H)/2 > 256);

  if (bLarge)
  {
  for (int y=0; y<256; y++)
  {
  src_y = (y*(H)) / 256;
  src_scanline_offset = src_y*W;

  for (int x=0; x<256; x++)
  {
  src_x = (x*(W)) / 256;

  ((unsigned int *)tex[iSlot])[dest_offset++] = 
  ((unsigned int *)temp_aligned)[src_scanline_offset + src_x];

  //src_x = (x*(W)) / 256;
  //src_offset_UL = (src_scanline_offset + src_x)*4;
  //tex[iSlot][dest_offset++] = temp_aligned[src_offset_UL];
  //tex[iSlot][dest_offset++] = temp_aligned[src_offset_UL+1];
  //tex[iSlot][dest_offset++] = temp_aligned[src_offset_UL+2];
  // skip the padding byte:
  //dest_offset++;
  }
  }
  }
  else
  {
  for (int y=0; y<256; y++)
  {
  src_y = y*(H-1) / 256;
  dy = (y*(H-1)) - (src_y * 256);     // fraction, from 0..255
  dy_inv = 255 - dy;
  src_scanline_offset = src_y*W*4;

  for (int x=0; x<256; x++)
  {
  src_x = x*(W-1) / 256;
  dx = (x*(W-1)) - (src_x * 256);     // fraction, from 0..255
  dx_inv = 255 - dx;

  src_offset_UL = src_scanline_offset + src_x*4;

  c = (int)temp_aligned[src_offset_UL          ] * dx_inv * dy_inv + 
  (int)temp_aligned[src_offset_UL + 4      ] * dx * dy_inv + 
  (int)temp_aligned[src_offset_UL + W*4    ] * dx_inv * dy + 
  (int)temp_aligned[src_offset_UL + W*4 + 4] * dx * dy;
  tex[iSlot][dest_offset++] = (unsigned char)(c >> 16);

  c = (int)temp_aligned[src_offset_UL          +1] * dx_inv * dy_inv + 
  (int)temp_aligned[src_offset_UL + 4      +1] * dx * dy_inv + 
  (int)temp_aligned[src_offset_UL + W*4    +1] * dx_inv * dy + 
  (int)temp_aligned[src_offset_UL + W*4 + 4+1] * dx * dy;
  tex[iSlot][dest_offset++] = (unsigned char)(c >> 16);

  c = (int)temp_aligned[src_offset_UL          +2] * dx_inv * dy_inv + 
  (int)temp_aligned[src_offset_UL + 4      +2] * dx * dy_inv + 
  (int)temp_aligned[src_offset_UL + W*4    +2] * dx_inv * dy + 
  (int)temp_aligned[src_offset_UL + W*4 + 4+2] * dx * dy;
  tex[iSlot][dest_offset++] = (unsigned char)(c >> 16);

  // skip the padding byte:
  dest_offset++;
  }
  }
  }

  // free up the old (big) image
  delete [] temp_orig;
  }
  else
  {
  // don't resize - just clip the image (note: image is > 256x256)
  int x_net_skip = W - 256;
  int y_net_skip = H - 256;
  int xL = x_net_skip/2;	// in terms of image on disk
  int xR = x_net_skip/2 + 256;

  // skip any lines, if necessary
  if (y_net_skip/2 > 0)
  {
  int bytes_to_skip = (y_net_skip/2) * ( W*3 + nPadBytesPerScanline );
  fseek(infile, bytes_to_skip, SEEK_SET);
  }

  unsigned int tex_offset = 0;
  unsigned char buf[2048*3];
  int bufpos, y;

  // read in texture
  for (y=0; y<256; y++)
  {
  fread(buf, W*3, 1, infile);
  for (bufpos=xL*3; bufpos<xR*3; bufpos += 3)
  {
  tex[iSlot][tex_offset  ] = buf[bufpos  ];//((tex_offset/4) % W)/(float)W*255;
  tex[iSlot][tex_offset+1] = buf[bufpos+1];//((tex_offset/4) % W)/(float)W*255;
  tex[iSlot][tex_offset+2] = buf[bufpos+2];//((tex_offset/4) % W)/(float)W*255;
  tex_offset += 4;
  }

  if (nPadBytesPerScanline > 0)
  {
  fread(buf, sizeof(unsigned char), nPadBytesPerScanline, infile);
  }
  }
  }

  */
  // done loading new
  texW[iSlot]     = 256;
  texH[iSlot]     = 256;

  //		fclose(infile);
  LPDIRECT3DTEXTURE8 texture = NULL;
  D3DXCreateTextureFromFileEx(g_pd3dDevice, szFilename, 256, 256, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, 0x00000000, NULL, NULL, &texture);
  if (texture)
  {
    IDirect3DSurface8* surface;
    D3DLOCKED_RECT lock;

    texture->GetSurfaceLevel(0, &surface);
    surface->LockRect(&lock, NULL, 0);
    memcpy(tex[iSlot], lock.pBits, 256*256*4);
    surface->UnlockRect();
    surface->Release();
    texture->Release();
  }
  else
  {
    memset(tex[iSlot], 0, 256*256*4);
  }

//  if (bAutoBlend)
//  {
//    BlendEdges256(iSlot, iBlendPercent);
//  }

  return true;
}

    // could not open file
//    return false;
//}


/*
void my_error_exit(j_common_ptr cinfo)
{
  / * cinfo->err really points to a my_error_mgr struct, so coerce pointer * /
  my_error_ptr myerr = (my_error_ptr) cinfo->err;
  / * Always display the message. * /
  / * We could postpone this until after returning, if we chose. * /
  (*cinfo->err->output_message) (cinfo);
  / * Return control to the setjmp point * /
  longjmp(myerr->setjmp_buffer, 1);
}
*/


/*

bool texmgr::LoadJpg256(char *filename, int iSlot, bool bResize, bool bAutoBlend, int iBlendPercent)
{
    // if w>256 and h>256
    //      if bResize is true 
    //          the image will be scaled down to 256x256
    //      else
    //          the image will be clipped to 256x256
    // else 
    //      the image will be scaled to 256x256
    //      (at least one dimension is < 256; the other can be anything, and is stretched.

    / * This struct contains the JPEG decompression parameters and pointers to
    * working space (which is allocated as needed by the JPEG library).
    * /
    struct jpeg_decompress_struct cinfo;
    / * We use our private extension JPEG error handler.
    * Note that this struct must live as long as the main JPEG parameter
    * struct, to avoid dangling-pointer problems.
    * /
    struct my_error_mgr jerr;
    / * More stuff * /
    FILE * infile;		/ * source file * /
    JSAMPARRAY buffer;		/ * Output row buffer * /
    int row_stride;		/ * physical row width in output buffer * /
                        / * In this example we want to open the input file before doing anything else,
                        * so that the setjmp() error recovery below can assume the file is open.
                        * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
                        * requires it in order to read binary files.
    * /
    if ((infile = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "can't open %s\n", filename);
        return false;
    }
    / * Step 1: allocate and initialize JPEG decompression object * /
    / * We set up the normal JPEG error routines, then override error_exit. * /
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    / * Establish the setjmp return context for my_error_exit to use. * /
    if (setjmp(jerr.setjmp_buffer)) {
    / * If we get here, the JPEG code has signaled an error.
    * We need to clean up the JPEG object, close the input file, and return.
        * /
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return false;
    }
    / * Now we can initialize the JPEG decompression object. * /
    jpeg_create_decompress(&cinfo);
    / * Step 2: specify data source (eg, a file) * /
    jpeg_stdio_src(&cinfo, infile);

    / * Step 3: read file parameters with jpeg_read_header() * /
    (void) jpeg_read_header(&cinfo, TRUE);
    / * We can ignore the return value from jpeg_read_header since
    *   (a) suspension is not possible with the stdio data source, and
    *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
    * See libjpeg.doc for more info.
    * /
    / * Step 4: set parameters for decompression * /
    / * In this example, we don't need to change any of the defaults set by
    * jpeg_read_header(), so we do nothing here.
    * /
    / * Step 5: Start decompressor * /
    (void) jpeg_start_decompress(&cinfo);
    / * We can ignore the return value since suspension is not possible
    * with the stdio data source.
    * /
    / * We may need to do some setup of our own at this point before reading
    * the data.  After jpeg_start_decompress() we have the correct scaled
    * output image dimensions available, as well as the output colormap
    * if we asked for color quantization.
    * In this example, we need to make an output work buffer of the right size.
    * / 
    / * JSAMPLEs per row in output buffer * /
    row_stride = cinfo.output_width * cinfo.output_components;
    / * Make a one-row-high sample array that will go away when done with image * /
    buffer = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
    / * Step 6: while (scan lines remain to be read) * /
    / *           jpeg_read_scanlines(...); * /
    / * Here we use the library's state variable cinfo.output_scanline as the
    * loop counter, so that we don't have to keep track ourselves.
    * /

    if ((cinfo.output_components != 3) || 
        (cinfo.image_width > 2048) ||
        (cinfo.image_height > 2048))   // RG
    {
        // error: not 24-bit!
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return false;
    }

    / *
    if (cinfo.output_width != 256 || cinfo.output_height != 256)   // RG
    {
        // error: not 256x256
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return false;
    }
    * /

    int w = cinfo.output_width;
    int h = cinfo.output_height;

    if (w == 256 && h == 256)
    {
        // read directly into 256x256 buffer

	    unsigned int tex_offset = 0;
        int y = h - 1;

        while (cinfo.output_scanline < h) 
        {
            jpeg_read_scanlines(&cinfo, buffer, 1);

            tex_offset = w*4 * y;

		    for (int x=0; x<w; x++)
		    {
			    tex[iSlot][tex_offset  ] = buffer[0][x*3+2];//((tex_offset/4) % W)/(float)W*255;
			    tex[iSlot][tex_offset+1] = buffer[0][x*3+1];//((tex_offset/4) % W)/(float)W*255;
			    tex[iSlot][tex_offset+2] = buffer[0][x*3  ];//((tex_offset/4) % W)/(float)W*255;
			    tex_offset += 4;
		    }

            y--;
        }
    }
    else
    {
	    // allocate temporary buffer
	    unsigned char *temp_orig = new unsigned char[ cinfo.output_width*cinfo.output_height*4 + 16 ];
	    unsigned char *temp_aligned = temp_orig;
	    if (((unsigned long)(temp_orig)) % 8 != 0)
	    {
		    //align tex buffer to 8-byte boundary
		    temp_aligned = (unsigned char *)((((unsigned long)(temp_orig))/8 + 1) * 8);
	    }
	    unsigned int tex_offset = 0;
        int y = cinfo.image_height - 1;

        while (cinfo.output_scanline < h) {
        / * jpeg_read_scanlines expects an array of pointers to scanlines.
        * Here the array is only one element long, but you could ask for
        * more than one scanline at a time if that's more convenient.
            * /
            (void) jpeg_read_scanlines(&cinfo, buffer, 1);
            / * Assume put_scanline_someplace wants a pointer and sample count. * /
            //put_scanline_someplace(buffer[0], row_stride);

            tex_offset = w*4 * y;

		    for (int x=0; x<w; x++)
		    {
			    temp_aligned[tex_offset  ] = buffer[0][x*3+2];//((tex_offset/4) % W)/(float)W*255;
			    temp_aligned[tex_offset+1] = buffer[0][x*3+1];//((tex_offset/4) % W)/(float)W*255;
			    temp_aligned[tex_offset+2] = buffer[0][x*3  ];//((tex_offset/4) % W)/(float)W*255;
			    tex_offset += 4;
		    }

            y--;
        }

        //--------------------------------------------------------------------
        // move 'temp_aligned[]' into 'tex[iSlot][]', rescaling or clipping to 256x256
        //--------------------------------------------------------------------
    
        if (bResize || w < 256 || h < 256)
        {
            int dest_offset = 0;

            int src_y, dy, dy_inv;
            int src_x, dx, dx_inv;
            int src_offset;
            int c;
            int src_scanline_offset, src_offset_UL;
            bool bLarge = ((w+h)/2 > 256);

            if (bLarge)
            {
                for (int y=0; y<256; y++)
                {
                    src_y = y*(h) / 256;
                    src_scanline_offset = src_y*w;

                    for (int x=0; x<256; x++)
                    {
                        src_x = (x*(w)) / 256;

                        ((unsigned int *)tex[iSlot])[dest_offset++] = 
                             ((unsigned int *)temp_aligned)[src_scanline_offset + src_x];

                        //src_x = (x*(W)) / 256;
                        //src_offset_UL = (src_scanline_offset + src_x)*4;
                        //tex[iSlot][dest_offset++] = temp_aligned[src_offset_UL];
                        //tex[iSlot][dest_offset++] = temp_aligned[src_offset_UL+1];
                        //tex[iSlot][dest_offset++] = temp_aligned[src_offset_UL+2];
                        // skip the padding byte:
                        //dest_offset++;
                    }
                }
            }
            else
            {
                for (int y=0; y<256; y++)
                {
                    src_y = y*(h-1) / 256;
                    dy = (y*(h-1)) - (src_y * 256);     // fraction, from 0..255
                    dy_inv = 255 - dy;
                    src_scanline_offset = src_y*w*4;

                    for (int x=0; x<256; x++)
                    {
                        src_x = x*(w-1) / 256;
                        dx = (x*(w-1)) - (src_x * 256);     // fraction, from 0..255
                        dx_inv = 255 - dx;

                        src_offset_UL = src_scanline_offset + src_x*4;

                        c = (int)temp_aligned[src_offset_UL          ] * dx_inv * dy_inv + 
                            (int)temp_aligned[src_offset_UL + 4      ] * dx * dy_inv + 
                            (int)temp_aligned[src_offset_UL + w*4    ] * dx_inv * dy + 
                            (int)temp_aligned[src_offset_UL + w*4 + 4] * dx * dy;
                        tex[iSlot][dest_offset++] = (unsigned char)(c >> 16);

                        c = (int)temp_aligned[src_offset_UL          +1] * dx_inv * dy_inv + 
                            (int)temp_aligned[src_offset_UL + 4      +1] * dx * dy_inv + 
                            (int)temp_aligned[src_offset_UL + w*4    +1] * dx_inv * dy + 
                            (int)temp_aligned[src_offset_UL + w*4 + 4+1] * dx * dy;
                        tex[iSlot][dest_offset++] = (unsigned char)(c >> 16);

                        c = (int)temp_aligned[src_offset_UL          +2] * dx_inv * dy_inv + 
                            (int)temp_aligned[src_offset_UL + 4      +2] * dx * dy_inv + 
                            (int)temp_aligned[src_offset_UL + w*4    +2] * dx_inv * dy + 
                            (int)temp_aligned[src_offset_UL + w*4 + 4+2] * dx * dy;
                        tex[iSlot][dest_offset++] = (unsigned char)(c >> 16);

                        // skip the padding byte:
                        dest_offset++;
                    }
                }
            }
        }
        else
        {
            // don't resize - just clip the image (note: image is > 256x256)
            int y_start = (h-256)/2;
            int y_end   = y_start + 256;
            int x_start = (w-256)/2;
            int x_end   = x_start + 256;

            for (int y=0; y<256; y++)
            {
                memcpy(&tex[iSlot][y*256*4], 
                       &temp_aligned[(y+y_start)*w*4 + x_start*4],
                       256*4);
            }
        }
        //--------------------------------------------------------------------

        // delete temp
        delete [] temp_orig;
    }

	// done loading new; now link to it, then delete old one.
	texW[iSlot]     = 256;
	texH[iSlot]     = 256;

    / * Step 7: Finish decompression * /
    (void) jpeg_finish_decompress(&cinfo);
    / * We can ignore the return value since suspension is not possible
    * with the stdio data source.
    * /
    / * Step 8: Release JPEG decompression object * /
    / * This is an important step since it will release a good deal of memory. * /
    jpeg_destroy_decompress(&cinfo);
    / * After finish_decompress, we can close the input file.
    * Here we postpone it until after no more JPEG errors are possible,
    * so as to simplify the setjmp error logic above.  (Actually, I don't
    * think that jpeg_destroy can do an error exit, but why assume anything...)
    * /
    fclose(infile);

    / * At this point you may want to check to see whether any corrupt-data
    * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
    * /
    / * And we're done! * /

    if (bAutoBlend)
    {
        BlendEdges256(iSlot, iBlendPercent);
    }

    return true;
}
*/
/*

void texmgr::BlendEdges256(int iSlot, int iBlendPercent)
{
	if (iSlot < 0) return;
	if (iSlot >= NUM_TEX) return;
    if (tex[iSlot] == NULL) return;

    int z = iBlendPercent*256/100;
    int non_z = 256 - z;
    int x, y;

    unsigned char temp[256*256*4];

    memcpy(temp, tex[iSlot], 256*256*4);

    int c2_precalx[256];
    
    for (x=0; x<256; x++)
    {
        //c2_precalx[x] = 128 - (x * 128 / z);
        c2_precalx[x] = 128 - 128*powf(x / (float)z, 1.0f);
    }

    for (y=0; y<256; y++)
    {
        for (x=0; x<z; x++)
        {
            int c2 = c2_precalx[x];        // 128 -> 0
            int c1 = 256 - c2;              // 128 -> 256
            
            int offset  = y*256+x;
            int offset2 = y*256+255-x;
            tex[iSlot][offset*4  ] = (temp[offset*4  ]*c1 + temp[offset2*4  ]*c2)/256;
            tex[iSlot][offset*4+1] = (temp[offset*4+1]*c1 + temp[offset2*4+1]*c2)/256;
            tex[iSlot][offset*4+2] = (temp[offset*4+2]*c1 + temp[offset2*4+2]*c2)/256;

            tex[iSlot][offset2*4  ] = (temp[offset*4  ]*c2 + temp[offset2*4  ]*c1)/256;
            tex[iSlot][offset2*4+1] = (temp[offset*4+1]*c2 + temp[offset2*4+1]*c1)/256;
            tex[iSlot][offset2*4+2] = (temp[offset*4+2]*c2 + temp[offset2*4+2]*c1)/256;
        }
    }

    memcpy(temp, tex[iSlot], 256*256*4);

    for (y=0; y<z; y++)
    {
        for (x=0; x<256; x++)
        {
            int c2 = c2_precalx[y];         // 128 -> 0
            int c1 = 256 - c2;              // 128 -> 256
            
            int offset  = y*256+x;
            int offset2 = (255-y)*256+x;
            tex[iSlot][offset*4  ] = (temp[offset*4  ]*c1 + temp[offset2*4  ]*c2)/256;
            tex[iSlot][offset*4+1] = (temp[offset*4+1]*c1 + temp[offset2*4+1]*c2)/256;
            tex[iSlot][offset*4+2] = (temp[offset*4+2]*c1 + temp[offset2*4+2]*c2)/256;

            tex[iSlot][offset2*4  ] = (temp[offset*4  ]*c2 + temp[offset2*4  ]*c1)/256;
            tex[iSlot][offset2*4+1] = (temp[offset*4+1]*c2 + temp[offset2*4+1]*c1)/256;
            tex[iSlot][offset2*4+2] = (temp[offset*4+2]*c2 + temp[offset2*4+2]*c1)/256;
        }
    }
    
}

*/








