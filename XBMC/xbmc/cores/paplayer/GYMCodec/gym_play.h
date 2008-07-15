/////////////////////////////////////////////////////////////////////////////////////////////
// SOUND.H
/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef SOUND_H
#define SOUND_H

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

extern int* Seg_L, *Seg_R;
extern int Seg_Lenght;

extern unsigned int Sound_Extrapol[312][2];

void Start_Play_GYM(int sampleRate);
unsigned char *Play_GYM(void *Dump_Buf, unsigned char *gym_start, unsigned char *gym_pos, unsigned int gym_size, unsigned int gym_loop);

unsigned char *jump_gym_time_pos(unsigned char *gym_start, unsigned int gym_size, unsigned int new_pos);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif

