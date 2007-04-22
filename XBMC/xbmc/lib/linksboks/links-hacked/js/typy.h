/* typy.h
 * (c) 2002 Martin 'PerM' Pergel
 * This file is a part of the Links program, released under GPL.
 */

#define INTEGER 1
#define FLOAT 2
#define UNDEFINED 0
#define NULLOVY 3
#define BOOLEAN 4
#define STRING 5
#define VARIABLE 6
#define FUNKCE 7
#define FUNKINT 8
#define ARGUMENTY 9
#define ADDRSPACE 10
#define ADDRSPACEP 11 /*Prirazeny addrspace*/
#define VARINT 12
/* interni variable - bude jich dost */
#define ARRAY 13
#define PARLIST 14
#define INTVAR 15

#define MAINADDRSPC 10

#define TRUE 1 /*Todle je dulezite! Stoji na tom logicka aritmetika!*/
#define FALSE 0
void delarg(abuf*,js_context*);
void clearvar(lns*,js_context*);
void vartoarg(lns*,abuf*,js_context*);

