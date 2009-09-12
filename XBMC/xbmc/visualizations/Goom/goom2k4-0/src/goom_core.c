/**
* file: goom_core.c
 * author: Jean-Christophe Hoelt (which is not so proud of it)
 *
 * Contains the core of goom's work.
 *
 * (c)2000-2003, by iOS-software.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "goom.h"
#include "goom_tools.h"
#include "goom_filters.h"
#include "lines.h"
#include "ifs.h"
#include "tentacle3d.h"
#include "gfontlib.h"

#include "sound_tester.h"
#include "goom_plugin_info.h"
#include "goom_fx.h"
#include "goomsl.h"

/* #define VERBOSE */

#define STOP_SPEED 128
/* TODO: put that as variable in PluginInfo */
#define TIME_BTW_CHG 300

static void choose_a_goom_line (PluginInfo *goomInfo, float *param1, float *param2, int *couleur,
                                int *mode, float *amplitude, int far);

static void update_message (PluginInfo *goomInfo, char *message);

static void init_buffers(PluginInfo *goomInfo, int buffsize)
{
    goomInfo->pixel = (guint32 *) malloc (buffsize * sizeof (guint32) + 128);
    bzero (goomInfo->pixel, buffsize * sizeof (guint32) + 128);
    goomInfo->back = (guint32 *) malloc (buffsize * sizeof (guint32) + 128);
    bzero (goomInfo->back, buffsize * sizeof (guint32) + 128);
    goomInfo->conv = (Pixel *) malloc (buffsize * sizeof (guint32) + 128);
    bzero (goomInfo->conv, buffsize * sizeof (guint32) + 128);

    goomInfo->outputBuf = goomInfo->conv;
    
    goomInfo->p1 = (Pixel *) ((1 + ((uintptr_t) (goomInfo->pixel)) / 128) * 128);
    goomInfo->p2 = (Pixel *) ((1 + ((uintptr_t) (goomInfo->back)) / 128) * 128);
}

/**************************
*         INIT           *
**************************/
PluginInfo *goom_init (guint32 resx, guint32 resy)
{
    PluginInfo *goomInfo = (PluginInfo*)malloc(sizeof(PluginInfo));
    
#ifdef VERBOSE
    printf ("GOOM: init (%d, %d);\n", resx, resy);
#endif
    
    plugin_info_init(goomInfo,4);
    
    goomInfo->star_fx = flying_star_create();
    goomInfo->star_fx.init(&goomInfo->star_fx, goomInfo);
    
    goomInfo->zoomFilter_fx = zoomFilterVisualFXWrapper_create ();
    goomInfo->zoomFilter_fx.init(&goomInfo->zoomFilter_fx, goomInfo);
    
    goomInfo->tentacles_fx = tentacle_fx_create();
    goomInfo->tentacles_fx.init(&goomInfo->tentacles_fx, goomInfo);
    
    goomInfo->convolve_fx = convolve_create();
    goomInfo->convolve_fx.init(&goomInfo->convolve_fx, goomInfo);
    
    plugin_info_add_visual (goomInfo, 0, &goomInfo->zoomFilter_fx);
    plugin_info_add_visual (goomInfo, 1, &goomInfo->tentacles_fx);
    plugin_info_add_visual (goomInfo, 2, &goomInfo->star_fx);
    plugin_info_add_visual (goomInfo, 3, &goomInfo->convolve_fx);
    
    goomInfo->screen.width = resx;
    goomInfo->screen.height = resy;
    goomInfo->screen.size = resx * resy;
    
    init_buffers(goomInfo, goomInfo->screen.size);
    goomInfo->gRandom = goom_random_init((uintptr_t)goomInfo->pixel);
    
    goomInfo->cycle = 0;
    
    goomInfo->ifs_fx = ifs_visualfx_create();
    goomInfo->ifs_fx.init(&goomInfo->ifs_fx, goomInfo);
    
    goomInfo->gmline1 = goom_lines_init (goomInfo, resx, goomInfo->screen.height,
                                         GML_HLINE, goomInfo->screen.height, GML_BLACK,
                                         GML_CIRCLE, 0.4f * (float) goomInfo->screen.height, GML_VERT);
    goomInfo->gmline2 = goom_lines_init (goomInfo, resx, goomInfo->screen.height,
                                         GML_HLINE, 0, GML_BLACK,
                                         GML_CIRCLE, 0.2f * (float) goomInfo->screen.height, GML_RED);
    
    gfont_load ();
 
    /* goom_set_main_script(goomInfo, goomInfo->main_script_str); */
    
    return goomInfo;
}



void goom_set_resolution (PluginInfo *goomInfo, guint32 resx, guint32 resy)
{
    free (goomInfo->pixel);
    free (goomInfo->back);
    free (goomInfo->conv);
    
    goomInfo->screen.width = resx;
    goomInfo->screen.height = resy;
    goomInfo->screen.size = resx * resy;
    
    init_buffers(goomInfo, goomInfo->screen.size);
    
    /* init_ifs (goomInfo, resx, goomInfo->screen.height); */
    goomInfo->ifs_fx.free(&goomInfo->ifs_fx);
    goomInfo->ifs_fx.init(&goomInfo->ifs_fx, goomInfo);
    
    goom_lines_set_res (goomInfo->gmline1, resx, goomInfo->screen.height);
    goom_lines_set_res (goomInfo->gmline2, resx, goomInfo->screen.height);
}

int goom_set_screenbuffer(PluginInfo *goomInfo, void *buffer)
{
  goomInfo->outputBuf = (Pixel*)buffer;
  return 1;
}

/********************************************
*                  UPDATE                  *
********************************************

* WARNING: this is a 600 lines function ! (21-11-2003)
*/
guint32 *goom_update (PluginInfo *goomInfo, gint16 data[2][512],
                      int forceMode, float fps, char *songTitle, char *message)
{
    Pixel *return_val;
    guint32 pointWidth;
    guint32 pointHeight;
    int     i;
    float   largfactor;	/* elargissement de l'intervalle d'évolution des points */
    Pixel *tmp;
    
    ZoomFilterData *pzfd;
    
    /* test if the config has changed, update it if so */
    pointWidth = (goomInfo->screen.width * 2) / 5;
    pointHeight = ((goomInfo->screen.height) * 2) / 5;
    
    /* ! etude du signal ... */
    evaluate_sound (data, &(goomInfo->sound));
    
    /* goom_execute_main_script(goomInfo); */
    
    /* ! calcul du deplacement des petits points ... */
    largfactor = goomInfo->sound.speedvar / 150.0f + goomInfo->sound.volume / 1.5f;
    
    if (largfactor > 1.5f)
        largfactor = 1.5f;
    
    goomInfo->update.decay_ifs--;
    if (goomInfo->update.decay_ifs > 0)
        goomInfo->update.ifs_incr += 2;
    if (goomInfo->update.decay_ifs == 0)
        goomInfo->update.ifs_incr = 0;
    
    if (goomInfo->update.recay_ifs) {
        goomInfo->update.ifs_incr -= 2;
        goomInfo->update.recay_ifs--;
        if ((goomInfo->update.recay_ifs == 0)&&(goomInfo->update.ifs_incr<=0))
            goomInfo->update.ifs_incr = 1;
    }
    
    if (goomInfo->update.ifs_incr > 0)
        goomInfo->ifs_fx.apply(&goomInfo->ifs_fx, goomInfo->p2, goomInfo->p1, goomInfo);
    
    if (goomInfo->curGState->drawPoints) {
        for (i = 1; i * 15 <= goomInfo->sound.speedvar*80.0f + 15; i++) {
            goomInfo->update.loopvar += goomInfo->sound.speedvar*50 + 1;
            
            pointFilter (goomInfo, goomInfo->p1,
                         YELLOW,
                         ((pointWidth - 6.0f) * largfactor + 5.0f),
                         ((pointHeight - 6.0f) * largfactor + 5.0f),
                         i * 152.0f, 128.0f, goomInfo->update.loopvar + i * 2032);
            pointFilter (goomInfo, goomInfo->p1, ORANGE,
                         ((pointWidth / 2) * largfactor) / i + 10.0f * i,
                         ((pointHeight / 2) * largfactor) / i + 10.0f * i,
                         96.0f, i * 80.0f, goomInfo->update.loopvar / i);
            pointFilter (goomInfo, goomInfo->p1, VIOLET,
                         ((pointHeight / 3 + 5.0f) * largfactor) / i + 10.0f * i,
                         ((pointHeight / 3 + 5.0f) * largfactor) / i + 10.0f * i,
                         i + 122.0f, 134.0f, goomInfo->update.loopvar / i);
            pointFilter (goomInfo, goomInfo->p1, BLACK,
                         ((pointHeight / 3) * largfactor + 20.0f),
                         ((pointHeight / 3) * largfactor + 20.0f),
                         58.0f, i * 66.0f, goomInfo->update.loopvar / i);
            pointFilter (goomInfo, goomInfo->p1, WHITE,
                         (pointHeight * largfactor + 10.0f * i) / i,
                         (pointHeight * largfactor + 10.0f * i) / i,
                         66.0f, 74.0f, goomInfo->update.loopvar + i * 500);
        }
    }
    
    /* par défaut pas de changement de zoom */
    pzfd = NULL;
    
    /* 
        * Test forceMode
     */
#ifdef VERBOSE
    if (forceMode != 0) {
        printf ("forcemode = %d\n", forceMode);
    }
#endif
    
    
    /* diminuer de 1 le temps de lockage */
    /* note pour ceux qui n'ont pas suivis : le lockvar permet d'empecher un */
    /* changement d'etat du plugin juste apres un autre changement d'etat. oki */
    if (--goomInfo->update.lockvar < 0)
        goomInfo->update.lockvar = 0;
    
    /* on verifie qu'il ne se pas un truc interressant avec le son. */
    if ((goomInfo->sound.timeSinceLastGoom == 0)
        || (forceMode > 0)
        || (goomInfo->update.cyclesSinceLastChange > TIME_BTW_CHG)) {
        
        /* changement eventuel de mode */
        if (goom_irand(goomInfo->gRandom,16) == 0)
            switch (goom_irand(goomInfo->gRandom,34)) {
                case 0:
                case 10:
                    goomInfo->update.zoomFilterData.hypercosEffect = goom_irand(goomInfo->gRandom,2);
                case 13:
                case 20:
                case 21:
                    goomInfo->update.zoomFilterData.mode = WAVE_MODE;
                    goomInfo->update.zoomFilterData.reverse = 0;
                    goomInfo->update.zoomFilterData.waveEffect = (goom_irand(goomInfo->gRandom,3) == 0);
                    if (goom_irand(goomInfo->gRandom,2))
                        goomInfo->update.zoomFilterData.vitesse = (goomInfo->update.zoomFilterData.vitesse + 127) >> 1;
                        break;
                case 1:
                case 11:
                    goomInfo->update.zoomFilterData.mode = CRYSTAL_BALL_MODE;
                    goomInfo->update.zoomFilterData.waveEffect = 0;
                    goomInfo->update.zoomFilterData.hypercosEffect = 0;
                    break;
                case 2:
                case 12:
                    goomInfo->update.zoomFilterData.mode = AMULETTE_MODE;
                    goomInfo->update.zoomFilterData.waveEffect = 0;
                    goomInfo->update.zoomFilterData.hypercosEffect = 0;
                    break;
                case 3:
                    goomInfo->update.zoomFilterData.mode = WATER_MODE;
                    goomInfo->update.zoomFilterData.waveEffect = 0;
                    goomInfo->update.zoomFilterData.hypercosEffect = 0;
                    break;
                case 4:
                case 14:
                    goomInfo->update.zoomFilterData.mode = SCRUNCH_MODE;
                    goomInfo->update.zoomFilterData.waveEffect = 0;
                    goomInfo->update.zoomFilterData.hypercosEffect = 0;
                    break;
                case 5:
                case 15:
                case 22:
                    goomInfo->update.zoomFilterData.mode = HYPERCOS1_MODE;
                    goomInfo->update.zoomFilterData.waveEffect = 0;
                    goomInfo->update.zoomFilterData.hypercosEffect = (goom_irand(goomInfo->gRandom,3) == 0);
                    break;
                case 6:
                case 16:
                    goomInfo->update.zoomFilterData.mode = HYPERCOS2_MODE;
                    goomInfo->update.zoomFilterData.waveEffect = 0;
                    goomInfo->update.zoomFilterData.hypercosEffect = 0;
                    break;
                case 7:
                case 17:
                    goomInfo->update.zoomFilterData.mode = CRYSTAL_BALL_MODE;
                    goomInfo->update.zoomFilterData.waveEffect = (goom_irand(goomInfo->gRandom,4) == 0);
                    goomInfo->update.zoomFilterData.hypercosEffect = goom_irand(goomInfo->gRandom,2);
                    break;
                case 8:
                case 18:
                case 19:
                    goomInfo->update.zoomFilterData.mode = SCRUNCH_MODE;
                    goomInfo->update.zoomFilterData.waveEffect = 1;
                    goomInfo->update.zoomFilterData.hypercosEffect = 1;
                    break;
                case 29:
                case 30:
                    goomInfo->update.zoomFilterData.mode = YONLY_MODE;
                    break;
                case 31:
                case 32:
                case 33:
                    goomInfo->update.zoomFilterData.mode = SPEEDWAY_MODE;
                    break;
                default:
                    goomInfo->update.zoomFilterData.mode = NORMAL_MODE;
                    goomInfo->update.zoomFilterData.waveEffect = 0;
                    goomInfo->update.zoomFilterData.hypercosEffect = 0;
            }
    }
        
        /* tout ceci ne sera fait qu'en cas de non-blocage */
        if (goomInfo->update.lockvar == 0) {
            /* reperage de goom (acceleration forte de l'acceleration du volume) */
            /* -> coup de boost de la vitesse si besoin.. */
            if (goomInfo->sound.timeSinceLastGoom == 0) {
                
                int i;
                goomInfo->update.goomvar++;
                
                /* SELECTION OF THE GOOM STATE */
                if ((!goomInfo->update.stateSelectionBlocker)&&(goom_irand(goomInfo->gRandom,3))) {
                    goomInfo->update.stateSelectionRnd = goom_irand(goomInfo->gRandom,goomInfo->statesRangeMax);
                    goomInfo->update.stateSelectionBlocker = 3;
                }
                else if (goomInfo->update.stateSelectionBlocker) goomInfo->update.stateSelectionBlocker--;
                
                for (i=0;i<goomInfo->statesNumber;i++)
                    if ((goomInfo->update.stateSelectionRnd >= goomInfo->states[i].rangemin)
                        && (goomInfo->update.stateSelectionRnd <= goomInfo->states[i].rangemax))
                        goomInfo->curGState = &(goomInfo->states[i]);
                
                if ((goomInfo->curGState->drawIFS) && (goomInfo->update.ifs_incr<=0)) {
                    goomInfo->update.recay_ifs = 5;
                    goomInfo->update.ifs_incr = 11;
                }
                
                if ((!goomInfo->curGState->drawIFS) && (goomInfo->update.ifs_incr>0) && (goomInfo->update.decay_ifs<=0))
                    goomInfo->update.decay_ifs = 100;
                
                if (!goomInfo->curGState->drawScope)
                    goomInfo->update.stop_lines = 0xf000 & 5;
                
                if (!goomInfo->curGState->drawScope) {
                    goomInfo->update.stop_lines = 0;			
                    goomInfo->update.lineMode = goomInfo->update.drawLinesDuration;
                }
                
                /* if (goomInfo->update.goomvar % 1 == 0) */
                {
                    guint32 vtmp;
                    guint32 newvit;
                    
                    goomInfo->update.lockvar = 50;
                    newvit = STOP_SPEED + 1 - ((float)3.5f * log10(goomInfo->sound.speedvar * 60 + 1));
                    /* retablir le zoom avant.. */
                    if ((goomInfo->update.zoomFilterData.reverse) && (!(goomInfo->cycle % 13)) && (rand () % 5 == 0)) {
                        goomInfo->update.zoomFilterData.reverse = 0;
                        goomInfo->update.zoomFilterData.vitesse = STOP_SPEED - 2;
                        goomInfo->update.lockvar = 75;
                    }
                    if (goom_irand(goomInfo->gRandom,10) == 0) {
                        goomInfo->update.zoomFilterData.reverse = 1;
                        goomInfo->update.lockvar = 100;
                    }
                    
                    if (goom_irand(goomInfo->gRandom,10) == 0)
                        goomInfo->update.zoomFilterData.vitesse = STOP_SPEED - 1;
                    if (goom_irand(goomInfo->gRandom,12) == 0)
                        goomInfo->update.zoomFilterData.vitesse = STOP_SPEED + 1;
                    
                    /* changement de milieu.. */
                    switch (goom_irand(goomInfo->gRandom,25)) {
                        case 0:
                        case 3:
                        case 6:
                            goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height - 1;
                            goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width / 2;
                            break;
                        case 1:
                        case 4:
                            goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width - 1;
                            break;
                        case 2:
                        case 5:
                            goomInfo->update.zoomFilterData.middleX = 1;
                            break;
                        default:
                            goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height / 2;
                            goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width / 2;
                    }
                    
                    if ((goomInfo->update.zoomFilterData.mode == WATER_MODE)
                        || (goomInfo->update.zoomFilterData.mode == YONLY_MODE)
                        || (goomInfo->update.zoomFilterData.mode == AMULETTE_MODE)) {
                        goomInfo->update.zoomFilterData.middleX = goomInfo->screen.width / 2;
                        goomInfo->update.zoomFilterData.middleY = goomInfo->screen.height / 2;
                    }
                    
                    switch (vtmp = (goom_irand(goomInfo->gRandom,15))) {
                        case 0:
                            goomInfo->update.zoomFilterData.vPlaneEffect = goom_irand(goomInfo->gRandom,3)
                            - goom_irand(goomInfo->gRandom,3);
                            goomInfo->update.zoomFilterData.hPlaneEffect = goom_irand(goomInfo->gRandom,3)
                                - goom_irand(goomInfo->gRandom,3);
                            break;
                        case 3:
                            goomInfo->update.zoomFilterData.vPlaneEffect = 0;
                            goomInfo->update.zoomFilterData.hPlaneEffect = goom_irand(goomInfo->gRandom,8)
                                - goom_irand(goomInfo->gRandom,8);
                            break;
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                            goomInfo->update.zoomFilterData.vPlaneEffect = goom_irand(goomInfo->gRandom,5)
                            - goom_irand(goomInfo->gRandom,5);
                            goomInfo->update.zoomFilterData.hPlaneEffect = -goomInfo->update.zoomFilterData.vPlaneEffect;
                            break;
                        case 8:
                            goomInfo->update.zoomFilterData.hPlaneEffect = 5 + goom_irand(goomInfo->gRandom,8);
                            goomInfo->update.zoomFilterData.vPlaneEffect = -goomInfo->update.zoomFilterData.hPlaneEffect;
                            break;
                        case 9:
                            goomInfo->update.zoomFilterData.vPlaneEffect = 5 + goom_irand(goomInfo->gRandom,8);
                            goomInfo->update.zoomFilterData.hPlaneEffect = -goomInfo->update.zoomFilterData.hPlaneEffect;
                            break;
                        case 13:
                            goomInfo->update.zoomFilterData.hPlaneEffect = 0;
                            goomInfo->update.zoomFilterData.vPlaneEffect = goom_irand(goomInfo->gRandom,10)
                                - goom_irand(goomInfo->gRandom,10);
                            break;
                        case 14:
                            goomInfo->update.zoomFilterData.hPlaneEffect = goom_irand(goomInfo->gRandom,10)
                            - goom_irand(goomInfo->gRandom,10);
                            goomInfo->update.zoomFilterData.vPlaneEffect = goom_irand(goomInfo->gRandom,10)
                                - goom_irand(goomInfo->gRandom,10);
                            break;
                        default:
                            if (vtmp < 10) {
                                goomInfo->update.zoomFilterData.vPlaneEffect = 0;
                                goomInfo->update.zoomFilterData.hPlaneEffect = 0;
                            }
                    }
                    
                    if (goom_irand(goomInfo->gRandom,5) != 0)
                        goomInfo->update.zoomFilterData.noisify = 0;
                    else {
                        goomInfo->update.zoomFilterData.noisify = goom_irand(goomInfo->gRandom,2) + 1;
                        goomInfo->update.lockvar *= 2;
                    }
                    
                    if (goomInfo->update.zoomFilterData.mode == AMULETTE_MODE) {
                        goomInfo->update.zoomFilterData.vPlaneEffect = 0;
                        goomInfo->update.zoomFilterData.hPlaneEffect = 0;
                        goomInfo->update.zoomFilterData.noisify = 0;
                    }
                    
                    if ((goomInfo->update.zoomFilterData.middleX == 1) || (goomInfo->update.zoomFilterData.middleX == (signed int)goomInfo->screen.width - 1)) {
                        goomInfo->update.zoomFilterData.vPlaneEffect = 0;
                        if (goom_irand(goomInfo->gRandom,2))
                            goomInfo->update.zoomFilterData.hPlaneEffect = 0;
                    }
                    
                    if ((signed int)newvit < goomInfo->update.zoomFilterData.vitesse)	/* on accelere */
                    {
                        pzfd = &goomInfo->update.zoomFilterData;
                        if (((newvit < STOP_SPEED - 7) &&
                             (goomInfo->update.zoomFilterData.vitesse < STOP_SPEED - 6) &&
                             (goomInfo->cycle % 3 == 0)) || (goom_irand(goomInfo->gRandom,40) == 0)) {
                            goomInfo->update.zoomFilterData.vitesse = STOP_SPEED - goom_irand(goomInfo->gRandom,2)
                            + goom_irand(goomInfo->gRandom,2);
                            goomInfo->update.zoomFilterData.reverse = !goomInfo->update.zoomFilterData.reverse;
                        }
                        else {
                            goomInfo->update.zoomFilterData.vitesse = (newvit + goomInfo->update.zoomFilterData.vitesse * 7) / 8;
                        }
                        goomInfo->update.lockvar += 50;
                    }
                }
                
                if (goomInfo->update.lockvar > 150) {
                    goomInfo->update.switchIncr = goomInfo->update.switchIncrAmount;
                    goomInfo->update.switchMult = 1.0f;
                }
            }
            /* mode mega-lent */
            if (goom_irand(goomInfo->gRandom,700) == 0) {
                /* 
                * printf ("coup du sort...\n") ;
                 */
                pzfd = &goomInfo->update.zoomFilterData;
                goomInfo->update.zoomFilterData.vitesse = STOP_SPEED - 1;
                goomInfo->update.zoomFilterData.pertedec = 8;
                goomInfo->update.zoomFilterData.sqrtperte = 16;
                goomInfo->update.goomvar = 1;
                goomInfo->update.lockvar += 50;
                goomInfo->update.switchIncr = goomInfo->update.switchIncrAmount;
                goomInfo->update.switchMult = 1.0f;
            }
        }
        
        /*
         * gros frein si la musique est calme
         */
        if ((goomInfo->sound.speedvar < 0.01f)
            && (goomInfo->update.zoomFilterData.vitesse < STOP_SPEED - 4)
            && (goomInfo->cycle % 16 == 0)) {
            pzfd = &goomInfo->update.zoomFilterData;
            goomInfo->update.zoomFilterData.vitesse += 3;
            goomInfo->update.zoomFilterData.pertedec = 8;
            goomInfo->update.zoomFilterData.sqrtperte = 16;
            goomInfo->update.goomvar = 0;
        }
        
        /*
         * baisser regulierement la vitesse...
         */
        if ((goomInfo->cycle % 73 == 0) && (goomInfo->update.zoomFilterData.vitesse < STOP_SPEED - 5)) {
            pzfd = &goomInfo->update.zoomFilterData;
            goomInfo->update.zoomFilterData.vitesse++;
        }
        
        /*
         * arreter de decrémenter au bout d'un certain temps
         */
        if ((goomInfo->cycle % 101 == 0) && (goomInfo->update.zoomFilterData.pertedec == 7)) {
            pzfd = &goomInfo->update.zoomFilterData;
            goomInfo->update.zoomFilterData.pertedec = 8;
            goomInfo->update.zoomFilterData.sqrtperte = 16;
        }
        
        /*
         * Permet de forcer un effet.
         */
        if ((forceMode > 0) && (forceMode <= NB_FX)) {
            pzfd = &goomInfo->update.zoomFilterData;
            pzfd->mode = forceMode - 1;
        }
        
        if (forceMode == -1) {
            pzfd = NULL;
        }
        
        /*
         * Changement d'effet de zoom !
         */
        if (pzfd != NULL) {
            int        dif;
            
            goomInfo->update.cyclesSinceLastChange = 0;
            
            goomInfo->update.switchIncr = goomInfo->update.switchIncrAmount;
            
            dif = goomInfo->update.zoomFilterData.vitesse - goomInfo->update.previousZoomSpeed;
            if (dif < 0)
                dif = -dif;
            
            if (dif > 2) {
                goomInfo->update.switchIncr *= (dif + 2) / 2;
            }
            goomInfo->update.previousZoomSpeed = goomInfo->update.zoomFilterData.vitesse;
            goomInfo->update.switchMult = 1.0f;
            
            if (((goomInfo->sound.timeSinceLastGoom == 0)
                 && (goomInfo->sound.totalgoom < 2)) || (forceMode > 0)) {
                goomInfo->update.switchIncr = 0;
                goomInfo->update.switchMult = goomInfo->update.switchMultAmount;
            }
        }
        else {
            if (goomInfo->update.cyclesSinceLastChange > TIME_BTW_CHG) {
                pzfd = &goomInfo->update.zoomFilterData;
                goomInfo->update.cyclesSinceLastChange = 0;
            }
            else
                goomInfo->update.cyclesSinceLastChange++;
        }
        
#ifdef VERBOSE
        if (pzfd) {
            printf ("GOOM: pzfd->mode = %d\n", pzfd->mode);
        }
#endif
        
        /* Zoom here ! */
        zoomFilterFastRGB (goomInfo, goomInfo->p1, goomInfo->p2, pzfd, goomInfo->screen.width, goomInfo->screen.height,
                           goomInfo->update.switchIncr, goomInfo->update.switchMult);
        
        /*
         * Affichage tentacule
         */
        
        goomInfo->tentacles_fx.apply(&goomInfo->tentacles_fx, goomInfo->p1, goomInfo->p2, goomInfo);
        goomInfo->star_fx.apply (&goomInfo->star_fx,goomInfo->p2,goomInfo->p1,goomInfo);
        
        /*
         * Affichage de texte
         */
        {
            /*char title[1024];*/
            char text[64];
            
            /*
             * Le message
             */
            update_message (goomInfo, message);
            
            if (fps > 0) {
                sprintf (text, "%2.0f fps", fps);
                goom_draw_text (goomInfo->p1,goomInfo->screen.width,goomInfo->screen.height,
                                10, 24, text, 1, 0);
            }
            
            /*
             * Le titre
             */
            if (songTitle != NULL) {
                strncpy (goomInfo->update.titleText, songTitle, 1023);
                goomInfo->update.titleText[1023]=0;
                goomInfo->update.timeOfTitleDisplay = 200;
            }
            
            if (goomInfo->update.timeOfTitleDisplay) {
                goom_draw_text (goomInfo->p1,goomInfo->screen.width,goomInfo->screen.height,
                                goomInfo->screen.width / 2, goomInfo->screen.height / 2 + 7, goomInfo->update.titleText,
                                ((float) (190 - goomInfo->update.timeOfTitleDisplay) / 10.0f), 1);
                goomInfo->update.timeOfTitleDisplay--;
                if (goomInfo->update.timeOfTitleDisplay < 4)
                    goom_draw_text (goomInfo->p2,goomInfo->screen.width,goomInfo->screen.height,
                                    goomInfo->screen.width / 2, goomInfo->screen.height / 2 + 7, goomInfo->update.titleText,
                                    ((float) (190 - goomInfo->update.timeOfTitleDisplay) / 10.0f), 1);
            }
        }
        
        /*
         * Gestion du Scope
         */
        
        /*
         * arret demande
         */
        if ((goomInfo->update.stop_lines & 0xf000)||(!goomInfo->curGState->drawScope)) {
            float   param1, param2, amplitude;
            int     couleur;
            int     mode;
            
            choose_a_goom_line (goomInfo, &param1, &param2, &couleur, &mode, &amplitude,1);
            couleur = GML_BLACK;
            
            goom_lines_switch_to (goomInfo->gmline1, mode, param1, amplitude, couleur);
            goom_lines_switch_to (goomInfo->gmline2, mode, param2, amplitude, couleur);
            goomInfo->update.stop_lines &= 0x0fff;
        }
        
        /*
         * arret aleatore.. changement de mode de ligne..
         */
        if (goomInfo->update.lineMode != goomInfo->update.drawLinesDuration) {
            goomInfo->update.lineMode--;
            if (goomInfo->update.lineMode == -1)
                goomInfo->update.lineMode = 0;
        }
        else
            if ((goomInfo->cycle%80==0)&&(goom_irand(goomInfo->gRandom,5)==0)&&goomInfo->update.lineMode)
                goomInfo->update.lineMode--;
        
        if ((goomInfo->cycle % 120 == 0)
            && (goom_irand(goomInfo->gRandom,4) == 0)
            && (goomInfo->curGState->drawScope)) {
            if (goomInfo->update.lineMode == 0)
                goomInfo->update.lineMode = goomInfo->update.drawLinesDuration;
            else if (goomInfo->update.lineMode == goomInfo->update.drawLinesDuration) {
                float   param1, param2, amplitude;
                int     couleur1,couleur2;
                int     mode;
                
                goomInfo->update.lineMode--;
                choose_a_goom_line (goomInfo, &param1, &param2, &couleur1,
                                    &mode, &amplitude,goomInfo->update.stop_lines);
                
                couleur2 = 5-couleur1;
                if (goomInfo->update.stop_lines) {
                    goomInfo->update.stop_lines--;
                    if (goom_irand(goomInfo->gRandom,2))
                        couleur2=couleur1 = GML_BLACK;
                }
                
                goom_lines_switch_to (goomInfo->gmline1, mode, param1, amplitude, couleur1);
                goom_lines_switch_to (goomInfo->gmline2, mode, param2, amplitude, couleur2);
            }
        }
        
        /*
         * si on est dans un goom : afficher les lignes...
         */
        if ((goomInfo->update.lineMode != 0) || (goomInfo->sound.timeSinceLastGoom < 5)) {
            goomInfo->gmline2->power = goomInfo->gmline1->power;
            
            goom_lines_draw (goomInfo, goomInfo->gmline1, data[0], goomInfo->p2);
            goom_lines_draw (goomInfo, goomInfo->gmline2, data[1], goomInfo->p2);
            
            if (((goomInfo->cycle % 121) == 9) && (goom_irand(goomInfo->gRandom,3) == 1)
                && ((goomInfo->update.lineMode == 0) || (goomInfo->update.lineMode == goomInfo->update.drawLinesDuration))) {
                float   param1, param2, amplitude;
                int     couleur1,couleur2;
                int     mode;
                
                choose_a_goom_line (goomInfo, &param1, &param2, &couleur1,
                                    &mode, &amplitude, goomInfo->update.stop_lines);
                couleur2 = 5-couleur1;
                
                if (goomInfo->update.stop_lines) {
                    goomInfo->update.stop_lines--;
                    if (goom_irand(goomInfo->gRandom,2))
                        couleur2=couleur1 = GML_BLACK;
                }
                goom_lines_switch_to (goomInfo->gmline1, mode, param1, amplitude, couleur1);
                goom_lines_switch_to (goomInfo->gmline2, mode, param2, amplitude, couleur2);
            }
        }
        
        return_val = goomInfo->p1;
        tmp = goomInfo->p1;
        goomInfo->p1 = goomInfo->p2;
        goomInfo->p2 = tmp;
        
        /* affichage et swappage des buffers.. */
        goomInfo->cycle++;
        
        goomInfo->convolve_fx.apply(&goomInfo->convolve_fx,return_val,goomInfo->outputBuf,goomInfo);
        
        return (guint32*)goomInfo->outputBuf;
}

/****************************************
*                CLOSE                 *
****************************************/
void goom_close (PluginInfo *goomInfo)
{
    if (goomInfo->pixel != NULL)
        free (goomInfo->pixel);
    if (goomInfo->back != NULL)
        free (goomInfo->back);
    if (goomInfo->conv != NULL)
        free (goomInfo->conv);
    
    goomInfo->pixel = goomInfo->back = NULL;
    goomInfo->conv = NULL;
    goom_random_free(goomInfo->gRandom);
    goom_lines_free (&goomInfo->gmline1);
    goom_lines_free (&goomInfo->gmline2);
    
    /* release_ifs (); */
    goomInfo->ifs_fx.free(&goomInfo->ifs_fx);
    goomInfo->convolve_fx.free(&goomInfo->convolve_fx);
    goomInfo->star_fx.free(&goomInfo->star_fx);
    goomInfo->tentacles_fx.free(&goomInfo->tentacles_fx);
    goomInfo->zoomFilter_fx.free(&goomInfo->zoomFilter_fx);

    // Release info visual
    free (goomInfo->params);
    free (goomInfo->sound.params.params);

    // Release PluginInfo
    free (goomInfo->visuals);
    gsl_free (goomInfo->scanner);
    gsl_free (goomInfo->main_scanner);
    
    free(goomInfo);
}


/* *** */
void
choose_a_goom_line (PluginInfo *goomInfo, float *param1, float *param2, int *couleur, int *mode,
                    float *amplitude, int far)
{
    *mode = goom_irand(goomInfo->gRandom,3);
    *amplitude = 1.0f;
    switch (*mode) {
        case GML_CIRCLE:
            if (far) {
                *param1 = *param2 = 0.47f;
                *amplitude = 0.8f;
                break;
            }
            if (goom_irand(goomInfo->gRandom,3) == 0) {
                *param1 = *param2 = 0;
                *amplitude = 3.0f;
            }
            else if (goom_irand(goomInfo->gRandom,2)) {
                *param1 = 0.40f * goomInfo->screen.height;
                *param2 = 0.22f * goomInfo->screen.height;
            }
            else {
                *param1 = *param2 = goomInfo->screen.height * 0.35;
            }
            break;
        case GML_HLINE:
            if (goom_irand(goomInfo->gRandom,4) || far) {
                *param1 = goomInfo->screen.height / 7;
                *param2 = 6.0f * goomInfo->screen.height / 7.0f;
            }
            else {
                *param1 = *param2 = goomInfo->screen.height / 2.0f;
                *amplitude = 2.0f;
            }
            break;
        case GML_VLINE:
            if (goom_irand(goomInfo->gRandom,3) || far) {
                *param1 = goomInfo->screen.width / 7.0f;
                *param2 = 6.0f * goomInfo->screen.width / 7.0f;
            }
            else {
                *param1 = *param2 = goomInfo->screen.width / 2.0f;
                *amplitude = 1.5f;
            }
            break;
    }
    
    *couleur = goom_irand(goomInfo->gRandom,6);
}

#define ECART_VARIATION 1.5
#define POS_VARIATION 3.0
#define SCROLLING_SPEED 80

/*
 * Met a jour l'affichage du message defilant
 */
void update_message (PluginInfo *goomInfo, char *message) {
    
    int fin = 0;
    
    if (message) {
        int i=1,j=0;
        sprintf (goomInfo->update_message.message, message);
        for (j=0;goomInfo->update_message.message[j];j++)
            if (goomInfo->update_message.message[j]=='\n')
                i++;
        goomInfo->update_message.numberOfLinesInMessage = i;
        goomInfo->update_message.affiche = goomInfo->screen.height + goomInfo->update_message.numberOfLinesInMessage * 25 + 105;
        goomInfo->update_message.longueur = strlen(goomInfo->update_message.message);
    }
    if (goomInfo->update_message.affiche) {
        int i = 0;
        char *msg = malloc(goomInfo->update_message.longueur + 1);
        char *ptr = msg;
        int pos;
        float ecart;
        message = msg;
        sprintf (msg, goomInfo->update_message.message);
        
        while (!fin) {
            while (1) {
                if (*ptr == 0) {
                    fin = 1;
                    break;
                }
                if (*ptr == '\n') {
                    *ptr = 0;
                    break;
                }
                ++ptr;
            }
            pos = goomInfo->update_message.affiche - (goomInfo->update_message.numberOfLinesInMessage - i)*25;
            pos += POS_VARIATION * (cos((double)pos / 20.0));
            pos -= SCROLLING_SPEED;
            ecart = (ECART_VARIATION * sin((double)pos / 20.0));
            if ((fin) && (2 * pos < (int)goomInfo->screen.height))
                pos = (int)goomInfo->screen.height / 2;
            pos += 7;
            
            goom_draw_text(goomInfo->p1,goomInfo->screen.width,goomInfo->screen.height,
                           goomInfo->screen.width/2, pos,
                           message,
                           ecart,
                           1);
            message = ++ptr;
            i++;
        }
        goomInfo->update_message.affiche --;
        free (msg);
    }
}

