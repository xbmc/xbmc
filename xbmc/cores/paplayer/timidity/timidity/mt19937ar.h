
#ifndef ___MT19937AR_H_
#define ___MT19937AR_H_

extern void init_genrand(unsigned long);
extern void init_by_array(unsigned long [], unsigned long);
extern unsigned long genrand_int32(void);
extern long genrand_int31(void);
extern double genrand_real1(void);

#endif /* ___MT19937AR_H_ */
