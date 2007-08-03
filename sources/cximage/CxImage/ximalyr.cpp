// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
// xImaLyr.cpp : Layers functions
/* 21/04/2003 v1.00 - ing.davide.pizzolato@libero.it
 * CxImage version 5.80 29/Sep/2003
 */

#include "ximage.h"

#if CXIMAGE_SUPPORT_LAYERS

////////////////////////////////////////////////////////////////////////////////
bool CxImage::LayerCreate(long position)
{
	if ( position < 0 || position > info.nNumLayers ) position = info.nNumLayers;

	CxImage** ptmp = (CxImage**)malloc((info.nNumLayers + 1)*sizeof(CxImage**));
	if (ptmp==0) return false;

	int i=0;
	for (int n=0; n<info.nNumLayers; n++){
		if (position == n){
			ptmp[n] = new CxImage();
			i=1;
		}
		ptmp[n+i]=pLayers[n];
	}
	if (i==0) ptmp[info.nNumLayers] = new CxImage();

	if (ptmp[position]){
		ptmp[position]->info.pParent = this;
	} else {
		free(ptmp);
		return false;
	}

	info.nNumLayers++;
	if (pLayers) free(pLayers);
	pLayers = ptmp;
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::LayerDelete(long position)
{
	if ( position >= info.nNumLayers ) return false;
	if ( position < 0) position = info.nNumLayers - 1;

	CxImage** ptmp = (CxImage**)malloc((info.nNumLayers - 1)*sizeof(CxImage**));
	if (ptmp==0) return false;

	int i=0;
	for (int n=0; n<(info.nNumLayers - 1); n++){
		if (position == n){
			delete pLayers[n];
			i=1;
		}
		ptmp[n]=pLayers[n+i];
	}
	if (i==0) delete pLayers[info.nNumLayers - 1];

	info.nNumLayers--;
	if (pLayers) free(pLayers);
	pLayers = ptmp;
	return true;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::LayerDeleteAll()
{
	if (pLayers) { 
		for(long n=0; n<info.nNumLayers;n++){ delete pLayers[n]; }
		free(pLayers); pLayers=0;
	}
}
////////////////////////////////////////////////////////////////////////////////
CxImage* CxImage::GetLayer(long position)
{
	if ( position >= info.nNumLayers ) return 0;
	if ( position < 0) position = info.nNumLayers - 1;
	return pLayers[position];
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_LAYERS
