/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GScanner: Flexible lexical scanner for general purpose.
 * Copyright (C) 1997, 1998 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

/* 
 * MT safe
 */

#include "config.h"

#include	<errno.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<string.h>
#include	<stdio.h>
#ifdef HAVE_UNISTD_H
#include	<unistd.h>
#endif

#include	"glib.h"
#include	"gprintfint.h"
#include        "galias.h"

#ifdef G_OS_WIN32
#include	<io.h>		/* For _read() */
#endif

/* --- defines --- */
#define	to_lower(c)				( \
	(guchar) (							\
	  ( (((guchar)(c))>='A' && ((guchar)(c))<='Z') * ('a'-'A') ) |	\
	  ( (((guchar)(c))>=192 && ((guchar)(c))<=214) * (224-192) ) |	\
	  ( (((guchar)(c))>=216 && ((guchar)(c))<=222) * (248-216) ) |	\
	  ((guchar)(c))							\
	)								\
)
#define	READ_BUFFER_SIZE	(4000)


/* --- typedefs --- */
typedef	struct	_GScannerKey	GScannerKey;

struct	_GScannerKey
{
  guint		 scope_id;
  gchar		*symbol;
  gpointer	 value;
};


/* --- variables --- */
static const GScannerConfig g_scanner_config_template =
{
  (
   " \t\r\n"
   )			/* cset_skip_characters */,
  (
   G_CSET_a_2_z
   "_"
   G_CSET_A_2_Z
   )			/* cset_identifier_first */,
  (
   G_CSET_a_2_z
   "_"
   G_CSET_A_2_Z
   G_CSET_DIGITS
   G_CSET_LATINS
   G_CSET_LATINC
   )			/* cset_identifier_nth */,
  ( "#\n" )		/* cpair_comment_single */,
  
  FALSE			/* case_sensitive */,
  
  TRUE			/* skip_comment_multi */,
  TRUE			/* skip_comment_single */,
  TRUE			/* scan_comment_multi */,
  TRUE			/* scan_identifier */,
  FALSE			/* scan_identifier_1char */,
  FALSE			/* scan_identifier_NULL */,
  TRUE			/* scan_symbols */,
  FALSE			/* scan_binary */,
  TRUE			/* scan_octal */,
  TRUE			/* scan_float */,
  TRUE			/* scan_hex */,
  FALSE			/* scan_hex_dollar */,
  TRUE			/* scan_string_sq */,
  TRUE			/* scan_string_dq */,
  TRUE			/* numbers_2_int */,
  FALSE			/* int_2_float */,
  FALSE			/* identifier_2_string */,
  TRUE			/* char_2_token */,
  FALSE			/* symbol_2_token */,
  FALSE			/* scope_0_fallback */,
  FALSE			/* store_int64 */,
};


/* --- prototypes --- */
static inline
GScannerKey*	g_scanner_lookup_internal (GScanner	*scanner,
					   guint	 scope_id,
					   const gchar	*symbol);
static gboolean	g_scanner_key_equal	  (gconstpointer v1,
					   gconstpointer v2);
static guint	g_scanner_key_hash	  (gconstpointer v);
static void	g_scanner_get_token_ll	  (GScanner	*scanner,
					   GTokenType	*token_p,
					   GTokenValue	*value_p,
					   guint	*line_p,
					   guint	*position_p);
static void	g_scanner_get_token_i	  (GScanner	*scanner,
					   GTokenType	*token_p,
					   GTokenValue	*value_p,
					   guint	*line_p,
					   guint	*position_p);

static guchar	g_scanner_peek_next_char  (GScanner	*scanner);
static guchar	g_scanner_get_char	  (GScanner	*scanner,
					   guint	*line_p,
					   guint	*position_p);
static void	g_scanner_msg_handler	  (GScanner	*scanner,
					   gchar	*message,
					   gboolean	 is_error);


/* --- functions --- */
static inline gint
g_scanner_char_2_num (guchar	c,
		      guchar	base)
{
  if (c >= '0' && c <= '9')
    c -= '0';
  else if (c >= 'A' && c <= 'Z')
    c -= 'A' - 10;
  else if (c >= 'a' && c <= 'z')
    c -= 'a' - 10;
  else
    return -1;
  
  if (c < base)
    return c;
  
  return -1;
}

GScanner*
g_scanner_new (const GScannerConfig *config_templ)
{
  GScanner *scanner;
  
  if (!config_templ)
    config_templ = &g_scanner_config_template;
  
  scanner = g_new0 (GScanner, 1);
  
  scanner->user_data = NULL;
  scanner->max_parse_errors = 1;
  scanner->parse_errors	= 0;
  scanner->input_name = NULL;
  g_datalist_init (&scanner->qdata);
  
  scanner->config = g_new0 (GScannerConfig, 1);
  
  scanner->config->case_sensitive	 = config_templ->case_sensitive;
  scanner->config->cset_skip_characters	 = config_templ->cset_skip_characters;
  if (!scanner->config->cset_skip_characters)
    scanner->config->cset_skip_characters = "";
  scanner->config->cset_identifier_first = config_templ->cset_identifier_first;
  scanner->config->cset_identifier_nth	 = config_templ->cset_identifier_nth;
  scanner->config->cpair_comment_single	 = config_templ->cpair_comment_single;
  scanner->config->skip_comment_multi	 = config_templ->skip_comment_multi;
  scanner->config->skip_comment_single	 = config_templ->skip_comment_single;
  scanner->config->scan_comment_multi	 = config_templ->scan_comment_multi;
  scanner->config->scan_identifier	 = config_templ->scan_identifier;
  scanner->config->scan_identifier_1char = config_templ->scan_identifier_1char;
  scanner->config->scan_identifier_NULL	 = config_templ->scan_identifier_NULL;
  scanner->config->scan_symbols		 = config_templ->scan_symbols;
  scanner->config->scan_binary		 = config_templ->scan_binary;
  scanner->config->scan_octal		 = config_templ->scan_octal;
  scanner->config->scan_float		 = config_templ->scan_float;
  scanner->config->scan_hex		 = config_templ->scan_hex;
  scanner->config->scan_hex_dollar	 = config_templ->scan_hex_dollar;
  scanner->config->scan_string_sq	 = config_templ->scan_string_sq;
  scanner->config->scan_string_dq	 = config_templ->scan_string_dq;
  scanner->config->numbers_2_int	 = config_templ->numbers_2_int;
  scanner->config->int_2_float		 = config_templ->int_2_float;
  scanner->config->identifier_2_string	 = config_templ->identifier_2_string;
  scanner->config->char_2_token		 = config_templ->char_2_token;
  scanner->config->symbol_2_token	 = config_templ->symbol_2_token;
  scanner->config->scope_0_fallback	 = config_templ->scope_0_fallback;
  scanner->config->store_int64		 = config_templ->store_int64;
  
  scanner->token = G_TOKEN_NONE;
  scanner->value.v_int64 = 0;
  scanner->line = 1;
  scanner->position = 0;
  
  scanner->next_token = G_TOKEN_NONE;
  scanner->next_value.v_int64 = 0;
  scanner->next_line = 1;
  scanner->next_position = 0;
  
  scanner->symbol_table = g_hash_table_new (g_scanner_key_hash, g_scanner_key_equal);
  scanner->input_fd = -1;
  scanner->text = NULL;
  scanner->text_end = NULL;
  scanner->buffer = NULL;
  scanner->scope_id = 0;
  
  scanner->msg_handler = g_scanner_msg_handler;
  
  return scanner;
}

static inline void
g_scanner_free_value (GTokenType     *token_p,
		      GTokenValue     *value_p)
{
  switch (*token_p)
    {
    case G_TOKEN_STRING:
    case G_TOKEN_IDENTIFIER:
    case G_TOKEN_IDENTIFIER_NULL:
    case G_TOKEN_COMMENT_SINGLE:
    case G_TOKEN_COMMENT_MULTI:
      g_free (value_p->v_string);
      break;
      
    default:
      break;
    }
  
  *token_p = G_TOKEN_NONE;
}

static void
g_scanner_destroy_symbol_table_entry (gpointer _key,
				      gpointer _value,
				      gpointer _data)
{
  GScannerKey *key = _key;
  
  g_free (key->symbol);
  g_free (key);
}

void
g_scanner_destroy (GScanner	*scanner)
{
  g_return_if_fail (scanner != NULL);
  
  g_datalist_clear (&scanner->qdata);
  g_hash_table_foreach (scanner->symbol_table, 
			g_scanner_destroy_symbol_table_entry, NULL);
  g_hash_table_destroy (scanner->symbol_table);
  g_scanner_free_value (&scanner->token, &scanner->value);
  g_scanner_free_value (&scanner->next_token, &scanner->next_value);
  g_free (scanner->config);
  g_free (scanner->buffer);
  g_free (scanner);
}

static void
g_scanner_msg_handler (GScanner		*scanner,
		       gchar		*message,
		       gboolean		is_error)
{
  g_return_if_fail (scanner != NULL);
  
  _g_fprintf (stderr, "%s:%d: ",
	      scanner->input_name ? scanner->input_name : "<memory>",
	      scanner->line);
  if (is_error)
    _g_fprintf (stderr, "error: ");
  _g_fprintf (stderr, "%s\n", message);
}

void
g_scanner_error (GScanner	*scanner,
		 const gchar	*format,
		 ...)
{
  g_return_if_fail (scanner != NULL);
  g_return_if_fail (format != NULL);
  
  scanner->parse_errors++;
  
  if (scanner->msg_handler)
    {
      va_list args;
      gchar *string;
      
      va_start (args, format);
      string = g_strdup_vprintf (format, args);
      va_end (args);
      
      scanner->msg_handler (scanner, string, TRUE);
      
      g_free (string);
    }
}

void
g_scanner_warn (GScanner       *scanner,
		const gchar    *format,
		...)
{
  g_return_if_fail (scanner != NULL);
  g_return_if_fail (format != NULL);
  
  if (scanner->msg_handler)
    {
      va_list args;
      gchar *string;
      
      va_start (args, format);
      string = g_strdup_vprintf (format, args);
      va_end (args);
      
      scanner->msg_handler (scanner, string, FALSE);
      
      g_free (string);
    }
}

static gboolean
g_scanner_key_equal (gconstpointer v1,
		     gconstpointer v2)
{
  const GScannerKey *key1 = v1;
  const GScannerKey *key2 = v2;
  
  return (key1->scope_id == key2->scope_id) && (strcmp (key1->symbol, key2->symbol) == 0);
}

static guint
g_scanner_key_hash (gconstpointer v)
{
  const GScannerKey *key = v;
  gchar *c;
  guint h;
  
  h = key->scope_id;
  for (c = key->symbol; *c; c++)
    h = (h << 5) - h + *c;
  
  return h;
}

static inline GScannerKey*
g_scanner_lookup_internal (GScanner	*scanner,
			   guint	 scope_id,
			   const gchar	*symbol)
{
  GScannerKey	*key_p;
  GScannerKey key;
  
  key.scope_id = scope_id;
  
  if (!scanner->config->case_sensitive)
    {
      gchar *d;
      const gchar *c;
      
      key.symbol = g_new (gchar, strlen (symbol) + 1);
      for (d = key.symbol, c = symbol; *c; c++, d++)
	*d = to_lower (*c);
      *d = 0;
      key_p = g_hash_table_lookup (scanner->symbol_table, &key);
      g_free (key.symbol);
    }
  else
    {
      key.symbol = (gchar*) symbol;
      key_p = g_hash_table_lookup (scanner->symbol_table, &key);
    }
  
  return key_p;
}

void
g_scanner_scope_add_symbol (GScanner	*scanner,
			    guint	 scope_id,
			    const gchar	*symbol,
			    gpointer	 value)
{
  GScannerKey	*key;
  
  g_return_if_fail (scanner != NULL);
  g_return_if_fail (symbol != NULL);
  
  key = g_scanner_lookup_internal (scanner, scope_id, symbol);
  
  if (!key)
    {
      key = g_new (GScannerKey, 1);
      key->scope_id = scope_id;
      key->symbol = g_strdup (symbol);
      key->value = value;
      if (!scanner->config->case_sensitive)
	{
	  gchar *c;
	  
	  c = key->symbol;
	  while (*c != 0)
	    {
	      *c = to_lower (*c);
	      c++;
	    }
	}
      g_hash_table_insert (scanner->symbol_table, key, key);
    }
  else
    key->value = value;
}

void
g_scanner_scope_remove_symbol (GScanner	   *scanner,
			       guint	    scope_id,
			       const gchar *symbol)
{
  GScannerKey	*key;
  
  g_return_if_fail (scanner != NULL);
  g_return_if_fail (symbol != NULL);
  
  key = g_scanner_lookup_internal (scanner, scope_id, symbol);
  
  if (key)
    {
      g_hash_table_remove (scanner->symbol_table, key);
      g_free (key->symbol);
      g_free (key);
    }
}

gpointer
g_scanner_lookup_symbol (GScanner	*scanner,
			 const gchar	*symbol)
{
  GScannerKey	*key;
  guint scope_id;
  
  g_return_val_if_fail (scanner != NULL, NULL);
  
  if (!symbol)
    return NULL;
  
  scope_id = scanner->scope_id;
  key = g_scanner_lookup_internal (scanner, scope_id, symbol);
  if (!key && scope_id && scanner->config->scope_0_fallback)
    key = g_scanner_lookup_internal (scanner, 0, symbol);
  
  if (key)
    return key->value;
  else
    return NULL;
}

gpointer
g_scanner_scope_lookup_symbol (GScanner	      *scanner,
			       guint	       scope_id,
			       const gchar    *symbol)
{
  GScannerKey	*key;
  
  g_return_val_if_fail (scanner != NULL, NULL);
  
  if (!symbol)
    return NULL;
  
  key = g_scanner_lookup_internal (scanner, scope_id, symbol);
  
  if (key)
    return key->value;
  else
    return NULL;
}

guint
g_scanner_set_scope (GScanner	    *scanner,
		     guint	     scope_id)
{
  guint old_scope_id;
  
  g_return_val_if_fail (scanner != NULL, 0);
  
  old_scope_id = scanner->scope_id;
  scanner->scope_id = scope_id;
  
  return old_scope_id;
}

static void
g_scanner_foreach_internal (gpointer  _key,
			    gpointer  _value,
			    gpointer  _user_data)
{
  GScannerKey *key;
  gpointer *d;
  GHFunc func;
  gpointer user_data;
  guint *scope_id;
  
  d = _user_data;
  func = (GHFunc) d[0];
  user_data = d[1];
  scope_id = d[2];
  key = _value;
  
  if (key->scope_id == *scope_id)
    func (key->symbol, key->value, user_data);
}

void
g_scanner_scope_foreach_symbol (GScanner       *scanner,
				guint		scope_id,
				GHFunc		func,
				gpointer	user_data)
{
  gpointer d[3];
  
  g_return_if_fail (scanner != NULL);
  
  d[0] = (gpointer) func;
  d[1] = user_data;
  d[2] = &scope_id;
  
  g_hash_table_foreach (scanner->symbol_table, g_scanner_foreach_internal, d);
}

GTokenType
g_scanner_peek_next_token (GScanner	*scanner)
{
  g_return_val_if_fail (scanner != NULL, G_TOKEN_EOF);
  
  if (scanner->next_token == G_TOKEN_NONE)
    {
      scanner->next_line = scanner->line;
      scanner->next_position = scanner->position;
      g_scanner_get_token_i (scanner,
			     &scanner->next_token,
			     &scanner->next_value,
			     &scanner->next_line,
			     &scanner->next_position);
    }
  
  return scanner->next_token;
}

GTokenType
g_scanner_get_next_token (GScanner	*scanner)
{
  g_return_val_if_fail (scanner != NULL, G_TOKEN_EOF);
  
  if (scanner->next_token != G_TOKEN_NONE)
    {
      g_scanner_free_value (&scanner->token, &scanner->value);
      
      scanner->token = scanner->next_token;
      scanner->value = scanner->next_value;
      scanner->line = scanner->next_line;
      scanner->position = scanner->next_position;
      scanner->next_token = G_TOKEN_NONE;
    }
  else
    g_scanner_get_token_i (scanner,
			   &scanner->token,
			   &scanner->value,
			   &scanner->line,
			   &scanner->position);
  
  return scanner->token;
}

GTokenType
g_scanner_cur_token (GScanner *scanner)
{
  g_return_val_if_fail (scanner != NULL, G_TOKEN_EOF);
  
  return scanner->token;
}

GTokenValue
g_scanner_cur_value (GScanner *scanner)
{
  GTokenValue v;
  
  v.v_int64 = 0;
  
  g_return_val_if_fail (scanner != NULL, v);

  /* MSC isn't capable of handling return scanner->value; ? */

  v = scanner->value;

  return v;
}

guint
g_scanner_cur_line (GScanner *scanner)
{
  g_return_val_if_fail (scanner != NULL, 0);
  
  return scanner->line;
}

guint
g_scanner_cur_position (GScanner *scanner)
{
  g_return_val_if_fail (scanner != NULL, 0);
  
  return scanner->position;
}

gboolean
g_scanner_eof (GScanner	*scanner)
{
  g_return_val_if_fail (scanner != NULL, TRUE);
  
  return scanner->token == G_TOKEN_EOF || scanner->token == G_TOKEN_ERROR;
}

void
g_scanner_input_file (GScanner *scanner,
		      gint	input_fd)
{
  g_return_if_fail (scanner != NULL);
  g_return_if_fail (input_fd >= 0);

  if (scanner->input_fd >= 0)
    g_scanner_sync_file_offset (scanner);

  scanner->token = G_TOKEN_NONE;
  scanner->value.v_int64 = 0;
  scanner->line = 1;
  scanner->position = 0;
  scanner->next_token = G_TOKEN_NONE;

  scanner->input_fd = input_fd;
  scanner->text = NULL;
  scanner->text_end = NULL;

  if (!scanner->buffer)
    scanner->buffer = g_new (gchar, READ_BUFFER_SIZE + 1);
}

void
g_scanner_input_text (GScanner	  *scanner,
		      const gchar *text,
		      guint	   text_len)
{
  g_return_if_fail (scanner != NULL);
  if (text_len)
    g_return_if_fail (text != NULL);
  else
    text = NULL;

  if (scanner->input_fd >= 0)
    g_scanner_sync_file_offset (scanner);

  scanner->token = G_TOKEN_NONE;
  scanner->value.v_int64 = 0;
  scanner->line = 1;
  scanner->position = 0;
  scanner->next_token = G_TOKEN_NONE;

  scanner->input_fd = -1;
  scanner->text = text;
  scanner->text_end = text + text_len;

  if (scanner->buffer)
    {
      g_free (scanner->buffer);
      scanner->buffer = NULL;
    }
}

static guchar
g_scanner_peek_next_char (GScanner *scanner)
{
  if (scanner->text < scanner->text_end)
    {
      return *scanner->text;
    }
  else if (scanner->input_fd >= 0)
    {
      gint count;
      gchar *buffer;

      buffer = scanner->buffer;
      do
	{
	  count = read (scanner->input_fd, buffer, READ_BUFFER_SIZE);
	}
      while (count == -1 && (errno == EINTR || errno == EAGAIN));

      if (count < 1)
	{
	  scanner->input_fd = -1;

	  return 0;
	}
      else
	{
	  scanner->text = buffer;
	  scanner->text_end = buffer + count;

	  return *buffer;
	}
    }
  else
    return 0;
}

void
g_scanner_sync_file_offset (GScanner *scanner)
{
  g_return_if_fail (scanner != NULL);

  /* for file input, rewind the filedescriptor to the current
   * buffer position and blow the file read ahead buffer. useful
   * for third party uses of our file descriptor, which hooks 
   * onto the current scanning position.
   */

  if (scanner->input_fd >= 0 && scanner->text_end > scanner->text)
    {
      gint buffered;

      buffered = scanner->text_end - scanner->text;
      if (lseek (scanner->input_fd, - buffered, SEEK_CUR) >= 0)
	{
	  /* we succeeded, blow our buffer's contents now */
	  scanner->text = NULL;
	  scanner->text_end = NULL;
	}
      else
	errno = 0;
    }
}

static guchar
g_scanner_get_char (GScanner	*scanner,
		    guint	*line_p,
		    guint	*position_p)
{
  guchar fchar;

  if (scanner->text < scanner->text_end)
    fchar = *(scanner->text++);
  else if (scanner->input_fd >= 0)
    {
      gint count;
      gchar *buffer;

      buffer = scanner->buffer;
      do
	{
	  count = read (scanner->input_fd, buffer, READ_BUFFER_SIZE);
	}
      while (count == -1 && (errno == EINTR || errno == EAGAIN));

      if (count < 1)
	{
	  scanner->input_fd = -1;
	  fchar = 0;
	}
      else
	{
	  scanner->text = buffer + 1;
	  scanner->text_end = buffer + count;
	  fchar = *buffer;
	  if (!fchar)
	    {
	      g_scanner_sync_file_offset (scanner);
	      scanner->text_end = scanner->text;
	      scanner->input_fd = -1;
	    }
	}
    }
  else
    fchar = 0;
  
  if (fchar == '\n')
    {
      (*position_p) = 0;
      (*line_p)++;
    }
  else if (fchar)
    {
      (*position_p)++;
    }
  
  return fchar;
}

void
g_scanner_unexp_token (GScanner		*scanner,
		       GTokenType	 expected_token,
		       const gchar	*identifier_spec,
		       const gchar	*symbol_spec,
		       const gchar	*symbol_name,
		       const gchar	*message,
		       gint		 is_error)
{
  gchar	*token_string;
  guint	token_string_len;
  gchar	*expected_string;
  guint	expected_string_len;
  gchar	*message_prefix;
  gboolean print_unexp;
  void (*msg_handler)	(GScanner*, const gchar*, ...);
  
  g_return_if_fail (scanner != NULL);
  
  if (is_error)
    msg_handler = g_scanner_error;
  else
    msg_handler = g_scanner_warn;
  
  if (!identifier_spec)
    identifier_spec = "identifier";
  if (!symbol_spec)
    symbol_spec = "symbol";
  
  token_string_len = 56;
  token_string = g_new (gchar, token_string_len + 1);
  expected_string_len = 64;
  expected_string = g_new (gchar, expected_string_len + 1);
  print_unexp = TRUE;
  
  switch (scanner->token)
    {
    case G_TOKEN_EOF:
      _g_snprintf (token_string, token_string_len, "end of file");
      break;
      
    default:
      if (scanner->token >= 1 && scanner->token <= 255)
	{
	  if ((scanner->token >= ' ' && scanner->token <= '~') ||
	      strchr (scanner->config->cset_identifier_first, scanner->token) ||
	      strchr (scanner->config->cset_identifier_nth, scanner->token))
	    _g_snprintf (token_string, token_string_len, "character `%c'", scanner->token);
	  else
	    _g_snprintf (token_string, token_string_len, "character `\\%o'", scanner->token);
	  break;
	}
      else if (!scanner->config->symbol_2_token)
	{
	  _g_snprintf (token_string, token_string_len, "(unknown) token <%d>", scanner->token);
	  break;
	}
      /* fall through */
    case G_TOKEN_SYMBOL:
      if (expected_token == G_TOKEN_SYMBOL ||
	  (scanner->config->symbol_2_token &&
	   expected_token > G_TOKEN_LAST))
	print_unexp = FALSE;
      if (symbol_name)
	_g_snprintf (token_string,
		     token_string_len,
		     "%s%s `%s'",
		     print_unexp ? "" : "invalid ",
		     symbol_spec,
		     symbol_name);
      else
	_g_snprintf (token_string,
		     token_string_len,
		     "%s%s",
		     print_unexp ? "" : "invalid ",
		     symbol_spec);
      break;
      
    case G_TOKEN_ERROR:
      print_unexp = FALSE;
      expected_token = G_TOKEN_NONE;
      switch (scanner->value.v_error)
	{
	case G_ERR_UNEXP_EOF:
	  _g_snprintf (token_string, token_string_len, "scanner: unexpected end of file");
	  break;
	  
	case G_ERR_UNEXP_EOF_IN_STRING:
	  _g_snprintf (token_string, token_string_len, "scanner: unterminated string constant");
	  break;
	  
	case G_ERR_UNEXP_EOF_IN_COMMENT:
	  _g_snprintf (token_string, token_string_len, "scanner: unterminated comment");
	  break;
	  
	case G_ERR_NON_DIGIT_IN_CONST:
	  _g_snprintf (token_string, token_string_len, "scanner: non digit in constant");
	  break;
	  
	case G_ERR_FLOAT_RADIX:
	  _g_snprintf (token_string, token_string_len, "scanner: invalid radix for floating constant");
	  break;
	  
	case G_ERR_FLOAT_MALFORMED:
	  _g_snprintf (token_string, token_string_len, "scanner: malformed floating constant");
	  break;
	  
	case G_ERR_DIGIT_RADIX:
	  _g_snprintf (token_string, token_string_len, "scanner: digit is beyond radix");
	  break;
	  
	case G_ERR_UNKNOWN:
	default:
	  _g_snprintf (token_string, token_string_len, "scanner: unknown error");
	  break;
	}
      break;
      
    case G_TOKEN_CHAR:
      _g_snprintf (token_string, token_string_len, "character `%c'", scanner->value.v_char);
      break;
      
    case G_TOKEN_IDENTIFIER:
    case G_TOKEN_IDENTIFIER_NULL:
      if (expected_token == G_TOKEN_IDENTIFIER ||
	  expected_token == G_TOKEN_IDENTIFIER_NULL)
	print_unexp = FALSE;
      _g_snprintf (token_string,
		  token_string_len,
		  "%s%s `%s'",
		  print_unexp ? "" : "invalid ",
		  identifier_spec,
		  scanner->token == G_TOKEN_IDENTIFIER ? scanner->value.v_string : "null");
      break;
      
    case G_TOKEN_BINARY:
    case G_TOKEN_OCTAL:
    case G_TOKEN_INT:
    case G_TOKEN_HEX:
      if (scanner->config->store_int64)
	_g_snprintf (token_string, token_string_len, "number `%" G_GUINT64_FORMAT "'", scanner->value.v_int64);
      else
	_g_snprintf (token_string, token_string_len, "number `%lu'", scanner->value.v_int);
      break;
      
    case G_TOKEN_FLOAT:
      _g_snprintf (token_string, token_string_len, "number `%.3f'", scanner->value.v_float);
      break;
      
    case G_TOKEN_STRING:
      if (expected_token == G_TOKEN_STRING)
	print_unexp = FALSE;
      _g_snprintf (token_string,
		   token_string_len,
		   "%s%sstring constant \"%s\"",
		   print_unexp ? "" : "invalid ",
		   scanner->value.v_string[0] == 0 ? "empty " : "",
		   scanner->value.v_string);
      token_string[token_string_len - 2] = '"';
      token_string[token_string_len - 1] = 0;
      break;
      
    case G_TOKEN_COMMENT_SINGLE:
    case G_TOKEN_COMMENT_MULTI:
      _g_snprintf (token_string, token_string_len, "comment");
      break;
      
    case G_TOKEN_NONE:
      /* somehow the user's parsing code is screwed, there isn't much
       * we can do about it.
       * Note, a common case to trigger this is
       * g_scanner_peek_next_token(); g_scanner_unexp_token();
       * without an intermediate g_scanner_get_next_token().
       */
      g_assert_not_reached ();
      break;
    }
  
  
  switch (expected_token)
    {
      gboolean need_valid;
      gchar *tstring;
    case G_TOKEN_EOF:
      _g_snprintf (expected_string, expected_string_len, "end of file");
      break;
    default:
      if (expected_token >= 1 && expected_token <= 255)
	{
	  if ((expected_token >= ' ' && expected_token <= '~') ||
	      strchr (scanner->config->cset_identifier_first, expected_token) ||
	      strchr (scanner->config->cset_identifier_nth, expected_token))
	    _g_snprintf (expected_string, expected_string_len, "character `%c'", expected_token);
	  else
	    _g_snprintf (expected_string, expected_string_len, "character `\\%o'", expected_token);
	  break;
	}
      else if (!scanner->config->symbol_2_token)
	{
	  _g_snprintf (expected_string, expected_string_len, "(unknown) token <%d>", expected_token);
	  break;
	}
      /* fall through */
    case G_TOKEN_SYMBOL:
      need_valid = (scanner->token == G_TOKEN_SYMBOL ||
		    (scanner->config->symbol_2_token &&
		     scanner->token > G_TOKEN_LAST));
      _g_snprintf (expected_string,
		   expected_string_len,
		   "%s%s",
		   need_valid ? "valid " : "",
		   symbol_spec);
      /* FIXME: should we attempt to lookup the symbol_name for symbol_2_token? */
      break;
    case G_TOKEN_CHAR:
      _g_snprintf (expected_string, expected_string_len, "%scharacter",
		   scanner->token == G_TOKEN_CHAR ? "valid " : "");
      break;
    case G_TOKEN_BINARY:
      tstring = "binary";
      _g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_OCTAL:
      tstring = "octal";
      _g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_INT:
      tstring = "integer";
      _g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_HEX:
      tstring = "hexadecimal";
      _g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_FLOAT:
      tstring = "float";
      _g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_STRING:
      _g_snprintf (expected_string,
		   expected_string_len,
		   "%sstring constant",
		   scanner->token == G_TOKEN_STRING ? "valid " : "");
      break;
    case G_TOKEN_IDENTIFIER:
    case G_TOKEN_IDENTIFIER_NULL:
      need_valid = (scanner->token == G_TOKEN_IDENTIFIER_NULL ||
		    scanner->token == G_TOKEN_IDENTIFIER);
      _g_snprintf (expected_string,
		   expected_string_len,
		   "%s%s",
		   need_valid ? "valid " : "",
		   identifier_spec);
      break;
    case G_TOKEN_COMMENT_SINGLE:
      tstring = "single-line";
      _g_snprintf (expected_string, expected_string_len, "%scomment (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_COMMENT_MULTI:
      tstring = "multi-line";
      _g_snprintf (expected_string, expected_string_len, "%scomment (%s)",
		   scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_NONE:
    case G_TOKEN_ERROR:
      /* this is handled upon printout */
      break;
    }
  
  if (message && message[0] != 0)
    message_prefix = " - ";
  else
    {
      message_prefix = "";
      message = "";
    }
  if (expected_token == G_TOKEN_ERROR)
    {
      msg_handler (scanner,
		   "failure around %s%s%s",
		   token_string,
		   message_prefix,
		   message);
    }
  else if (expected_token == G_TOKEN_NONE)
    {
      if (print_unexp)
	msg_handler (scanner,
		     "unexpected %s%s%s",
		     token_string,
		     message_prefix,
		     message);
      else
	msg_handler (scanner,
		     "%s%s%s",
		     token_string,
		     message_prefix,
		     message);
    }
  else
    {
      if (print_unexp)
	msg_handler (scanner,
		     "unexpected %s, expected %s%s%s",
		     token_string,
		     expected_string,
		     message_prefix,
		     message);
      else
	msg_handler (scanner,
		     "%s, expected %s%s%s",
		     token_string,
		     expected_string,
		     message_prefix,
		     message);
    }
  
  g_free (token_string);
  g_free (expected_string);
}

static void
g_scanner_get_token_i (GScanner	*scanner,
		       GTokenType	*token_p,
		       GTokenValue	*value_p,
		       guint		*line_p,
		       guint		*position_p)
{
  do
    {
      g_scanner_free_value (token_p, value_p);
      g_scanner_get_token_ll (scanner, token_p, value_p, line_p, position_p);
    }
  while (((*token_p > 0 && *token_p < 256) &&
	  strchr (scanner->config->cset_skip_characters, *token_p)) ||
	 (*token_p == G_TOKEN_CHAR &&
	  strchr (scanner->config->cset_skip_characters, value_p->v_char)) ||
	 (*token_p == G_TOKEN_COMMENT_MULTI &&
	  scanner->config->skip_comment_multi) ||
	 (*token_p == G_TOKEN_COMMENT_SINGLE &&
	  scanner->config->skip_comment_single));
  
  switch (*token_p)
    {
    case G_TOKEN_IDENTIFIER:
      if (scanner->config->identifier_2_string)
	*token_p = G_TOKEN_STRING;
      break;
      
    case G_TOKEN_SYMBOL:
      if (scanner->config->symbol_2_token)
	*token_p = (GTokenType) value_p->v_symbol;
      break;
      
    case G_TOKEN_BINARY:
    case G_TOKEN_OCTAL:
    case G_TOKEN_HEX:
      if (scanner->config->numbers_2_int)
	*token_p = G_TOKEN_INT;
      break;
      
    default:
      break;
    }
  
  if (*token_p == G_TOKEN_INT &&
      scanner->config->int_2_float)
    {
      *token_p = G_TOKEN_FLOAT;
      if (scanner->config->store_int64)
        {
#ifdef _MSC_VER
          /* work around error C2520, see gvaluetransform.c */
          value_p->v_float = (__int64)value_p->v_int64;
#else
          value_p->v_float = value_p->v_int64;
#endif
        }
      else
	value_p->v_float = value_p->v_int;
    }
  
  errno = 0;
}

static void
g_scanner_get_token_ll	(GScanner	*scanner,
			 GTokenType	*token_p,
			 GTokenValue	*value_p,
			 guint		*line_p,
			 guint		*position_p)
{
  GScannerConfig *config;
  GTokenType	   token;
  gboolean	   in_comment_multi;
  gboolean	   in_comment_single;
  gboolean	   in_string_sq;
  gboolean	   in_string_dq;
  GString	  *gstring;
  GTokenValue	   value;
  guchar	   ch;
  
  config = scanner->config;
  (*value_p).v_int64 = 0;
  
  if ((scanner->text >= scanner->text_end && scanner->input_fd < 0) ||
      scanner->token == G_TOKEN_EOF)
    {
      *token_p = G_TOKEN_EOF;
      return;
    }
  
  in_comment_multi = FALSE;
  in_comment_single = FALSE;
  in_string_sq = FALSE;
  in_string_dq = FALSE;
  gstring = NULL;
  
  do /* while (ch != 0) */
    {
      gboolean dotted_float = FALSE;
      
      ch = g_scanner_get_char (scanner, line_p, position_p);
      
      value.v_int64 = 0;
      token = G_TOKEN_NONE;
      
      /* this is *evil*, but needed ;(
       * we first check for identifier first character, because	 it
       * might interfere with other key chars like slashes or numbers
       */
      if (config->scan_identifier &&
	  ch && strchr (config->cset_identifier_first, ch))
	goto identifier_precedence;
      
      switch (ch)
	{
	case 0:
	  token = G_TOKEN_EOF;
	  (*position_p)++;
	  /* ch = 0; */
	  break;
	  
	case '/':
	  if (!config->scan_comment_multi ||
	      g_scanner_peek_next_char (scanner) != '*')
	    goto default_case;
	  g_scanner_get_char (scanner, line_p, position_p);
	  token = G_TOKEN_COMMENT_MULTI;
	  in_comment_multi = TRUE;
	  gstring = g_string_new (NULL);
	  while ((ch = g_scanner_get_char (scanner, line_p, position_p)) != 0)
	    {
	      if (ch == '*' && g_scanner_peek_next_char (scanner) == '/')
		{
		  g_scanner_get_char (scanner, line_p, position_p);
		  in_comment_multi = FALSE;
		  break;
		}
	      else
		gstring = g_string_append_c (gstring, ch);
	    }
	  ch = 0;
	  break;
	  
	case '\'':
	  if (!config->scan_string_sq)
	    goto default_case;
	  token = G_TOKEN_STRING;
	  in_string_sq = TRUE;
	  gstring = g_string_new (NULL);
	  while ((ch = g_scanner_get_char (scanner, line_p, position_p)) != 0)
	    {
	      if (ch == '\'')
		{
		  in_string_sq = FALSE;
		  break;
		}
	      else
		gstring = g_string_append_c (gstring, ch);
	    }
	  ch = 0;
	  break;
	  
	case '"':
	  if (!config->scan_string_dq)
	    goto default_case;
	  token = G_TOKEN_STRING;
	  in_string_dq = TRUE;
	  gstring = g_string_new (NULL);
	  while ((ch = g_scanner_get_char (scanner, line_p, position_p)) != 0)
	    {
	      if (ch == '"')
		{
		  in_string_dq = FALSE;
		  break;
		}
	      else
		{
		  if (ch == '\\')
		    {
		      ch = g_scanner_get_char (scanner, line_p, position_p);
		      switch (ch)
			{
			  guint	i;
			  guint	fchar;
			  
			case 0:
			  break;
			  
			case '\\':
			  gstring = g_string_append_c (gstring, '\\');
			  break;
			  
			case 'n':
			  gstring = g_string_append_c (gstring, '\n');
			  break;
			  
			case 't':
			  gstring = g_string_append_c (gstring, '\t');
			  break;
			  
			case 'r':
			  gstring = g_string_append_c (gstring, '\r');
			  break;
			  
			case 'b':
			  gstring = g_string_append_c (gstring, '\b');
			  break;
			  
			case 'f':
			  gstring = g_string_append_c (gstring, '\f');
			  break;
			  
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			  i = ch - '0';
			  fchar = g_scanner_peek_next_char (scanner);
			  if (fchar >= '0' && fchar <= '7')
			    {
			      ch = g_scanner_get_char (scanner, line_p, position_p);
			      i = i * 8 + ch - '0';
			      fchar = g_scanner_peek_next_char (scanner);
			      if (fchar >= '0' && fchar <= '7')
				{
				  ch = g_scanner_get_char (scanner, line_p, position_p);
				  i = i * 8 + ch - '0';
				}
			    }
			  gstring = g_string_append_c (gstring, i);
			  break;
			  
			default:
			  gstring = g_string_append_c (gstring, ch);
			  break;
			}
		    }
		  else
		    gstring = g_string_append_c (gstring, ch);
		}
	    }
	  ch = 0;
	  break;
	  
	case '.':
	  if (!config->scan_float)
	    goto default_case;
	  token = G_TOKEN_FLOAT;
	  dotted_float = TRUE;
	  ch = g_scanner_get_char (scanner, line_p, position_p);
	  goto number_parsing;
	  
	case '$':
	  if (!config->scan_hex_dollar)
	    goto default_case;
	  token = G_TOKEN_HEX;
	  ch = g_scanner_get_char (scanner, line_p, position_p);
	  goto number_parsing;
	  
	case '0':
	  if (config->scan_octal)
	    token = G_TOKEN_OCTAL;
	  else
	    token = G_TOKEN_INT;
	  ch = g_scanner_peek_next_char (scanner);
	  if (config->scan_hex && (ch == 'x' || ch == 'X'))
	    {
	      token = G_TOKEN_HEX;
	      g_scanner_get_char (scanner, line_p, position_p);
	      ch = g_scanner_get_char (scanner, line_p, position_p);
	      if (ch == 0)
		{
		  token = G_TOKEN_ERROR;
		  value.v_error = G_ERR_UNEXP_EOF;
		  (*position_p)++;
		  break;
		}
	      if (g_scanner_char_2_num (ch, 16) < 0)
		{
		  token = G_TOKEN_ERROR;
		  value.v_error = G_ERR_DIGIT_RADIX;
		  ch = 0;
		  break;
		}
	    }
	  else if (config->scan_binary && (ch == 'b' || ch == 'B'))
	    {
	      token = G_TOKEN_BINARY;
	      g_scanner_get_char (scanner, line_p, position_p);
	      ch = g_scanner_get_char (scanner, line_p, position_p);
	      if (ch == 0)
		{
		  token = G_TOKEN_ERROR;
		  value.v_error = G_ERR_UNEXP_EOF;
		  (*position_p)++;
		  break;
		}
	      if (g_scanner_char_2_num (ch, 10) < 0)
		{
		  token = G_TOKEN_ERROR;
		  value.v_error = G_ERR_NON_DIGIT_IN_CONST;
		  ch = 0;
		  break;
		}
	    }
	  else
	    ch = '0';
	  /* fall through */
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	number_parsing:
	{
          gboolean in_number = TRUE;
	  gchar *endptr;
	  
	  if (token == G_TOKEN_NONE)
	    token = G_TOKEN_INT;
	  
	  gstring = g_string_new (dotted_float ? "0." : "");
	  gstring = g_string_append_c (gstring, ch);
	  
	  do /* while (in_number) */
	    {
	      gboolean is_E;
	      
	      is_E = token == G_TOKEN_FLOAT && (ch == 'e' || ch == 'E');
	      
	      ch = g_scanner_peek_next_char (scanner);
	      
	      if (g_scanner_char_2_num (ch, 36) >= 0 ||
		  (config->scan_float && ch == '.') ||
		  (is_E && (ch == '+' || ch == '-')))
		{
		  ch = g_scanner_get_char (scanner, line_p, position_p);
		  
		  switch (ch)
		    {
		    case '.':
		      if (token != G_TOKEN_INT && token != G_TOKEN_OCTAL)
			{
			  value.v_error = token == G_TOKEN_FLOAT ? G_ERR_FLOAT_MALFORMED : G_ERR_FLOAT_RADIX;
			  token = G_TOKEN_ERROR;
			  in_number = FALSE;
			}
		      else
			{
			  token = G_TOKEN_FLOAT;
			  gstring = g_string_append_c (gstring, ch);
			}
		      break;
		      
		    case '0':
		    case '1':
		    case '2':
		    case '3':
		    case '4':
		    case '5':
		    case '6':
		    case '7':
		    case '8':
		    case '9':
		      gstring = g_string_append_c (gstring, ch);
		      break;
		      
		    case '-':
		    case '+':
		      if (token != G_TOKEN_FLOAT)
			{
			  token = G_TOKEN_ERROR;
			  value.v_error = G_ERR_NON_DIGIT_IN_CONST;
			  in_number = FALSE;
			}
		      else
			gstring = g_string_append_c (gstring, ch);
		      break;
		      
		    case 'e':
		    case 'E':
		      if ((token != G_TOKEN_HEX && !config->scan_float) ||
			  (token != G_TOKEN_HEX &&
			   token != G_TOKEN_OCTAL &&
			   token != G_TOKEN_FLOAT &&
			   token != G_TOKEN_INT))
			{
			  token = G_TOKEN_ERROR;
			  value.v_error = G_ERR_NON_DIGIT_IN_CONST;
			  in_number = FALSE;
			}
		      else
			{
			  if (token != G_TOKEN_HEX)
			    token = G_TOKEN_FLOAT;
			  gstring = g_string_append_c (gstring, ch);
			}
		      break;
		      
		    default:
		      if (token != G_TOKEN_HEX)
			{
			  token = G_TOKEN_ERROR;
			  value.v_error = G_ERR_NON_DIGIT_IN_CONST;
			  in_number = FALSE;
			}
		      else
			gstring = g_string_append_c (gstring, ch);
		      break;
		    }
		}
	      else
		in_number = FALSE;
	    }
	  while (in_number);
	  
	  endptr = NULL;
	  if (token == G_TOKEN_FLOAT)
	    value.v_float = g_strtod (gstring->str, &endptr);
	  else
	    {
	      guint64 ui64 = 0;
	      switch (token)
		{
		case G_TOKEN_BINARY:
		  ui64 = g_ascii_strtoull (gstring->str, &endptr, 2);
		  break;
		case G_TOKEN_OCTAL:
		  ui64 = g_ascii_strtoull (gstring->str, &endptr, 8);
		  break;
		case G_TOKEN_INT:
		  ui64 = g_ascii_strtoull (gstring->str, &endptr, 10);
		  break;
		case G_TOKEN_HEX:
		  ui64 = g_ascii_strtoull (gstring->str, &endptr, 16);
		  break;
		default: ;
		}
	      if (scanner->config->store_int64)
		value.v_int64 = ui64;
	      else
		value.v_int = ui64;
	    }
	  if (endptr && *endptr)
	    {
	      token = G_TOKEN_ERROR;
	      if (*endptr == 'e' || *endptr == 'E')
		value.v_error = G_ERR_NON_DIGIT_IN_CONST;
	      else
		value.v_error = G_ERR_DIGIT_RADIX;
	    }
	  g_string_free (gstring, TRUE);
	  gstring = NULL;
	  ch = 0;
	} /* number_parsing:... */
	break;
	
	default:
	default_case:
	{
	  if (config->cpair_comment_single &&
	      ch == config->cpair_comment_single[0])
	    {
	      token = G_TOKEN_COMMENT_SINGLE;
	      in_comment_single = TRUE;
	      gstring = g_string_new (NULL);
	      ch = g_scanner_get_char (scanner, line_p, position_p);
	      while (ch != 0)
		{
		  if (ch == config->cpair_comment_single[1])
		    {
		      in_comment_single = FALSE;
		      ch = 0;
		      break;
		    }
		  
		  gstring = g_string_append_c (gstring, ch);
		  ch = g_scanner_get_char (scanner, line_p, position_p);
		}
	      /* ignore a missing newline at EOF for single line comments */
	      if (in_comment_single &&
		  config->cpair_comment_single[1] == '\n')
		in_comment_single = FALSE;
	    }
	  else if (config->scan_identifier && ch &&
		   strchr (config->cset_identifier_first, ch))
	    {
	    identifier_precedence:
	      
	      if (config->cset_identifier_nth && ch &&
		  strchr (config->cset_identifier_nth,
			  g_scanner_peek_next_char (scanner)))
		{
		  token = G_TOKEN_IDENTIFIER;
		  gstring = g_string_new (NULL);
		  gstring = g_string_append_c (gstring, ch);
		  do
		    {
		      ch = g_scanner_get_char (scanner, line_p, position_p);
		      gstring = g_string_append_c (gstring, ch);
		      ch = g_scanner_peek_next_char (scanner);
		    }
		  while (ch && strchr (config->cset_identifier_nth, ch));
		  ch = 0;
		}
	      else if (config->scan_identifier_1char)
		{
		  token = G_TOKEN_IDENTIFIER;
		  value.v_identifier = g_new0 (gchar, 2);
		  value.v_identifier[0] = ch;
		  ch = 0;
		}
	    }
	  if (ch)
	    {
	      if (config->char_2_token)
		token = ch;
	      else
		{
		  token = G_TOKEN_CHAR;
		  value.v_char = ch;
		}
	      ch = 0;
	    }
	} /* default_case:... */
	break;
	}
      g_assert (ch == 0 && token != G_TOKEN_NONE); /* paranoid */
    }
  while (ch != 0);
  
  if (in_comment_multi || in_comment_single ||
      in_string_sq || in_string_dq)
    {
      token = G_TOKEN_ERROR;
      if (gstring)
	{
	  g_string_free (gstring, TRUE);
	  gstring = NULL;
	}
      (*position_p)++;
      if (in_comment_multi || in_comment_single)
	value.v_error = G_ERR_UNEXP_EOF_IN_COMMENT;
      else /* (in_string_sq || in_string_dq) */
	value.v_error = G_ERR_UNEXP_EOF_IN_STRING;
    }
  
  if (gstring)
    {
      value.v_string = g_string_free (gstring, FALSE);
      gstring = NULL;
    }
  
  if (token == G_TOKEN_IDENTIFIER)
    {
      if (config->scan_symbols)
	{
	  GScannerKey *key;
	  guint scope_id;
	  
	  scope_id = scanner->scope_id;
	  key = g_scanner_lookup_internal (scanner, scope_id, value.v_identifier);
	  if (!key && scope_id && scanner->config->scope_0_fallback)
	    key = g_scanner_lookup_internal (scanner, 0, value.v_identifier);
	  
	  if (key)
	    {
	      g_free (value.v_identifier);
	      token = G_TOKEN_SYMBOL;
	      value.v_symbol = key->value;
	    }
	}
      
      if (token == G_TOKEN_IDENTIFIER &&
	  config->scan_identifier_NULL &&
	  strlen (value.v_identifier) == 4)
	{
	  gchar *null_upper = "NULL";
	  gchar *null_lower = "null";
	  
	  if (scanner->config->case_sensitive)
	    {
	      if (value.v_identifier[0] == null_upper[0] &&
		  value.v_identifier[1] == null_upper[1] &&
		  value.v_identifier[2] == null_upper[2] &&
		  value.v_identifier[3] == null_upper[3])
		token = G_TOKEN_IDENTIFIER_NULL;
	    }
	  else
	    {
	      if ((value.v_identifier[0] == null_upper[0] ||
		   value.v_identifier[0] == null_lower[0]) &&
		  (value.v_identifier[1] == null_upper[1] ||
		   value.v_identifier[1] == null_lower[1]) &&
		  (value.v_identifier[2] == null_upper[2] ||
		   value.v_identifier[2] == null_lower[2]) &&
		  (value.v_identifier[3] == null_upper[3] ||
		   value.v_identifier[3] == null_lower[3]))
		token = G_TOKEN_IDENTIFIER_NULL;
	    }
	}
    }
  
  *token_p = token;
  *value_p = value;
}

#define __G_SCANNER_C__
#include "galiasdef.c"
