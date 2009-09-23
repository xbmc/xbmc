/*---------------------------------------------------------------------------*/
/*
** config_param.c
** Goom Project
**
** Created by Jean-Christophe Hoelt on Sat Jul 19 2003
** Copyright (c) 2003 iOS. All rights reserved.
*/
/*---------------------------------------------------------------------------*/

#include "goom_config_param.h"
#include <string.h>

/* TODO: Ajouter goom_ devant ces fonctions */

static void empty_fct(PluginParam *dummy) {
}

PluginParam goom_secure_param() {
  PluginParam p;
  p.changed = empty_fct;
  p.change_listener = empty_fct;
  p.user_data = 0;
  p.name = p.desc = 0;
  p.rw = 1;
  return p;
}

PluginParam goom_secure_f_param(char *name) {
  PluginParam p = secure_param();
  p.name = name;
  p.type = PARAM_FLOATVAL;
  FVAL(p) = 0.5f;
  FMIN(p) = 0.0f;
  FMAX(p) = 1.0f;
  FSTEP(p) = 0.01f;
  return p;
}

PluginParam goom_secure_f_feedback(char *name) {
  PluginParam p = secure_f_param(name);
  p.rw = 0;
  return p;
}

PluginParam goom_secure_s_param(char *name) {
    PluginParam p = secure_param();
    p.name = name;
    p.type = PARAM_STRVAL;
    SVAL(p) = 0;
    return p;
}

PluginParam goom_secure_b_param(char *name, int value) {
  PluginParam p = secure_param();
  p.name = name;
  p.type = PARAM_BOOLVAL;
  BVAL(p) = value;
  return p;
}

PluginParam goom_secure_i_param(char *name) {
  PluginParam p = secure_param();
  p.name = name;
  p.type = PARAM_INTVAL;
  IVAL(p) = 50;
  IMIN(p) = 0;
  IMAX(p) = 100;
  ISTEP(p) = 1;
  return p;
}

PluginParam goom_secure_i_feedback(char *name) {
  PluginParam p = secure_i_param(name);
  p.rw = 0;
  return p;
}

PluginParameters goom_plugin_parameters(const char *name, int nb) {
  PluginParameters p;
  p.name = (char *)name;
  p.desc = "";
  p.nbParams = nb;
  p.params = (PluginParam**)malloc(nb*sizeof(PluginParam*));
  return p;
}

/*---------------------------------------------------------------------------*/

void goom_set_str_param_value(PluginParam *p, const char *str) {
    int len = strlen(str);
    if (SVAL(*p))
        SVAL(*p) = (char*)realloc(SVAL(*p), len+1);
    else
        SVAL(*p) = (char*)malloc(len+1);
    memcpy(SVAL(*p), str, len+1);
}

void goom_set_list_param_value(PluginParam *p, const char *str) {
    int len = strlen(str);
#ifdef VERBOSE
    printf("%s: %d\n", str, len);
#endif
    if (LVAL(*p))
        LVAL(*p) = (char*)realloc(LVAL(*p), len+1);
    else
        LVAL(*p) = (char*)malloc(len+1);
    memcpy(LVAL(*p), str, len+1);
}

