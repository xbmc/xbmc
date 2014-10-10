#include "goom_fx.h"
#include "goom_plugin_info.h"
#include "goom_tools.h"

#include "mathtools.h"

/* TODO:-- FAIRE PROPREMENT... BOAH... */
#define NCOL 15

/*static const int colval[] = {
0xfdf6f5,
0xfae4e4,
0xf7d1d1,
0xf3b6b5,
0xefa2a2,
0xec9190,
0xea8282,
0xe87575,
0xe46060,
0xe14b4c,
0xde3b3b,
0xdc2d2f,
0xd92726,
0xd81619,
0xd50c09,
0
};
*/
static const int colval[] = {
	0x1416181a,
	0x1419181a,
	0x141f181a,
	0x1426181a,
	0x142a181a,
	0x142f181a,
	0x1436181a,
	0x142f1819,
	0x14261615,
	0x13201411,
	0x111a100a,
	0x0c180508,
	0x08100304,
	0x00050101,
	0x0
};


/* The different modes of the visual FX.
 * Put this values on fx_mode */
#define FIREWORKS_FX 0
#define RAIN_FX 1
#define FOUNTAIN_FX 2
#define LAST_FX 3

typedef struct _FS_STAR {
	float x,y;
	float vx,vy;
	float ax,ay;
	float age,vage;
} Star;

typedef struct _FS_DATA{

	int fx_mode;
	int nbStars;

	int maxStars;
	Star *stars;

	float min_age;
	float max_age;

	PluginParam min_age_p;
	PluginParam max_age_p;
	PluginParam nbStars_p;
	PluginParam nbStars_limit_p;
	PluginParam fx_mode_p;

	PluginParameters params;
} FSData;

static void fs_init(VisualFX *_this, PluginInfo *info) {
	
	FSData *data;
	data = (FSData*)malloc(sizeof(FSData));

	data->fx_mode = FIREWORKS_FX;
	data->maxStars = 4096;
	data->stars = (Star*)malloc(data->maxStars * sizeof(Star));
	data->nbStars = 0;

	data->max_age_p = secure_i_param ("Fireworks Smallest Bombs");
	IVAL(data->max_age_p) = 80;
	IMIN(data->max_age_p) = 0;
	IMAX(data->max_age_p) = 100;
	ISTEP(data->max_age_p) = 1;

	data->min_age_p = secure_i_param ("Fireworks Largest Bombs");
	IVAL(data->min_age_p) = 99;
	IMIN(data->min_age_p) = 0;
	IMAX(data->min_age_p) = 100;
	ISTEP(data->min_age_p) = 1;

	data->nbStars_limit_p = secure_i_param ("Max Number of Particules");
	IVAL(data->nbStars_limit_p) = 512;
	IMIN(data->nbStars_limit_p) = 0;
	IMAX(data->nbStars_limit_p) = data->maxStars;
	ISTEP(data->nbStars_limit_p) = 64;

	data->fx_mode_p = secure_i_param ("FX Mode");
	IVAL(data->fx_mode_p) = data->fx_mode;
	IMIN(data->fx_mode_p) = 1;
	IMAX(data->fx_mode_p) = 3;
	ISTEP(data->fx_mode_p) = 1;

	data->nbStars_p = secure_f_feedback ("Number of Particules (% of Max)");

	data->params = plugin_parameters ("Particule System", 7);
	data->params.params[0] = &data->fx_mode_p;
	data->params.params[1] = &data->nbStars_limit_p;
	data->params.params[2] = 0;
	data->params.params[3] = &data->min_age_p;
	data->params.params[4] = &data->max_age_p;
	data->params.params[5] = 0;
	data->params.params[6] = &data->nbStars_p;

	_this->params = &data->params;
	_this->fx_data = (void*)data;
}

static void fs_free(VisualFX *_this) {
       FSData *data = (FSData*)_this->fx_data;
       free (data->stars);
       free (data->params.params);
	free (data);
}


/**
 * Cree une nouvelle 'bombe', c'est a dire une particule appartenant a une fusee d'artifice.
 */
static void addABomb (FSData *fs, int mx, int my, float radius, float vage, float gravity, PluginInfo *info) {

	int i = fs->nbStars;
	float ro;
	int theta;

	if (fs->nbStars >= fs->maxStars)
		return;
	fs->nbStars++;

	fs->stars[i].x = mx;
	fs->stars[i].y = my;

	ro = radius * (float)goom_irand(info->gRandom,100) / 100.0f;
	ro *= (float)goom_irand(info->gRandom,100)/100.0f + 1.0f;
	theta = goom_irand(info->gRandom,256);

	fs->stars[i].vx = ro * cos256[theta];
	fs->stars[i].vy = -0.2f + ro * sin256[theta];

	fs->stars[i].ax = 0;
	fs->stars[i].ay = gravity;

	fs->stars[i].age = 0;
	if (vage < fs->min_age)
		vage=fs->min_age;
	fs->stars[i].vage = vage;
}


/**
 * Met a jour la position et vitesse d'une particule.
 */
static void updateStar (Star *s) {
	s->x+=s->vx;
	s->y+=s->vy;
	s->vx+=s->ax;
	s->vy+=s->ay;
	s->age+=s->vage;
}


/**
 * Ajoute de nouvelles particules au moment d'un evenement sonore.
 */
static void fs_sound_event_occured(VisualFX *_this, PluginInfo *info) {

	FSData *data = (FSData*)_this->fx_data;
	int i;

	int max = (int)((1.0f+info->sound.goomPower)*goom_irand(info->gRandom,150)) + 100;
	float radius = (1.0f+info->sound.goomPower) * (float)(goom_irand(info->gRandom,150)+50)/300;
	int mx;
	int my;
	float vage, gravity = 0.02f;

	switch (data->fx_mode) {
		case FIREWORKS_FX:
      {  
      double dx,dy;
      do {
        mx = goom_irand(info->gRandom,info->screen.width);
	  		my = goom_irand(info->gRandom,info->screen.height);
        dx = (mx - info->screen.width/2);
        dy = (my - info->screen.height/2);
      } while (dx*dx + dy*dy < (info->screen.height/2)*(info->screen.height/2));
			vage = data->max_age * (1.0f - info->sound.goomPower);
      }
			break;
		case RAIN_FX:
      mx = goom_irand(info->gRandom,info->screen.width);
      if (mx > info->screen.width/2)
        mx = info->screen.width;
      else
        mx = 0;
			my = -(info->screen.height/3)-goom_irand(info->gRandom,info->screen.width/3);
			radius *= 1.5;
			vage = 0.002f;
			break;
		case FOUNTAIN_FX:
			my = info->screen.height+2;
			vage = 0.001f;
			radius += 1.0f;
			mx = info->screen.width / 2;
			gravity = 0.04f;
			break;
		default:
			return;
			/* my = i R A N D (info->screen.height); vage = 0.01f; */
	}

	radius *= info->screen.height / 200.0f; /* why 200 ? because the FX was developped on 320x200 */
	max *= info->screen.height / 200.0f;

	if (info->sound.timeSinceLastBigGoom < 1) {
		radius *= 1.5;
		max *= 2;
	}
	for (i=0;i<max;++i)
		addABomb (data,mx,my,radius,vage,gravity,info);
}


/**
 * Main methode of the FX.
 */
static void fs_apply(VisualFX *_this, Pixel *src, Pixel *dest, PluginInfo *info) {

	int i;
	int col;
	FSData *data = (FSData*)_this->fx_data;

	/* Get the new parameters values */
	data->min_age = 1.0f - (float)IVAL(data->min_age_p)/100.0f;
	data->max_age = 1.0f - (float)IVAL(data->max_age_p)/100.0f;
	FVAL(data->nbStars_p) = (float)data->nbStars / (float)data->maxStars;
	data->nbStars_p.change_listener(&data->nbStars_p);
	data->maxStars = IVAL(data->nbStars_limit_p);
	data->fx_mode = IVAL(data->fx_mode_p);

	/* look for events */
	if (info->sound.timeSinceLastGoom < 1) {
		fs_sound_event_occured(_this, info);
		if (goom_irand(info->gRandom,20)==1) {
			IVAL(data->fx_mode_p) = goom_irand(info->gRandom,(LAST_FX*3));
			data->fx_mode_p.change_listener(&data->fx_mode_p);
		}
	}

	/* update particules */
	for (i=0;i<data->nbStars;++i) {
		updateStar(&data->stars[i]);

		/* dead particule */
		if (data->stars[i].age>=NCOL)
			continue;

		/* choose the color of the particule */
		col = colval[(int)data->stars[i].age];

		/* draws the particule */
		info->methods.draw_line(dest,(int)data->stars[i].x,(int)data->stars[i].y,
				(int)(data->stars[i].x-data->stars[i].vx*6),
				(int)(data->stars[i].y-data->stars[i].vy*6),
				col,
				(int)info->screen.width, (int)info->screen.height);
		info->methods.draw_line(dest,(int)data->stars[i].x,(int)data->stars[i].y,
				(int)(data->stars[i].x-data->stars[i].vx*2),
				(int)(data->stars[i].y-data->stars[i].vy*2),
				col,
				(int)info->screen.width, (int)info->screen.height);
	}

	/* look for dead particules */
	for (i=0;i<data->nbStars;) {

		if ((data->stars[i].x > info->screen.width + 64)
				||((data->stars[i].vy>=0)&&(data->stars[i].y - 16*data->stars[i].vy > info->screen.height))
				||(data->stars[i].x < -64)
				||(data->stars[i].age>=NCOL)) {
			data->stars[i] = data->stars[data->nbStars-1];
			data->nbStars--;
		}
		else ++i;
	}
}

VisualFX flying_star_create(void) {
  VisualFX vfx;
  vfx.init = fs_init;
  vfx.free = fs_free;
  vfx.apply = fs_apply;
  vfx.fx_data = 0;
  return vfx;
}
