/* buitin.h
 * (c) 2002 Martin 'PerM' Pergel
 * This file is a part of the Links program, released under GPL.
 */

void get_var_value(lns*,long*,long*,js_context*);
void kill_var(lns*);
void set_var_value(lns*,long,long,js_context*);
void js_intern_fupcall(js_context*,long,lns*);
void add_builtin(js_context*);
