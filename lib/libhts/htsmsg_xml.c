/*
 *  Functions converting HTSMSGs to/from XML
 *  Copyright (C) 2008 Andreas �man
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

/**
 * XML parser, written according to this spec:
 *
 * http://www.w3.org/TR/2006/REC-xml-20060816/
 *
 * Parses of UTF-8 and ISO-8859-1 (Latin 1) encoded XML and output as
 * htsmsg's with UTF-8 encoded payloads
 *
 *  Supports:                             Example:
 *  
 *  Comments                              <!--  a comment               -->
 *  Processing Instructions               <?xml                          ?>
 *  CDATA                                 <![CDATA[  <litteraly copied> ]]>
 *  Label references                      &amp;
 *  Character references                  &#65;
 *  Empty tags                            <tagname/>
 *
 *
 *  Not supported:
 *
 *  UTF-16 (mandatory by standard)
 *  Intelligent parsing of <!DOCTYPE>
 *  Entity declarations
 *
 */


#include <assert.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "htsmsg_xml.h"
#include "htsbuf.h"

TAILQ_HEAD(cdata_content_queue, cdata_content);

LIST_HEAD(xmlns_list, xmlns);

typedef struct xmlns {
  LIST_ENTRY(xmlns) xmlns_global_link;
  LIST_ENTRY(xmlns) xmlns_scope_link;

  char *xmlns_prefix;
  unsigned int xmlns_prefix_len;

  char *xmlns_norm;
  unsigned int xmlns_norm_len;

} xmlns_t;

typedef struct xmlparser {
  enum {
    XML_ENCODING_UTF8,
    XML_ENCODING_8859_1,
  } xp_encoding;

  char xp_errmsg[128];

  int xp_srcdataused;

  struct xmlns_list xp_namespaces;

} xmlparser_t;

#define xmlerr(xp, fmt...) \
 snprintf((xp)->xp_errmsg, sizeof((xp)->xp_errmsg), fmt)


typedef struct cdata_content {
  TAILQ_ENTRY(cdata_content) cc_link;
  char *cc_start, *cc_end; /* end points to byte AFTER last char */
  int cc_encoding;
  char cc_buf[0];
} cdata_content_t;

static char *htsmsg_xml_parse_cd(xmlparser_t *xp, 
				 htsmsg_t *parent, char *src);

/**
 *
 */
static void
add_unicode(struct cdata_content_queue *ccq, int c)
{
  cdata_content_t *cc;
  int r;

  cc = malloc(sizeof(cdata_content_t) + 6);
  r = put_utf8(cc->cc_buf, c);
  if(r == 0) {
    free(cc);
    return;
  }

  cc->cc_encoding = XML_ENCODING_UTF8;
  TAILQ_INSERT_TAIL(ccq, cc, cc_link);
  cc->cc_start = cc->cc_buf;
  cc->cc_end   = cc->cc_buf + r;
}

/**
 *
 */
static int
decode_character_reference(char **src)
{
  int v = 0;
  char c;

  if(**src == 'x') {
    /* hexadecimal */
    (*src)++;

    /* decimal */
    while(1) {
      c = **src;
      switch(c) {
      case '0' ... '9':
	v = v * 0x10 + c - '0';
	break;
      case 'a' ... 'f':
	v = v * 0x10 + c - 'a' + 10;
	break;
      case 'A' ... 'F':
	v = v * 0x10 + c - 'A' + 10;
	break;
      case ';':
	(*src)++;
	return v;
      default:
	return 0;
      }
      (*src)++;
    }

  } else {

    /* decimal */
    while(1) {
      c = **src;
      switch(c) {
      case '0' ... '9':
	v = v * 10 + c - '0';
	(*src)++;
	break;
      case ';':
	(*src)++;
	return v;
      default:
	return 0;
      }
    }
  }
}

/**
 *
 */
static inline int
is_xmlws(char c)
{
  return c > 0 && c <= 32;
  //  return c == 32 || c == 9 || c = 10 || c = 13;
}


/**
 *
 */
static void
xmlns_destroy(xmlns_t *ns)
{
  LIST_REMOVE(ns, xmlns_global_link);
  LIST_REMOVE(ns, xmlns_scope_link);
  free(ns->xmlns_prefix);
  free(ns->xmlns_norm);
  free(ns);
}

/**
 *
 */
static char *
htsmsg_xml_parse_attrib(xmlparser_t *xp, htsmsg_t *msg, char *src,
			struct xmlns_list *xmlns_scope_list)
{
  char *attribname, *payload;
  int attriblen, payloadlen;
  char quote;
  htsmsg_field_t *f;
  xmlns_t *ns;

  attribname = src;
  /* Parse attribute name */
  while(1) {
    if(*src == 0) {
      xmlerr(xp, "Unexpected end of file during attribute name parsing");
      return NULL;
    }

    if(is_xmlws(*src) || *src == '=')
      break;
    src++;
  }

  attriblen = src - attribname;
  if(attriblen < 1 || attriblen > 65535) {
    xmlerr(xp, "Invalid attribute name");
    return NULL;
  }

  while(is_xmlws(*src))
    src++;

  if(*src != '=') {
    xmlerr(xp, "Expected '=' in attribute parsing");
    return NULL;
  }
  src++;

  while(is_xmlws(*src))
    src++;

  
  /* Parse attribute payload */
  quote = *src++;
  if(quote != '"' && quote != '\'') {
    xmlerr(xp, "Expected ' or \" before attribute value");
    return NULL;
  }

  payload = src;
  while(1) {
    if(*src == 0) {
      xmlerr(xp, "Unexpected end of file during attribute value parsing");
      return NULL;
    }
    if(*src == quote)
      break;
    src++;
  }

  payloadlen = src - payload;
  if(payloadlen < 0 || payloadlen > 65535) {
    xmlerr(xp, "Invalid attribute value");
    return NULL;
  }

  src++;
  while(is_xmlws(*src))
    src++;

  if(xmlns_scope_list != NULL && 
     attriblen > 6 && !memcmp(attribname, "xmlns:", 6)) {

    attribname += 6;
    attriblen  -= 6;

    ns = malloc(sizeof(xmlns_t));

    ns->xmlns_prefix = malloc(attriblen + 1);
    memcpy(ns->xmlns_prefix, attribname, attriblen);
    ns->xmlns_prefix[attriblen] = 0;
    ns->xmlns_prefix_len = attriblen;

    ns->xmlns_norm = malloc(payloadlen + 1);
    memcpy(ns->xmlns_norm, payload, payloadlen);
    ns->xmlns_norm[payloadlen] = 0;
    ns->xmlns_norm_len = payloadlen;

    LIST_INSERT_HEAD(&xp->xp_namespaces, ns, xmlns_global_link);
    LIST_INSERT_HEAD(xmlns_scope_list,   ns, xmlns_scope_link);
    return src;
  }

  xp->xp_srcdataused = 1;
  attribname[attriblen] = 0;
  payload[payloadlen] = 0;

  f = htsmsg_field_add(msg, attribname, HMF_STR, 0);
  f->hmf_str = payload;
  return src;
}

/**
 *
 */
static char *
htsmsg_xml_parse_tag(xmlparser_t *xp, htsmsg_t *parent, char *src)
{
  htsmsg_t *m, *attrs;
  struct xmlns_list nslist;
  char *tagname;
  int taglen, empty = 0, i;
  xmlns_t *ns;

  tagname = src;

  LIST_INIT(&nslist);

  while(1) {
    if(*src == 0) {
      xmlerr(xp, "Unexpected end of file during tag name parsing");
      return NULL;
    }
    if(is_xmlws(*src) || *src == '>' || *src == '/')
      break;
    src++;
  }

  taglen = src - tagname;
  if(taglen < 1 || taglen > 65535) {
    xmlerr(xp, "Invalid tag name");
    return NULL;
  }

  attrs = htsmsg_create_map();

  while(1) {

    while(is_xmlws(*src))
      src++;

    if(*src == 0) {
      htsmsg_destroy(attrs);
      xmlerr(xp, "Unexpected end of file in tag");
      return NULL;
    }

    if(src[0] == '/' && src[1] == '>') {
      empty = 1;
      src += 2;
      break;
    }

    if(*src == '>') {
      src++;
      break;
    }

    if((src = htsmsg_xml_parse_attrib(xp, attrs, src, &nslist)) == NULL) {
      htsmsg_destroy(attrs);
      return NULL;
    }
  }

  m = htsmsg_create_map();

  if(TAILQ_FIRST(&attrs->hm_fields) != NULL) {
    htsmsg_add_msg_extname(m, "attrib", attrs);
  } else {
    htsmsg_destroy(attrs);
  }

  if(!empty)
    src = htsmsg_xml_parse_cd(xp, m, src);

  for(i = 0; i < taglen - 1; i++) {
    if(tagname[i] == ':') {

      LIST_FOREACH(ns, &xp->xp_namespaces, xmlns_global_link) {
	if(ns->xmlns_prefix_len == i && 
	   !memcmp(ns->xmlns_prefix, tagname, ns->xmlns_prefix_len)) {

	  int llen = taglen - i - 1;
	  char *n = malloc(ns->xmlns_norm_len + llen + 1);

	  n[ns->xmlns_norm_len + llen] = 0;
	  memcpy(n, ns->xmlns_norm, ns->xmlns_norm_len);
	  memcpy(n + ns->xmlns_norm_len, tagname + i + 1, llen);
	  htsmsg_add_msg(parent, n, m);
	  free(n);
	  goto done;
	}
      }
    }
  }

  xp->xp_srcdataused = 1;
  tagname[taglen] = 0;
  htsmsg_add_msg_extname(parent, tagname, m);

 done:
  while((ns = LIST_FIRST(&nslist)) != NULL)
    xmlns_destroy(ns);
  return src;
}





/**
 *
 */
static char *
htsmsg_xml_parse_pi(xmlparser_t *xp, htsmsg_t *parent, char *src)
{
  htsmsg_t *attrs;
  char *s = src;
  char *piname;
  int l;

  while(1) {
    if(*src == 0) {
      xmlerr(xp, "Unexpected end of file during parsing of "
	     "Processing instructions");
      return NULL;
    }

    if(is_xmlws(*src) || *src == '?')
      break;
    src++;
  }

  l = src - s;
  if(l < 1 || l > 65536) {
    xmlerr(xp, "Invalid 'Processing instructions' name");
    return NULL;
  }
  piname = alloca(l + 1);
  memcpy(piname, s, l);
  piname[l] = 0;

  attrs = htsmsg_create_map();

  while(1) {

    while(is_xmlws(*src))
      src++;

    if(*src == 0) {
      htsmsg_destroy(attrs);
      xmlerr(xp, "Unexpected end of file during parsing of "
	     "Processing instructions");
      return NULL;
    }

    if(src[0] == '?' && src[1] == '>') {
      src += 2;
      break;
    }

    if((src = htsmsg_xml_parse_attrib(xp, attrs, src, NULL)) == NULL) {
      htsmsg_destroy(attrs);
      return NULL;
    }
  }


  if(TAILQ_FIRST(&attrs->hm_fields) != NULL && parent != NULL) {
    htsmsg_add_msg(parent, piname, attrs);
  } else {
    htsmsg_destroy(attrs);
  }
  return src;
}


/**
 *
 */
static char *
xml_parse_comment(xmlparser_t *xp, char *src)
{
  /* comment */
  while(1) {
    if(*src == 0) { /* EOF inside comment is invalid */
      xmlerr(xp, "Unexpected end of file inside a comment");
      return NULL;
    }

    if(src[0] == '-' && src[1] == '-' && src[2] == '>')
      return src + 3;
    src++;
  }
}

/**
 *
 */
static char *
decode_label_reference(xmlparser_t *xp, 
		       struct cdata_content_queue *ccq, char *src)
{
  char *s = src;
  int l;
  char *label;

  while(*src != 0 && *src != ';')
    src++;
  if(*src == 0) {
    xmlerr(xp, "Unexpected end of file during parsing of label reference");
    return NULL;
  }

  l = src - s;
  if(l < 1 || l > 65535)
    return NULL;
  label = alloca(l + 1);
  memcpy(label, s, l);
  label[l] = 0;
  src++;

  if(!strcmp(label, "amp"))
    add_unicode(ccq, '&');
  else if(!strcmp(label, "gt"))
    add_unicode(ccq, '>');
  else if(!strcmp(label, "lt"))
    add_unicode(ccq, '<');
  else if(!strcmp(label, "apos"))
    add_unicode(ccq, '\'');
  else if(!strcmp(label, "quot"))
    add_unicode(ccq, '"');
  else {
    xmlerr(xp, "Unknown label referense: \"&%s;\"\n", label);
    return NULL;
  }

  return src;
}

/**
 *
 */
static char *
htsmsg_xml_parse_cd0(xmlparser_t *xp, 
		     struct cdata_content_queue *ccq, htsmsg_t *tags,
		     htsmsg_t *pis, char *src, int raw)
{
  cdata_content_t *cc = NULL;
  int c;

  while(src != NULL && *src != 0) {

    if(raw && src[0] == ']' && src[1] == ']' && src[2] == '>') {
      if(cc != NULL)
	cc->cc_end = src;
      cc = NULL;
      src += 3;
      break;
    }

    if(*src == '<' && !raw) {

      if(cc != NULL)
	cc->cc_end = src;
      cc = NULL;

      src++;
      if(*src == '?') {
	src++;
	src = htsmsg_xml_parse_pi(xp, pis, src);
	continue;
      }

      if(src[0] == '!') {

	src++;

	if(src[0] == '-' && src[1] == '-') {
	  src = xml_parse_comment(xp, src + 2);
	  continue;
	}

	if(!strncmp(src, "[CDATA[", 7)) {
	  src += 7;
	  src = htsmsg_xml_parse_cd0(xp, ccq, tags, pis, src, 1);
	  continue;
	}
	xmlerr(xp, "Unknown syntatic element: <!%.10s", src);
	return NULL;
      }

      if(*src == '/') {
	/* End-tag */
	src++;
	while(*src != '>') {
	  if(*src == 0) { /* EOF inside endtag */
	    xmlerr(xp, "Unexpected end of file inside close tag");
	    return NULL;
	  }
	  src++;
	}
	src++;
	break;
      }

      src = htsmsg_xml_parse_tag(xp, tags, src);
      continue;
    }
    
    if(*src == '&' && !raw) {
      if(cc != NULL)
	cc->cc_end = src;
      cc = NULL;

      src++;

      if(*src == '#') {
	src++;
	/* Character reference */
	if((c = decode_character_reference(&src)) != 0)
	  add_unicode(ccq, c);
	else {
	  xmlerr(xp, "Invalid character reference");
	  return NULL;
	}
      } else {
	/* Label references */
	src = decode_label_reference(xp, ccq, src);
      }
      continue;
    }

    if(cc == NULL) {
      if(*src <= 32) {
	src++;
	continue;
      }
      cc = malloc(sizeof(cdata_content_t));
      cc->cc_encoding = xp->xp_encoding;
      TAILQ_INSERT_TAIL(ccq, cc, cc_link);
      cc->cc_start = src;
    }
    src++;
  }

  if(cc != NULL) {
    assert(src != NULL);
    cc->cc_end = src;
  }
  return src;
}

/**
 *
 */
static char *
htsmsg_xml_parse_cd(xmlparser_t *xp, htsmsg_t *parent, char *src)
{
  struct cdata_content_queue ccq;
  htsmsg_field_t *f;
  cdata_content_t *cc;
  int c = 0, l, y = 0;
  char *x, *body;
  htsmsg_t *tags = htsmsg_create_map();
  
  TAILQ_INIT(&ccq);
  src = htsmsg_xml_parse_cd0(xp, &ccq, tags, NULL, src, 0);

  /* Assemble body */

  TAILQ_FOREACH(cc, &ccq, cc_link) {

    switch(cc->cc_encoding) {
    case XML_ENCODING_UTF8:
      c += cc->cc_end - cc->cc_start;
      y++;
      break;

    case XML_ENCODING_8859_1:
      l = 0;
      for(x = cc->cc_start; x < cc->cc_end; x++)
	l += 1 + (*x >= 0x80);

      c += l;
      y += 1 + (l != cc->cc_end - cc->cc_start);
      break;
    }
  }

  if(y == 1 && c > 1) {
    /* One segment UTF-8 (or 7bit ASCII),
       use data directly from source */

    cc = TAILQ_FIRST(&ccq);

    assert(cc != NULL);
    assert(TAILQ_NEXT(cc, cc_link) == NULL);
    
    f = htsmsg_field_add(parent, "cdata", HMF_STR, 0);
    f->hmf_str = cc->cc_start;
    *cc->cc_end = 0;
    free(cc);

  } else if(c > 1) {
    body = malloc(c + 1);
    c = 0;

    while((cc = TAILQ_FIRST(&ccq)) != NULL) {

      switch(cc->cc_encoding) {
      case XML_ENCODING_UTF8:
	l = cc->cc_end - cc->cc_start;
	memcpy(body + c, cc->cc_start, l);
	c += l;
	break;

      case XML_ENCODING_8859_1:
	for(x = cc->cc_start; x < cc->cc_end; x++)
	  c += put_utf8(body + c, *x);
	break;
      }
      
      TAILQ_REMOVE(&ccq, cc, cc_link);
      free(cc);
    }
    body[c] = 0;

    f = htsmsg_field_add(parent, "cdata", HMF_STR, HMF_ALLOCED);
    f->hmf_str = body;

  } else {

    while((cc = TAILQ_FIRST(&ccq)) != NULL) {
      TAILQ_REMOVE(&ccq, cc, cc_link);
      free(cc);
    }
  }


  if(src == NULL) {
    htsmsg_destroy(tags);
    return NULL;
  }

  if(TAILQ_FIRST(&tags->hm_fields) != NULL) {
    htsmsg_add_msg_extname(parent, "tags", tags);
  } else {
    htsmsg_destroy(tags);
  }

  return src;
}


/**
 *
 */
static char *
htsmsg_parse_prolog(xmlparser_t *xp, char *src)
{
  htsmsg_t *pis = htsmsg_create_map();
  htsmsg_t *xmlpi;
  const char *encoding;

  while(1) {
    if(*src == 0)
      break;

    while(is_xmlws(*src))
      src++;
    
    if(!strncmp(src, "<?", 2)) {
      src += 2;
      src = htsmsg_xml_parse_pi(xp, pis, src);
      continue;
    }

    if(!strncmp(src, "<!--", 4)) {
      src = xml_parse_comment(xp, src + 4);
      continue;
    }

    if(!strncmp(src, "<!DOCTYPE", 9)) {
      while(*src != 0) {
	if(*src == '>') {
	  src++;
	  break;
	}
	src++;
      }
      continue;
    }
    break;
  }

  if((xmlpi = htsmsg_get_map(pis, "xml")) != NULL) {

    if((encoding = htsmsg_get_str(xmlpi, "encoding")) != NULL) {
      if(!strcasecmp(encoding, "iso-8859-1") ||
	 !strcasecmp(encoding, "iso-8859_1") ||
	 !strcasecmp(encoding, "iso_8859-1") ||
	 !strcasecmp(encoding, "iso_8859_1")) {
	xp->xp_encoding = XML_ENCODING_8859_1;
      }
    }
  }

  htsmsg_destroy(pis);

  return src;
}



/**
 *
 */
htsmsg_t *
htsmsg_xml_deserialize(char *src, char *errbuf, size_t errbufsize)
{
  htsmsg_t *m;
  xmlparser_t xp;
  char *src0 = src;
  int i;

  xp.xp_errmsg[0] = 0;
  xp.xp_encoding = XML_ENCODING_UTF8;
  LIST_INIT(&xp.xp_namespaces);

  if((src = htsmsg_parse_prolog(&xp, src)) == NULL)
    goto err;

  m = htsmsg_create_map();

  if(htsmsg_xml_parse_cd(&xp, m, src) == NULL) {
    htsmsg_destroy(m);
    goto err;
  }

  if(xp.xp_srcdataused) {
    m->hm_data = src0;
  } else {
    free(src0);
  }

  return m;

 err:
  free(src0);
  snprintf(errbuf, errbufsize, "%s", xp.xp_errmsg);
  
  /* Remove any odd chars inside of errmsg */
  for(i = 0; i < errbufsize; i++) {
    if(errbuf[i] < 32) {
      errbuf[i] = 0;
      break;
    }
  }

  return NULL;
}

/**
 *
 */
int
put_utf8(char *out, int c)
{
  if(c == 0xfffe || c == 0xffff || (c >= 0xD800 && c < 0xE000))
    return 0;

  if (c < 0x80) {
    *out = c;
    return 1;
  }

  if(c < 0x800) {
    *out++ = 0xc0 | (0x1f & (c >>  6));
    *out   = 0x80 | (0x3f &  c);
    return 2;
  }

  if(c < 0x10000) {
    *out++ = 0xe0 | (0x0f & (c >> 12));
    *out++ = 0x80 | (0x3f & (c >> 6));
    *out   = 0x80 | (0x3f &  c);
    return 3;
  }

  if(c < 0x200000) {
    *out++ = 0xf0 | (0x07 & (c >> 18));
    *out++ = 0x80 | (0x3f & (c >> 12));
    *out++ = 0x80 | (0x3f & (c >> 6));
    *out   = 0x80 | (0x3f &  c);
    return 4;
  }

  if(c < 0x4000000) {
    *out++ = 0xf8 | (0x03 & (c >> 24));
    *out++ = 0x80 | (0x3f & (c >> 18));
    *out++ = 0x80 | (0x3f & (c >> 12));
    *out++ = 0x80 | (0x3f & (c >>  6));
    *out++ = 0x80 | (0x3f &  c);
    return 5;
  }

  *out++ = 0xfc | (0x01 & (c >> 30));
  *out++ = 0x80 | (0x3f & (c >> 24));
  *out++ = 0x80 | (0x3f & (c >> 18));
  *out++ = 0x80 | (0x3f & (c >> 12));
  *out++ = 0x80 | (0x3f & (c >>  6));
  *out++ = 0x80 | (0x3f &  c);
  return 6;
}
