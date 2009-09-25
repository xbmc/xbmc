// --- CHUI EN TRAIN DE SUPPRIMER LES EXTERN RESOLX ET C_RESOLY ---

/* filter.c version 0.7
* contient les filtres applicable a un buffer
* creation : 01/10/2000
*  -ajout de sinFilter()
*  -ajout de zoomFilter()
*  -copie de zoomFilter() en zoomFilterRGB(), gerant les 3 couleurs
*  -optimisation de sinFilter (utilisant une table de sin)
*	-asm
*	-optimisation de la procedure de generation du buffer de transformation
*		la vitesse est maintenant comprise dans [0..128] au lieu de [0..100]
*/

/* #define _DEBUG_PIXEL */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <inttypes.h>

#include "goom_filters.h"
#include "goom_graphic.h"
#include "goom_tools.h"
#include "goom_plugin_info.h"
#include "goom_fx.h"
#include "v3d.h"

/* TODO : MOVE THIS AWAY !!! */
/* jeko: j'ai essayer de le virer, mais si on veut les laisser inline c'est un peu lourdo... */
static inline void setPixelRGB (PluginInfo *goomInfo, Pixel *buffer, Uint x, Uint y, Color c)
{
    Pixel i;
    
    i.channels.b = c.b;
    i.channels.g = c.v;
    i.channels.r = c.r;
    *(buffer + (x + y * goomInfo->screen.width)) = i;
}

static inline void setPixelRGB_ (Pixel *buffer, Uint x, Color c)
{
    buffer[x].channels.r = c.r;
    buffer[x].channels.g = c.v;
    buffer[x].channels.b = c.b;
}

static inline void getPixelRGB (PluginInfo *goomInfo, Pixel *buffer, Uint x, Uint y, Color * c)
{
    Pixel i = *(buffer + (x + y * goomInfo->screen.width));
    c->b = i.channels.b;
    c->v = i.channels.g;
    c->r = i.channels.r;
}

static inline void getPixelRGB_ (Pixel *buffer, Uint x, Color * c)
{
    Pixel i = *(buffer + x);
    c->b = i.channels.b;
    c->v = i.channels.g;
    c->r = i.channels.r;
}
/* END TODO */


/* DEPRECATED */
// retourne x>>s , en testant le signe de x
//#define ShiftRight(_x,_s) (((_x)<0) ? -(-(_x)>>(_s)) : ((_x)>>(_s)))
//#define EFFECT_DISTORS 4
//#define EFFECT_DISTORS_SL 2
//#define INTERLACE_ADD 9
//#define INTERLACE_AND 0xf
/* END DEPRECATED */

#define BUFFPOINTNB 16
#define BUFFPOINTNBF 16.0f
#define BUFFPOINTMASK 0xffff

#define sqrtperte 16
/* faire : a % sqrtperte <=> a & pertemask */
#define PERTEMASK 0xf
/* faire : a / sqrtperte <=> a >> PERTEDEC */
#define PERTEDEC 4

/* pure c version of the zoom filter */
static void c_zoom (Pixel *expix1, Pixel *expix2, unsigned int prevX, unsigned int prevY, signed int *brutS, signed int *brutD, int buffratio, int precalCoef[BUFFPOINTNB][BUFFPOINTNB]);

/* simple wrapper to give it the same proto than the others */
void zoom_filter_c (int sizeX, int sizeY, Pixel *src, Pixel *dest, int *brutS, int *brutD, int buffratio, int precalCoef[16][16]) {
    c_zoom(src, dest, sizeX, sizeY, brutS, brutD, buffratio, precalCoef);
}

static void generatePrecalCoef (int precalCoef[BUFFPOINTNB][BUFFPOINTNB]);


typedef struct _ZOOM_FILTER_FX_WRAPPER_DATA {
    
    PluginParam enabled_bp;
    PluginParameters params;
    
    unsigned int *coeffs, *freecoeffs;
    
    signed int *brutS, *freebrutS; /* source */
    signed int *brutD, *freebrutD; /* dest */
    signed int *brutT, *freebrutT; /* temp (en cours de generation) */
    
    guint32 zoom_width;
    
    unsigned int prevX, prevY;
    
    float general_speed;
    int reverse; /* reverse the speed */
    char theMode;
    int waveEffect;
    int hypercosEffect;
    int vPlaneEffect;
    int hPlaneEffect;
    char noisify;
    int middleX, middleY;
    
    int mustInitBuffers;
    int interlace_start;
    
    /** modif by jeko : fixedpoint : buffration = (16:16) (donc 0<=buffration<=2^16) */
    int buffratio;
    int *firedec;
    
    /** modif d'optim by Jeko : precalcul des 4 coefs resultant des 2 pos */
    int precalCoef[BUFFPOINTNB][BUFFPOINTNB];
    
    /** calculatePXandPY statics */
    int wave;
    int wavesp;
    
} ZoomFilterFXWrapperData;




static inline v2g zoomVector(ZoomFilterFXWrapperData *data, float X, float Y)
{
    v2g vecteur;
    float vx, vy;
    float sq_dist = X*X + Y*Y;
    
    /*    sx = (X < 0.0f) ? -1.0f : 1.0f;
    sy = (Y < 0.0f) ? -1.0f : 1.0f;
    */  
    float coefVitesse = (1.0f+ data->general_speed) / 50.0f;
    
    // Effects
    
    /* Centralized FX */
    
    switch (data->theMode) {
        case CRYSTAL_BALL_MODE:
            coefVitesse -= (sq_dist-0.3f)/15.0f;
            break;
        case AMULETTE_MODE:
            coefVitesse += sq_dist * 3.5f;
            break;
        case WAVE_MODE:
            coefVitesse += sin(sq_dist*20.0f) / 100.0f;
            break;
        case SCRUNCH_MODE:
            coefVitesse += sq_dist / 10.0f;
            break;
            //case HYPERCOS1_MODE:
            break;
            //case HYPERCOS2_MODE:
            break;
            //case YONLY_MODE:
            break;
        case SPEEDWAY_MODE:
            coefVitesse *= 4.0f * Y;
            break;
        default:
            break;
    }
    
    if (coefVitesse < -2.01f)
        coefVitesse = -2.01f;
    if (coefVitesse > 2.01f)
        coefVitesse = 2.01f;
    
    vx = coefVitesse * X;
    vy = coefVitesse * Y;
    
    /* Amulette 2 */
    // vx = X * tan(dist);
    // vy = Y * tan(dist);
    
    /* Rotate */
    //vx = (X+Y)*0.1;
    //vy = (Y-X)*0.1;
    
    
    // Effects adds-on
    
    /* Noise */
    if (data->noisify)
    {
        vx += (((float)random()) / ((float)RAND_MAX) - 0.5f) / 50.0f;
        vy += (((float)random()) / ((float)RAND_MAX) - 0.5f) / 50.0f;
    }
    
    /* Hypercos */
    if (data->hypercosEffect)
    {
        vx += sin(Y*10.0f)/120.0f;
        vy += sin(X*10.0f)/120.0f;
    }
    
    /* H Plane */
    if (data->hPlaneEffect) vx += Y * 0.0025f * data->hPlaneEffect;
    
    /* V Plane */
    if (data->vPlaneEffect) vy += X * 0.0025f * data->vPlaneEffect;
    
    /* TODO : Water Mode */
    //    if (data->waveEffect)

    vecteur.x = vx;
    vecteur.y = vy;
    
    return vecteur;
}


/*
 * Makes a stripe of a transform buffer (brutT)
 *
 * The transform is (in order) :
 * Translation (-data->middleX, -data->middleY)
 * Homothetie (Center : 0,0   Coeff : 2/data->prevX)
 */
static void makeZoomBufferStripe(ZoomFilterFXWrapperData * data, int INTERLACE_INCR)
{
    // Position of the pixel to compute in pixmap coordinates
    Uint x, y;
    // Where (verticaly) to stop generating the buffer stripe
    int maxEnd = (data->interlace_start + INTERLACE_INCR);
    // Ratio from pixmap to normalized coordinates
    float ratio = 2.0f/((float)data->prevX);
    // Ratio from normalized to virtual pixmap coordinates
    float inv_ratio = BUFFPOINTNBF/ratio;
    float min = ratio/BUFFPOINTNBF;
    // Y position of the pixel to compute in normalized coordinates
    float Y = ((float)(data->interlace_start - data->middleY)) * ratio;
    
    maxEnd = data->prevY;
    if (maxEnd > (data->interlace_start + INTERLACE_INCR))
        maxEnd = (data->interlace_start + INTERLACE_INCR);
    
    for (y = data->interlace_start; (y < data->prevY) && ((signed int)y<maxEnd); y++) {
        Uint premul_y_prevX = y * data->prevX * 2;
        float X = - ((float)data->middleX) * ratio;
        for (x = 0; x < data->prevX; x++)
        {
            v2g vector = zoomVector (data, X, Y);
            
            /* Finish and avoid null displacement */
            if (fabs(vector.x) < min) vector.x = (vector.x < 0.0f) ? -min : min;
            if (fabs(vector.y) < min) vector.y = (vector.y < 0.0f) ? -min : min;
            
            data->brutT[premul_y_prevX] = ((int)((X-vector.x)*inv_ratio)+((int)(data->middleX*BUFFPOINTNB)));
            data->brutT[premul_y_prevX+1] = ((int)((Y-vector.y)*inv_ratio)+((int)(data->middleY*BUFFPOINTNB)));
            premul_y_prevX += 2;
            X += ratio;
        }
        Y += ratio;
    }
    data->interlace_start += INTERLACE_INCR;
    if (y >= data->prevY-1) data->interlace_start = -1;
}


/*
 * calculer px et py en fonction de x,y,middleX,middleY et theMode
 * px et py indique la nouvelle position (en sqrtperte ieme de pixel)
 * (valeur * 16)
 
 inline void calculatePXandPY (PluginInfo *goomInfo, ZoomFilterFXWrapperData *data, int x, int y, int *px, int *py)
 {
     if (data->theMode == WATER_MODE) {
         int yy;
         
         yy = y + goom_irand(goomInfo->gRandom, 4) - goom_irand(goomInfo->gRandom, 4) + data->wave / 10;
         if (yy < 0)
             yy = 0;
         if (yy >= (signed int)goomInfo->screen.height)
             yy = goomInfo->screen.height - 1;
         
         *px = (x << 4) + data->firedec[yy] + (data->wave / 10);
         *py = (y << 4) + 132 - ((data->vitesse < 131) ? data->vitesse : 130);
         
         data->wavesp += goom_irand(goomInfo->gRandom, 3) - goom_irand(goomInfo->gRandom, 3);
         if (data->wave < -10)
             data->wavesp += 2;
         if (data->wave > 10)
             data->wavesp -= 2;
         data->wave += (data->wavesp / 10) + goom_irand(goomInfo->gRandom, 3) - goom_irand(goomInfo->gRandom, 3);
         if (data->wavesp > 100)
             data->wavesp = (data->wavesp * 9) / 10;
     }
     else {
         int     dist = 0, vx9, vy9;
         int     vx, vy;
         int     ppx, ppy;
         int     fvitesse = data->vitesse << 4;
         
         if (data->noisify) {
             x += goom_irand(goomInfo->gRandom, data->noisify) - goom_irand(goomInfo->gRandom, data->noisify);
             y += goom_irand(goomInfo->gRandom, data->noisify) - goom_irand(goomInfo->gRandom, data->noisify);
         }
         vx = (x - data->middleX) << 9;
         vy = (y - data->middleY) << 9;
         
         if (data->hPlaneEffect)
             vx += data->hPlaneEffect * (y - data->middleY);
         
         if (data->vPlaneEffect)
             vy += data->vPlaneEffect * (x - data->middleX);
         
         if (data->waveEffect) {
             fvitesse *=
             1024 +
             ShiftRight (goomInfo->sintable
                         [(unsigned short) (dist * 0xffff + EFFECT_DISTORS)], 6);
             fvitesse /= 1024;
         }
         
         if (data->hypercosEffect) {
             vx += ShiftRight (goomInfo->sintable[(-vy + dist) & 0xffff], 1);
             vy += ShiftRight (goomInfo->sintable[(vx + dist) & 0xffff], 1);
         }
         
         vx9 = ShiftRight (vx, 9);
         vy9 = ShiftRight (vy, 9);
         dist = vx9 * vx9 + vy9 * vy9;
         
         switch (data->theMode) {
             case WAVE_MODE:
                 fvitesse *=
                 1024 +
                 ShiftRight (goomInfo->sintable
                             [(unsigned short) (dist * 0xffff * EFFECT_DISTORS)], 6);
                 fvitesse>>=10;
                 break;
             case CRYSTAL_BALL_MODE:
                 fvitesse += (dist >> (10-EFFECT_DISTORS_SL));
                 break;
             case AMULETTE_MODE:
                 fvitesse -= (dist >> (4 - EFFECT_DISTORS_SL));
                 break;
             case SCRUNCH_MODE:
                 fvitesse -= (dist >> (10 - EFFECT_DISTORS_SL));
                 break;
             case HYPERCOS1_MODE:
                 vx = vx + ShiftRight (goomInfo->sintable[(-vy + dist) & 0xffff], 1);
                 vy = vy + ShiftRight (goomInfo->sintable[(vx + dist) & 0xffff], 1);
                 break;
             case HYPERCOS2_MODE:
                 vx =
                 vx + ShiftRight (goomInfo->sintable[(-ShiftRight (vy, 1) + dist) & 0xffff], 0);
                 vy =
                     vy + ShiftRight (goomInfo->sintable[(ShiftRight (vx, 1) + dist) & 0xffff], 0);
                 fvitesse = 128 << 4;
                 break;
             case YONLY_MODE:
                 fvitesse *= 1024 + ShiftRight (goomInfo->sintable[vy & 0xffff], 6);
                 fvitesse >>= 10;
                 break;
             case SPEEDWAY_MODE:
                 fvitesse -= (ShiftRight(vy,10-EFFECT_DISTORS_SL));
                 break;
         }
         
         if (fvitesse < -3024)
             fvitesse = -3024;
         
         if (vx < 0)									// pb avec decalage sur nb negatif
             ppx = -(-(vx * fvitesse) >> 16);
         // 16 = 9 + 7 (7 = nb chiffre virgule de vitesse * (v = 128 => immobile)
         //    * * * * * 9 = nb chiffre virgule de vx) 
         else
             ppx = ((vx * fvitesse) >> 16);
         
         if (vy < 0)
             ppy = -(-(vy * fvitesse) >> 16);
         else
             ppy = ((vy * fvitesse) >> 16);
         
         *px = (data->middleX << 4) + ppx;
         *py = (data->middleY << 4) + ppy;
     }
 }
 */



static void c_zoom (Pixel *expix1, Pixel *expix2, unsigned int prevX, unsigned int prevY, signed int *brutS, signed int *brutD,
                    int buffratio, int precalCoef[16][16])
{
    int     myPos, myPos2;
    Color   couleur;
    
    unsigned int ax = (prevX - 1) << PERTEDEC, ay = (prevY - 1) << PERTEDEC;
    
    int     bufsize = prevX * prevY * 2;
    int     bufwidth = prevX;
    
    expix1[0].val=expix1[prevX-1].val=expix1[prevX*prevY-1].val=expix1[prevX*prevY-prevX].val=0;
    
    for (myPos = 0; myPos < bufsize; myPos += 2) {
        Color   col1, col2, col3, col4;
        int     c1, c2, c3, c4, px, py;
        int     pos;
        int     coeffs;
        
        int     brutSmypos = brutS[myPos];
        
        myPos2 = myPos + 1;
        
        px = brutSmypos + (((brutD[myPos] - brutSmypos) * buffratio) >> BUFFPOINTNB);
        brutSmypos = brutS[myPos2];
        py = brutSmypos + (((brutD[myPos2] - brutSmypos) * buffratio) >> BUFFPOINTNB);
        
        if ((py >= ay) || (px >= ax)) {
            pos = coeffs = 0;
        } else {
            pos = ((px >> PERTEDEC) + prevX * (py >> PERTEDEC));
            /* coef en modulo 15 */
            coeffs = precalCoef[px & PERTEMASK][py & PERTEMASK];
        }
        getPixelRGB_ (expix1, pos, &col1);
        getPixelRGB_ (expix1, pos + 1, &col2);
        getPixelRGB_ (expix1, pos + bufwidth, &col3);
        getPixelRGB_ (expix1, pos + bufwidth + 1, &col4);
        
        c1 = coeffs;
        c2 = (c1 >> 8) & 0xFF;
        c3 = (c1 >> 16) & 0xFF;
        c4 = (c1 >> 24) & 0xFF;
        c1 = c1 & 0xff;
        
        couleur.r = col1.r * c1 + col2.r * c2 + col3.r * c3 + col4.r * c4;
        if (couleur.r > 5)
            couleur.r -= 5;
        couleur.r >>= 8;
        
        couleur.v = col1.v * c1 + col2.v * c2 + col3.v * c3 + col4.v * c4;
        if (couleur.v > 5)
            couleur.v -= 5;
        couleur.v >>= 8;
        
        couleur.b = col1.b * c1 + col2.b * c2 + col3.b * c3 + col4.b * c4;
        if (couleur.b > 5)
            couleur.b -= 5;
        couleur.b >>= 8;
        
        setPixelRGB_ (expix2, myPos >> 1, couleur);
    }
}

/** generate the water fx horizontal direction buffer */
static void generateTheWaterFXHorizontalDirectionBuffer(PluginInfo *goomInfo, ZoomFilterFXWrapperData *data) {
    
    int loopv;
    int decc = goom_irand(goomInfo->gRandom, 8) - 4;
    int spdc = goom_irand(goomInfo->gRandom, 8) - 4;
    int accel = goom_irand(goomInfo->gRandom, 8) - 4;
    
    for (loopv = data->prevY; loopv != 0;) {
        
        loopv--;
        data->firedec[loopv] = decc;
        decc += spdc / 10;
        spdc += goom_irand(goomInfo->gRandom, 3) - goom_irand(goomInfo->gRandom, 3);
        
        if (decc > 4)
            spdc -= 1;
        if (decc < -4)
            spdc += 1;
        
        if (spdc > 30)
            spdc = spdc - goom_irand(goomInfo->gRandom, 3) + accel / 10;
        if (spdc < -30)
            spdc = spdc + goom_irand(goomInfo->gRandom, 3) + accel / 10;
        
        if (decc > 8 && spdc > 1)
            spdc -= goom_irand(goomInfo->gRandom, 3) - 2;
        
        if (decc < -8 && spdc < -1)
            spdc += goom_irand(goomInfo->gRandom, 3) + 2;
        
        if (decc > 8 || decc < -8)
            decc = decc * 8 / 9;
        
        accel += goom_irand(goomInfo->gRandom, 2) - goom_irand(goomInfo->gRandom, 2);
        if (accel > 20)
            accel -= 2;
        if (accel < -20)
            accel += 2;
    }
}



/**
* Main work for the dynamic displacement map.
 * 
 * Reads data from pix1, write to pix2.
 *
 * Useful datas for this FX are stored in ZoomFilterData.
 * 
 * If you think that this is a strange function name, let me say that a long time ago,
 *  there has been a slow version and a gray-level only one. Then came these function,
 *  fast and workin in RGB colorspace ! nice but it only was applying a zoom to the image.
 *  So that is why you have this name, for the nostalgy of the first days of goom
 *  when it was just a tiny program writen in Turbo Pascal on my i486...
 */
void zoomFilterFastRGB (PluginInfo *goomInfo, Pixel * pix1, Pixel * pix2, ZoomFilterData * zf, Uint resx, Uint resy, int switchIncr, float switchMult)
{
    Uint x, y;
    
    ZoomFilterFXWrapperData *data = (ZoomFilterFXWrapperData*)goomInfo->zoomFilter_fx.fx_data;
    
    if (!BVAL(data->enabled_bp)) return;
    
    /** changement de taille **/
    if ((data->prevX != resx) || (data->prevY != resy)) {
        data->prevX = resx;
        data->prevY = resy;
        
        if (data->brutS) free (data->freebrutS);
        data->brutS = 0;
        if (data->brutD) free (data->freebrutD);
        data->brutD = 0;
        if (data->brutT) free (data->freebrutT);
        data->brutT = 0;
        
        data->middleX = resx / 2;
        data->middleY = resy / 2;
        data->mustInitBuffers = 1;
        if (data->firedec) free (data->firedec);
        data->firedec = 0;
    }
    
    if (data->interlace_start != -2)
        zf = NULL;
    
    /** changement de config **/
    if (zf) {
        data->reverse = zf->reverse;
        data->general_speed = (float)(zf->vitesse-128)/128.0f;
        if (data->reverse) data->general_speed = -data->general_speed;
        data->middleX = zf->middleX;
        data->middleY = zf->middleY;
        data->theMode = zf->mode;
        data->hPlaneEffect = zf->hPlaneEffect;
        data->vPlaneEffect = zf->vPlaneEffect;
        data->waveEffect = zf->waveEffect;
        data->hypercosEffect = zf->hypercosEffect;
        data->noisify = zf->noisify;
        data->interlace_start = 0;
    }
    
    
    if (data->mustInitBuffers) {
        
        data->mustInitBuffers = 0;
        data->freebrutS = (signed int *) calloc (resx * resy * 2 + 128, sizeof(unsigned int));
        data->brutS = (gint32 *) ((1 + ((uintptr_t) (data->freebrutS)) / 128) * 128);
        
        data->freebrutD = (signed int *) calloc (resx * resy * 2 + 128, sizeof(unsigned int));
        data->brutD = (gint32 *) ((1 + ((uintptr_t) (data->freebrutD)) / 128) * 128);
        
        data->freebrutT = (signed int *) calloc (resx * resy * 2 + 128, sizeof(unsigned int));
        data->brutT = (gint32 *) ((1 + ((uintptr_t) (data->freebrutT)) / 128) * 128);
        
        data->buffratio = 0;
        
        data->firedec = (int *) malloc (data->prevY * sizeof (int));
        generateTheWaterFXHorizontalDirectionBuffer(goomInfo, data);
        
        data->interlace_start = 0;
        makeZoomBufferStripe(data,resy);
        
        /* Copy the data from temp to dest and source */
        memcpy(data->brutS,data->brutT,resx * resy * 2 * sizeof(int));
        memcpy(data->brutD,data->brutT,resx * resy * 2 * sizeof(int));
    }
    
    /* generation du buffer de trans */
    if (data->interlace_start == -1) {
        
        /* sauvegarde de l'etat actuel dans la nouvelle source
        * TODO: write that in MMX (has been done in previous version, but did not follow some new fonctionnalities) */
        y = data->prevX * data->prevY * 2;
        for (x = 0; x < y; x += 2) {
            int brutSmypos = data->brutS[x];
            int x2 = x + 1;
            
            data->brutS[x] = brutSmypos + (((data->brutD[x] - brutSmypos) * data->buffratio) >> BUFFPOINTNB);
            brutSmypos = data->brutS[x2];
            data->brutS[x2] = brutSmypos + (((data->brutD[x2] - brutSmypos) * data->buffratio) >> BUFFPOINTNB);
        }
        data->buffratio = 0;
    }
    
    if (data->interlace_start == -1) {
        signed int * tmp;
        tmp = data->brutD;
        data->brutD=data->brutT;
        data->brutT=tmp;
        tmp = data->freebrutD;
        data->freebrutD=data->freebrutT;
        data->freebrutT=tmp;
        data->interlace_start = -2;
    }
    
    if (data->interlace_start>=0)
    {
        /* creation de la nouvelle destination */
        makeZoomBufferStripe(data,resy/16);
    }
    
    if (switchIncr != 0) {
        data->buffratio += switchIncr;
        if (data->buffratio > BUFFPOINTMASK)
            data->buffratio = BUFFPOINTMASK;
    }
    
    if (switchMult != 1.0f) {
        data->buffratio = (int) ((float) BUFFPOINTMASK * (1.0f - switchMult) +
                                 (float) data->buffratio * switchMult);
    }
    
    data->zoom_width = data->prevX;
    
    goomInfo->methods.zoom_filter (data->prevX, data->prevY, pix1, pix2,
                                   data->brutS, data->brutD, data->buffratio, data->precalCoef);
}

static void generatePrecalCoef (int precalCoef[16][16])
{
    int coefh, coefv;
    
    for (coefh = 0; coefh < 16; coefh++) {
        for (coefv = 0; coefv < 16; coefv++) {
            
            int i;
            int diffcoeffh;
            int diffcoeffv;
            
            diffcoeffh = sqrtperte - coefh;
            diffcoeffv = sqrtperte - coefv;
            
            if (!(coefh || coefv)) {
                i = 255;
            }
            else {
                int i1, i2, i3, i4;
                
                i1 = diffcoeffh * diffcoeffv;
                i2 = coefh * diffcoeffv;
                i3 = diffcoeffh * coefv;
                i4 = coefh * coefv;
                
                // TODO: faire mieux...
                if (i1) i1--;
                if (i2) i2--;
                if (i3) i3--;
                if (i4) i4--;
                
                i = (i1) | (i2 << 8) | (i3 << 16) | (i4 << 24);
            }
            precalCoef[coefh][coefv] = i;
        }
    }
}

/* VisualFX Wrapper */

static void zoomFilterVisualFXWrapper_init (struct _VISUAL_FX *_this, PluginInfo *info)
{
    ZoomFilterFXWrapperData *data = (ZoomFilterFXWrapperData*)malloc(sizeof(ZoomFilterFXWrapperData));
    
    data->coeffs = 0;
    data->freecoeffs = 0;
    data->brutS = 0;
    data->freebrutS = 0;
    data->brutD = 0;
    data->freebrutD = 0;
    data->brutT = 0;
    data->freebrutT = 0;
    data->prevX = 0;
    data->prevY = 0;
    
    data->mustInitBuffers = 1;
    data->interlace_start = -2;
    
    data->general_speed = 0.0f;
    data->reverse = 0;
    data->theMode = AMULETTE_MODE;
    data->waveEffect = 0;
    data->hypercosEffect = 0;
    data->vPlaneEffect = 0;
    data->hPlaneEffect = 0;
    data->noisify = 2;
    
    /** modif by jeko : fixedpoint : buffration = (16:16) (donc 0<=buffration<=2^16) */
    data->buffratio = 0;
    data->firedec = 0;
    
    data->wave = data->wavesp = 0;
    
    data->enabled_bp = secure_b_param("Enabled", 1);
    
    data->params = plugin_parameters ("Zoom Filter", 1);
    data->params.params[0] = &data->enabled_bp;
    
    _this->params = &data->params;
    _this->fx_data = (void*)data;
    
    /** modif d'optim by Jeko : precalcul des 4 coefs resultant des 2 pos */
    generatePrecalCoef(data->precalCoef);
}

static void zoomFilterVisualFXWrapper_free (struct _VISUAL_FX *_this)
{
    ZoomFilterFXWrapperData *data = (ZoomFilterFXWrapperData*)_this->fx_data;
    free (data->freebrutS);
    free (data->freebrutD);
    free (data->freebrutT);
    free (data->firedec);
    free (data->params.params);
    free(_this->fx_data);
}

static void zoomFilterVisualFXWrapper_apply (struct _VISUAL_FX *_this, Pixel *src, Pixel *dest, PluginInfo *info)
{
}

VisualFX zoomFilterVisualFXWrapper_create(void)
{
    VisualFX fx;
    fx.init = zoomFilterVisualFXWrapper_init;
    fx.free = zoomFilterVisualFXWrapper_free;
    fx.apply = zoomFilterVisualFXWrapper_apply;
    return fx;
}


/* TODO : MOVE THIS AWAY */

void pointFilter (PluginInfo *goomInfo, Pixel * pix1, Color c, float t1, float t2, float t3, float t4, Uint cycle)
{
    Uint x = (Uint) ((int) (goomInfo->screen.width / 2)
                     + (int) (t1 * cos ((float) cycle / t3)));
    Uint y = (Uint) ((int) (goomInfo->screen.height/2)
                     + (int) (t2 * sin ((float) cycle / t4)));
    
    if ((x > 1) && (y > 1) && (x < goomInfo->screen.width - 2) && (y < goomInfo->screen.height - 2)) {
        setPixelRGB (goomInfo, pix1, x + 1, y, c);
        setPixelRGB (goomInfo, pix1, x, y + 1, c);
        setPixelRGB (goomInfo, pix1, x + 1, y + 1, WHITE);
        setPixelRGB (goomInfo, pix1, x + 2, y + 1, c);
        setPixelRGB (goomInfo, pix1, x + 1, y + 2, c);
    }
}
