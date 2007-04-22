/* ns.h
 * Javascript namespace
 * (c) 2002 Martin 'PerM' Pergel
 * This file is a part of the Links program, released under GPL
 */

abuf* getarg(abuf**);
void add_to_parlist(lns*,lns*);
void zrusargy(abuf*,js_context*);
void delete_from_parlist(lns*,lns*);
lns* llookup(char*,js_id_name **,plns*,js_context*);
