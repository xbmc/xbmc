/* ipret.h
 * (c) 2002 Martin 'PerM' Pergel
 * This file is a part of the Links program, released under GPL.
 */

#ifdef DEBUGMEMORY
#define pulla(a) real_pulla(a,__FILE__,__LINE__)
#define pusha(a,b) real_pusha(a,b,__FILE__,__LINE__)
abuf* real_pulla(js_context*,unsigned char*,int);
void real_pusha(abuf*,js_context*,unsigned char*,int);
#else
abuf* pulla(js_context*);
void pusha(abuf*,js_context*);
#endif
char* tostring(abuf*,js_context*);
vrchol* pullp(js_context*);
float tofloat(abuf*,js_context*);
#define DELKACISLA 25 /*Kvalifikovany odhad, kolika znaku se muze dobrat cislo pri konverzi do stringu ZDE JE HOLE!*/
int to32int(abuf*,js_context*);
void js_error(char*,js_context*);
int tobool(abuf*,js_context*);
