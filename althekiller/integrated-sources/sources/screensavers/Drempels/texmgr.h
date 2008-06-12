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

#ifndef GEISS_TEXTURE_MANAGER
#define GEISS_TEXTURE_MANAGER 1

#define NUM_TEX 4

struct td_filenode
{
	char *szFilename;
	td_filenode *next;
};

class texmgr
{
public:
	texmgr();
	~texmgr();
	unsigned char *tex[NUM_TEX];
	int texW[NUM_TEX];
	int texH[NUM_TEX];
	bool LoadBuiltInTex256(int iSlot);
	bool LoadTex256(char *szFilename, int iSlot, bool bResize, bool bAutoBlend, int iBlendPercent);

	bool LoadJpg256(char *szFilename, int iSlot, bool bResize, bool bAutoBlend, int iBlendPercent);
	void BlendTex(int src1, int src2, int dest, float t, bool bMMX);
	void SwapTex(int s1, int s2);
	bool EnumTgaAndBmpFiles(char *szFileDir);
	char* GetRandomFilename();
private:
	unsigned char *orig_tex[NUM_TEX];
	td_filenode *files;
	int iNumFiles;
    void BlendEdges256(int iSlot, int iBlendPercent);
};



#endif