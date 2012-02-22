/*
 *  Functions converting HTSMSGs to/from JSON
 *  Copyright (C) 2008 Andreas Öman
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "htsmsg_json.h"
#include "htsbuf.h"

/**
 *
 */
static void
htsmsg_json_encode_string(const char *str, htsbuf_queue_t *hq)
{
  const char *s = str;

  htsbuf_append(hq, "\"", 1);

  while(*s != 0) {
    if(*s == '"' || *s == '\\' || *s == '\n' || *s == '\t' || *s == '\r') {
      htsbuf_append(hq, str, s - str);

      if(*s == '"')
	htsbuf_append(hq, "\\\"", 2);
      else if(*s == '\n') 
	htsbuf_append(hq, "\\n", 2);
      else if(*s == '\t') 
	htsbuf_append(hq, "\\t", 2);
      else if(*s == '\r')
	htsbuf_append(hq, "\\r", 2);
      else
	htsbuf_append(hq, "\\\\", 2);
      s++;
      str = s;
    } else {
      s++;
    }
  }
  htsbuf_append(hq, str, s - str);
  htsbuf_append(hq, "\"", 1);
}


/*
 * Note to future:
 * If your about to add support for numbers with decimal point,
 * remember to always serialize them with '.' as decimal point character
 * no matter what current locale says. This is according to the JSON spec.
 */
static void
htsmsg_json_write(htsmsg_t *msg, htsbuf_queue_t *hq, int isarray,
		  int indent, int pretty)
{
  htsmsg_field_t *f;
  char buf[30];
  static const char *indentor = "\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

  htsbuf_append(hq, isarray ? "[" : "{", 1);

  TAILQ_FOREACH(f, &msg->hm_fields, hmf_link) {

    if(pretty) 
      htsbuf_append(hq, indentor, indent < 16 ? indent : 16);

    if(!isarray) {
      htsmsg_json_encode_string(f->hmf_name ?: "noname", hq);
      htsbuf_append(hq, ": ", 2);
    }

    switch(f->hmf_type) {
    case HMF_MAP:
      htsmsg_json_write(&f->hmf_msg, hq, 0, indent + 1, pretty);
      break;

    case HMF_LIST:
      htsmsg_json_write(&f->hmf_msg, hq, 1, indent + 1, pretty);
      break;

    case HMF_STR:
      htsmsg_json_encode_string(f->hmf_str, hq);
      break;

    case HMF_BIN:
      htsmsg_json_encode_string("binary", hq);
      break;

    case HMF_S64:
      snprintf(buf, sizeof(buf), "%" PRId64, f->hmf_s64);
      htsbuf_append(hq, buf, strlen(buf));
      break;

    default:
      abort();
    }

    if(TAILQ_NEXT(f, hmf_link))
      htsbuf_append(hq, ",", 1);
  }
  
  if(pretty) 
    htsbuf_append(hq, indentor, indent-1 < 16 ? indent-1 : 16);
  htsbuf_append(hq, isarray ? "]" : "}", 1);
}

/**
 *
 */
int
htsmsg_json_serialize(htsmsg_t *msg, htsbuf_queue_t *hq, int pretty)
{
  htsmsg_json_write(msg, hq, msg->hm_islist, 2, pretty);
  if(pretty) 
    htsbuf_append(hq, "\n", 1);
  return 0;
}



static const char *htsmsg_json_parse_value(const char *s, 
					   htsmsg_t *parent, char *name);

/**
 *
 */
static char *
htsmsg_json_parse_string(const char *s, const char **endp)
{
  const char *start;
  char *r, *a, *b;
  int l, esc = 0;

  while(*s > 0 && *s < 33)
    s++;

  if(*s != '"')
    return NULL;

  s++;
  start = s;

  while(1) {
    if(*s == 0)
      return NULL;

    if(*s == '\\') {
      esc = 1;
    } else if(*s == '"' && s[-1] != '\\') {

      *endp = s + 1;

      /* End */
      l = s - start;
      r = malloc(l + 1);
      memcpy(r, start, l);
      r[l] = 0;

      if(esc) {
	/* Do deescaping inplace */

	a = b = r;

	while(*a) {
	  if(*a == '\\') {
	    a++;
	    if(*a == 'b')
	      *b++ = '\b';
	    else if(*a == 'f')
	      *b++ = '\f';
	    else if(*a == 'n')
	      *b++ = '\n';
	    else if(*a == 'r')
	      *b++ = '\r';
	    else if(*a == 't')
	      *b++ = '\t';
	    else if(*a == 'u') {
	      /* 4 hexdigits: Not supported */
	      free(r);
	      return NULL;
	    } else {
	      *b++ = *a;
	    }
	    a++;
	  } else {
	    *b++ = *a++;
	  }
	}
	*b = 0;
      }
      return r;
    }
    s++;
  }
}


/**
 *
 */
static htsmsg_t *
htsmsg_json_parse_object(const char *s, const char **endp)
{
  char *name;
  const char *s2;
  htsmsg_t *r;

  while(*s > 0 && *s < 33)
    s++;

  if(*s != '{')
    return NULL;

  s++;

  r = htsmsg_create_map();
  
  while(1) {

    if((name = htsmsg_json_parse_string(s, &s2)) == NULL) {
      htsmsg_destroy(r);
      return NULL;
    }

    s = s2;
    
    while(*s > 0 && *s < 33)
      s++;

    if(*s != ':') {
      htsmsg_destroy(r);
      free(name);
      return NULL;
    }
    s++;

    s2 = htsmsg_json_parse_value(s, r, name);
    free(name);

    if(s2 == NULL) {
      htsmsg_destroy(r);
      return NULL;
    }

    s = s2;

    while(*s > 0 && *s < 33)
      s++;

    if(*s == '}')
      break;

    if(*s != ',') {
      htsmsg_destroy(r);
      return NULL;
    }
    s++;
  }

  s++;
  *endp = s;
  return r;
}


/**
 *
 */
static htsmsg_t *
htsmsg_json_parse_array(const char *s, const char **endp)
{
  const char *s2;
  htsmsg_t *r;

  while(*s > 0 && *s < 33)
    s++;

  if(*s != '[')
    return NULL;

  s++;

  r = htsmsg_create_list();
  
  while(*s > 0 && *s < 33)
    s++;

  if(*s != ']') {

    while(1) {

      s2 = htsmsg_json_parse_value(s, r, NULL);

      if(s2 == NULL) {
	htsmsg_destroy(r);
	return NULL;
      }

      s = s2;

      while(*s > 0 && *s < 33)
	s++;

      if(*s == ']')
	break;

      if(*s != ',') {
	htsmsg_destroy(r);
	return NULL;
      }
      s++;
    }
  }
  s++;
  *endp = s;
  return r;
}

/* 
 * locale independent strtod.
 * does not support hex floats as the standard strtod
 */
static double
_strntod(const char *s, char decimal_point_char, char **ep)
{
  static char locale_decimal_point_char = 0;
  char buf[64];
  const char *c;
  double d;
  
  /* ugly but very portable way to get decimal point char */ 
  if(locale_decimal_point_char == 0) {
    snprintf(buf, sizeof(buf), "%f", 0.0);
    locale_decimal_point_char = buf[1];
    assert(locale_decimal_point_char != 0);
  }
  
  for(c = s; 
      *c != '\0' &&
      ((*c > 0 && *c < 33) || /* skip whitespace */
       (*c == decimal_point_char || strchr("+-0123456789", *c) != NULL)); c++)
    ;
  
  strncpy(buf, s, c - s); 
  buf[c - s] = '\0';
  
  /* replace if specified char is not same as current locale */
  if(decimal_point_char != locale_decimal_point_char) {
    char *r = strchr(buf, decimal_point_char);
    if(r != NULL)
      *r = locale_decimal_point_char;
  }
  
  d = strtod(buf, ep);
  
  /* figure out offset in original string */
  if(ep != NULL)
    *ep = (char *)s + (*ep - buf);
  
  return d;
}

/**
 *
 */
static char *
htsmsg_json_parse_number(const char *s, double *dp)
{
  char *ep;
  double d = _strntod(s, '.', &ep);

  if(ep == s)
    return NULL;

  *dp = d;
  return ep;
}

/**
 *
 */
static const char *
htsmsg_json_parse_value(const char *s, htsmsg_t *parent, char *name)
{
  const char *s2;
  char *str;
  double d = 0;
  htsmsg_t *c;

  if((c = htsmsg_json_parse_object(s, &s2)) != NULL) {
    htsmsg_add_msg(parent, name, c);
    return s2;
  } else if((c = htsmsg_json_parse_array(s, &s2)) != NULL) {
    htsmsg_add_msg(parent, name, c);
    return s2;
  } else if((str = htsmsg_json_parse_string(s, &s2)) != NULL) {
    htsmsg_add_str(parent, name, str);
    free(str);
    return s2;
  } else if((s2 = htsmsg_json_parse_number(s, &d)) != NULL) {
    htsmsg_add_s64(parent, name, d);
    return s2;
  }

  if(!strncmp(s, "true", 4)) {
    htsmsg_add_u32(parent, name, 1);
    return s + 4;
  }

  if(!strncmp(s, "false", 5)) {
    htsmsg_add_u32(parent, name, 0);
    return s + 5;
  }

  if(!strncmp(s, "null", 4)) {
    /* Don't add anything */
    return s + 4;
  }
  return NULL;
}


/**
 *
 */
htsmsg_t *
htsmsg_json_deserialize(const char *src)
{
  const char *end;
  htsmsg_t *c;

  if((c = htsmsg_json_parse_object(src, &end)) != NULL)
    return c;

  if((c = htsmsg_json_parse_array(src, &end)) != NULL) {
      c->hm_islist = 1;
      return c;
  }
  return NULL;
}
