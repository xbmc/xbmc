#include "sound_tester.h"

#include <stdlib.h>
#include <string.h>

/* some constants */
#define BIG_GOOM_DURATION 100
#define BIG_GOOM_SPEED_LIMIT 0.1f

#define ACCEL_MULT 0.95f
#define SPEED_MULT 0.99f


void evaluate_sound(gint16 data[2][512], SoundInfo *info) {

	int i;
	float difaccel;
  float prevspeed;

	/* find the max */
	int incvar = 0;
	for (i = 0; i < 512; i+=2) {
		if (incvar < data[0][i])
			incvar = data[0][i];
	}

	if (incvar > info->allTimesMax)
		info->allTimesMax = incvar;

	/* volume sonore */
	info->volume = (float)incvar / (float)info->allTimesMax;
	memcpy(info->samples[0],data[0],512*sizeof(short));
	memcpy(info->samples[1],data[1],512*sizeof(short));

	difaccel = info->accelvar;
	info->accelvar = info->volume; /* accel entre 0 et 1 */

	/* transformations sur la vitesse du son */
	if (info->speedvar > 1.0f)
		info->speedvar = 1.0f;

	if (info->speedvar < 0.1f)
		info->accelvar *= (1.0f - (float)info->speedvar);
	else if (info->speedvar < 0.3f)
		info->accelvar *= (0.9f - (float)(info->speedvar-0.1f)/2.0f);
	else
		info->accelvar *= (0.8f - (float)(info->speedvar-0.3f)/4.0f);

	/* adoucissement de l'acceleration */
	info->accelvar *= ACCEL_MULT;
	if (info->accelvar < 0)
		info->accelvar = 0;

	difaccel = info->accelvar - difaccel;
	if (difaccel < 0)
		difaccel = - difaccel;

	/* mise a jour de la vitesse */
  prevspeed = info->speedvar;
	info->speedvar = (info->speedvar + difaccel * 0.5f) / 2;
	info->speedvar *= SPEED_MULT;
  info->speedvar = (info->speedvar + 3.0f * prevspeed) / 4.0f;
	if (info->speedvar < 0)
		info->speedvar = 0;
	if (info->speedvar > 1)
		info->speedvar = 1;

	/* temps du goom */
	info->timeSinceLastGoom++;
	info->timeSinceLastBigGoom++;
	info->cycle++;

	/* detection des nouveaux gooms */
	if ((info->speedvar > (float)IVAL(info->biggoom_speed_limit_p)/100.0f)
			&& (info->accelvar > info->bigGoomLimit)
			&& (info->timeSinceLastBigGoom > BIG_GOOM_DURATION)) {
		info->timeSinceLastBigGoom = 0;
	}

	if (info->accelvar > info->goom_limit) {
		/* TODO: tester && (info->timeSinceLastGoom > 20)) { */
		info->totalgoom ++;
		info->timeSinceLastGoom = 0;
		info->goomPower = info->accelvar - info->goom_limit;    
	}

	if (info->accelvar > info->prov_max)
		info->prov_max = info->accelvar;

	if (info->goom_limit>1)
		info->goom_limit=1;

	/* toute les 2 secondes : vérifier si le taux de goom est correct
	 * et le modifier sinon.. */
	if (info->cycle % 64 == 0) {
		if (info->speedvar<0.01f)
			info->goom_limit *= 0.91;
		if (info->totalgoom > 4) {
			info->goom_limit+=0.02;
		}
		if (info->totalgoom > 7) {
			info->goom_limit*=1.03f;
			info->goom_limit+=0.03;
		}
		if (info->totalgoom > 16) {
			info->goom_limit*=1.05f;
			info->goom_limit+=0.04;
		}
		if (info->totalgoom == 0) {
			info->goom_limit = info->prov_max - 0.02;
		}
		if ((info->totalgoom == 1) && (info->goom_limit > 0.02))
			info->goom_limit-=0.01;
		info->totalgoom = 0;
		info->bigGoomLimit = info->goom_limit * (1.0f + (float)IVAL(info->biggoom_factor_p)/500.0f);
		info->prov_max = 0;
	}

	/* mise a jour des parametres pour la GUI */
	FVAL(info->volume_p) = info->volume;
	info->volume_p.change_listener(&info->volume_p);
	FVAL(info->speed_p) = info->speedvar * 4;
	info->speed_p.change_listener(&info->speed_p);
	FVAL(info->accel_p) = info->accelvar;
	info->accel_p.change_listener(&info->accel_p);

	FVAL(info->goom_limit_p) = info->goom_limit;
	info->goom_limit_p.change_listener(&info->goom_limit_p);
        FVAL(info->goom_power_p) = info->goomPower;
        info->goom_power_p.change_listener(&info->goom_power_p);
	FVAL(info->last_goom_p) = 1.0-((float)info->timeSinceLastGoom/20.0f);
	info->last_goom_p.change_listener(&info->last_goom_p);
	FVAL(info->last_biggoom_p) = 1.0-((float)info->timeSinceLastBigGoom/40.0f);
	info->last_biggoom_p.change_listener(&info->last_biggoom_p);

	/* bigGoomLimit ==goomLimit*9/8+7 ? */
	}

