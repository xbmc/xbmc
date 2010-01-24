#ifndef LOAD_PAT_H
#define LOAD_PAT_H

#ifdef __cplusplus
extern "C" {
#endif

void pat_init_patnames(void);
void pat_resetsmp(void);
int pat_numinstr(void);
int pat_numsmp(void);
int pat_smptogm(int smp);
int pat_gmtosmp(int gm);
int pat_gm_drumnr(int n);
int pat_gm_drumnote(int n);
const char *pat_gm_name(int gm);
int pat_modnote(int midinote);
int pat_smplooped(int smp);
//#ifdef NEWMIKMOD
BOOL PAT_Load_Instruments(void *c);
//#endif

#ifdef __cplusplus
}
#endif

#endif
