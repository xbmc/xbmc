/* gshell.c - Shell-related utilities
 *
 *  Copyright 2000 Red Hat, Inc.
 *  g_execvpe implementation based on GNU libc execvp:
 *   Copyright 1991, 92, 95, 96, 97, 98, 99 Free Software Foundation, Inc.
 *
 * GLib is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * GLib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GLib; see the file COPYING.LIB.  If not, write
 * to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include "glib.h"

#ifdef _
#warning "FIXME remove gettext hack"
#endif

#include "glibintl.h"
#include "galias.h"

GQuark
g_shell_error_quark (void)
{
  return g_quark_from_static_string ("g-shell-error-quark");
}

/* Single quotes preserve the literal string exactly. escape
 * sequences are not allowed; not even \' - if you want a '
 * in the quoted text, you have to do something like 'foo'\''bar'
 *
 * Double quotes allow $ ` " \ and newline to be escaped with backslash.
 * Otherwise double quotes preserve things literally.
 */

static gboolean 
unquote_string_inplace (gchar* str, gchar** end, GError** err)
{
  gchar* dest;
  gchar* s;
  gchar quote_char;
  
  g_return_val_if_fail(end != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  g_return_val_if_fail(str != NULL, FALSE);
  
  dest = s = str;

  quote_char = *s;
  
  if (!(*s == '"' || *s == '\''))
    {
      g_set_error_literal (err,
                           G_SHELL_ERROR,
                           G_SHELL_ERROR_BAD_QUOTING,
                           _("Quoted text doesn't begin with a quotation mark"));
      *end = str;
      return FALSE;
    }

  /* Skip the initial quote mark */
  ++s;

  if (quote_char == '"')
    {
      while (*s)
        {
          g_assert(s > dest); /* loop invariant */
      
          switch (*s)
            {
            case '"':
              /* End of the string, return now */
              *dest = '\0';
              ++s;
              *end = s;
              return TRUE;
              break;

            case '\\':
              /* Possible escaped quote or \ */
              ++s;
              switch (*s)
                {
                case '"':
                case '\\':
                case '`':
                case '$':
                case '\n':
                  *dest = *s;
                  ++s;
                  ++dest;
                  break;

                default:
                  /* not an escaped char */
                  *dest = '\\';
                  ++dest;
                  /* ++s already done. */
                  break;
                }
              break;

            default:
              *dest = *s;
              ++dest;
              ++s;
              break;
            }

          g_assert(s > dest); /* loop invariant */
        }
    }
  else
    {
      while (*s)
        {
          g_assert(s > dest); /* loop invariant */
          
          if (*s == '\'')
            {
              /* End of the string, return now */
              *dest = '\0';
              ++s;
              *end = s;
              return TRUE;
            }
          else
            {
              *dest = *s;
              ++dest;
              ++s;
            }

          g_assert(s > dest); /* loop invariant */
        }
    }
  
  /* If we reach here this means the close quote was never encountered */

  *dest = '\0';
  
  g_set_error_literal (err,
                       G_SHELL_ERROR,
                       G_SHELL_ERROR_BAD_QUOTING,
                       _("Unmatched quotation mark in command line or other shell-quoted text"));
  *end = s;
  return FALSE;
}

/**
 * g_shell_quote:
 * @unquoted_string: a literal string
 * 
 * Quotes a string so that the shell (/bin/sh) will interpret the
 * quoted string to mean @unquoted_string. If you pass a filename to
 * the shell, for example, you should first quote it with this
 * function.  The return value must be freed with g_free(). The
 * quoting style used is undefined (single or double quotes may be
 * used).
 * 
 * Return value: quoted string
 **/
gchar*
g_shell_quote (const gchar *unquoted_string)
{
  /* We always use single quotes, because the algorithm is cheesier.
   * We could use double if we felt like it, that might be more
   * human-readable.
   */

  const gchar *p;
  GString *dest;

  g_return_val_if_fail (unquoted_string != NULL, NULL);
  
  dest = g_string_new ("'");

  p = unquoted_string;

  /* could speed this up a lot by appending chunks of text at a
   * time.
   */
  while (*p)
    {
      /* Replace literal ' with a close ', a \', and a open ' */
      if (*p == '\'')
        g_string_append (dest, "'\\''");
      else
        g_string_append_c (dest, *p);

      ++p;
    }

  /* close the quote */
  g_string_append_c (dest, '\'');
  
  return g_string_free (dest, FALSE);
}

/**
 * g_shell_unquote:
 * @quoted_string: shell-quoted string
 * @error: error return location or NULL
 * 
 * Unquotes a string as the shell (/bin/sh) would. Only handles
 * quotes; if a string contains file globs, arithmetic operators,
 * variables, backticks, redirections, or other special-to-the-shell
 * features, the result will be different from the result a real shell
 * would produce (the variables, backticks, etc. will be passed
 * through literally instead of being expanded). This function is
 * guaranteed to succeed if applied to the result of
 * g_shell_quote(). If it fails, it returns %NULL and sets the
 * error. The @quoted_string need not actually contain quoted or
 * escaped text; g_shell_unquote() simply goes through the string and
 * unquotes/unescapes anything that the shell would. Both single and
 * double quotes are handled, as are escapes including escaped
 * newlines. The return value must be freed with g_free(). Possible
 * errors are in the #G_SHELL_ERROR domain.
 * 
 * Shell quoting rules are a bit strange. Single quotes preserve the
 * literal string exactly. escape sequences are not allowed; not even
 * \' - if you want a ' in the quoted text, you have to do something
 * like 'foo'\''bar'.  Double quotes allow $, `, ", \, and newline to
 * be escaped with backslash. Otherwise double quotes preserve things
 * literally.
 *
 * Return value: an unquoted string
 **/
gchar*
g_shell_unquote (const gchar *quoted_string,
                 GError     **error)
{
  gchar *unquoted;
  gchar *end;
  gchar *start;
  GString *retval;
  
  g_return_val_if_fail (quoted_string != NULL, NULL);
  
  unquoted = g_strdup (quoted_string);

  start = unquoted;
  end = unquoted;
  retval = g_string_new (NULL);

  /* The loop allows cases such as
   * "foo"blah blah'bar'woo foo"baz"la la la\'\''foo'
   */
  while (*start)
    {
      /* Append all non-quoted chars, honoring backslash escape
       */
      
      while (*start && !(*start == '"' || *start == '\''))
        {
          if (*start == '\\')
            {
              /* all characters can get escaped by backslash,
               * except newline, which is removed if it follows
               * a backslash outside of quotes
               */
              
              ++start;
              if (*start)
                {
                  if (*start != '\n')
                    g_string_append_c (retval, *start);
                  ++start;
                }
            }
          else
            {
              g_string_append_c (retval, *start);
              ++start;
            }
        }

      if (*start)
        {
          if (!unquote_string_inplace (start, &end, error))
            {
              goto error;
            }
          else
            {
              g_string_append (retval, start);
              start = end;
            }
        }
    }

  g_free (unquoted);
  return g_string_free (retval, FALSE);
  
 error:
  g_assert (error == NULL || *error != NULL);
  
  g_free (unquoted);
  g_string_free (retval, TRUE);
  return NULL;
}

/* g_parse_argv() does a semi-arbitrary weird subset of the way
 * the shell parses a command line. We don't do variable expansion,
 * don't understand that operators are tokens, don't do tilde expansion,
 * don't do command substitution, no arithmetic expansion, IFS gets ignored,
 * don't do filename globs, don't remove redirection stuff, etc.
 *
 * READ THE UNIX98 SPEC on "Shell Command Language" before changing
 * the behavior of this code.
 *
 * Steps to parsing the argv string:
 *
 *  - tokenize the string (but since we ignore operators,
 *    our tokenization may diverge from what the shell would do)
 *    note that tokenization ignores the internals of a quoted
 *    word and it always splits on spaces, not on IFS even
 *    if we used IFS. We also ignore "end of input indicator"
 *    (I guess this is control-D?)
 *
 *    Tokenization steps, from UNIX98 with operator stuff removed,
 *    are:
 * 
 *    1) "If the current character is backslash, single-quote or
 *        double-quote (\, ' or ") and it is not quoted, it will affect
 *        quoting for subsequent characters up to the end of the quoted
 *        text. The rules for quoting are as described in Quoting
 *        . During token recognition no substitutions will be actually
 *        performed, and the result token will contain exactly the
 *        characters that appear in the input (except for newline
 *        character joining), unmodified, including any embedded or
 *        enclosing quotes or substitution operators, between the quote
 *        mark and the end of the quoted text. The token will not be
 *        delimited by the end of the quoted field."
 *
 *    2) "If the current character is an unquoted newline character,
 *        the current token will be delimited."
 *
 *    3) "If the current character is an unquoted blank character, any
 *        token containing the previous character is delimited and the
 *        current character will be discarded."
 *
 *    4) "If the previous character was part of a word, the current
 *        character will be appended to that word."
 *
 *    5) "If the current character is a "#", it and all subsequent
 *        characters up to, but excluding, the next newline character
 *        will be discarded as a comment. The newline character that
 *        ends the line is not considered part of the comment. The
 *        "#" starts a comment only when it is at the beginning of a
 *        token. Since the search for the end-of-comment does not
 *        consider an escaped newline character specially, a comment
 *        cannot be continued to the next line."
 *
 *    6) "The current character will be used as the start of a new word."
 *
 *
 *  - for each token (word), perform portions of word expansion, namely
 *    field splitting (using default whitespace IFS) and quote
 *    removal.  Field splitting may increase the number of words.
 *    Quote removal does not increase the number of words.
 *
 *   "If the complete expansion appropriate for a word results in an
 *   empty field, that empty field will be deleted from the list of
 *   fields that form the completely expanded command, unless the
 *   original word contained single-quote or double-quote characters."
 *    - UNIX98 spec
 *
 *
 */

static inline void
ensure_token (GString **token)
{
  if (*token == NULL)
    *token = g_string_new (NULL);
}

static void
delimit_token (GString **token,
               GSList **retval)
{
  if (*token == NULL)
    return;

  *retval = g_slist_prepend (*retval, g_string_free (*token, FALSE));

  *token = NULL;
}

static GSList*
tokenize_command_line (const gchar *command_line,
                       GError **error)
{
  gchar current_quote;
  const gchar *p;
  GString *current_token = NULL;
  GSList *retval = NULL;
  gboolean quoted;;

  current_quote = '\0';
  quoted = FALSE;
  p = command_line;
 
  while (*p)
    {
      if (current_quote == '\\')
        {
          if (*p == '\n')
            {
              /* we append nothing; backslash-newline become nothing */
            }
          else
            {
              /* we append the backslash and the current char,
               * to be interpreted later after tokenization
               */
              ensure_token (&current_token);
              g_string_append_c (current_token, '\\');
              g_string_append_c (current_token, *p);
            }

          current_quote = '\0';
        }
      else if (current_quote == '#')
        {
          /* Discard up to and including next newline */
          while (*p && *p != '\n')
            ++p;

          current_quote = '\0';
          
          if (*p == '\0')
            break;
        }
      else if (current_quote)
        {
          if (*p == current_quote &&
              /* check that it isn't an escaped double quote */
              !(current_quote == '"' && quoted))
            {
              /* close the quote */
              current_quote = '\0';
            }

          /* Everything inside quotes, and the close quote,
           * gets appended literally.
           */

          ensure_token (&current_token);
          g_string_append_c (current_token, *p);
        }
      else
        {
          switch (*p)
            {
            case '\n':
              delimit_token (&current_token, &retval);
              break;

            case ' ':
            case '\t':
              /* If the current token contains the previous char, delimit
               * the current token. A nonzero length
               * token should always contain the previous char.
               */
              if (current_token &&
                  current_token->len > 0)
                {
                  delimit_token (&current_token, &retval);
                }
              
              /* discard all unquoted blanks (don't add them to a token) */
              break;


              /* single/double quotes are appended to the token,
               * escapes are maybe appended next time through the loop,
               * comment chars are never appended.
               */
              
            case '\'':
            case '"':
              ensure_token (&current_token);
              g_string_append_c (current_token, *p);

              /* FALL THRU */
              
            case '#':
            case '\\':
              current_quote = *p;
              break;

            default:
              /* Combines rules 4) and 6) - if we have a token, append to it,
               * otherwise create a new token.
               */
              ensure_token (&current_token);
              g_string_append_c (current_token, *p);
              break;
            }
        }

      /* We need to count consecutive backslashes mod 2, 
       * to detect escaped doublequotes.
       */
      if (*p != '\\')
	quoted = FALSE;
      else
	quoted = !quoted;

      ++p;
    }

  delimit_token (&current_token, &retval);

  if (current_quote)
    {
      if (current_quote == '\\')
        g_set_error (error,
                     G_SHELL_ERROR,
                     G_SHELL_ERROR_BAD_QUOTING,
                     _("Text ended just after a '\\' character."
                       " (The text was '%s')"),
                     command_line);
      else
        g_set_error (error,
                     G_SHELL_ERROR,
                     G_SHELL_ERROR_BAD_QUOTING,
                     _("Text ended before matching quote was found for %c."
                       " (The text was '%s')"),
                     current_quote, command_line);
      
      goto error;
    }

  if (retval == NULL)
    {
      g_set_error_literal (error,
                           G_SHELL_ERROR,
                           G_SHELL_ERROR_EMPTY_STRING,
                           _("Text was empty (or contained only whitespace)"));

      goto error;
    }
  
  /* we appended backward */
  retval = g_slist_reverse (retval);

  return retval;

 error:
  g_assert (error == NULL || *error != NULL);
  
  if (retval)
    {
      g_slist_foreach (retval, (GFunc)g_free, NULL);
      g_slist_free (retval);
    }

  return NULL;
}

/**
 * g_shell_parse_argv:
 * @command_line: command line to parse
 * @argcp: return location for number of args
 * @argvp: return location for array of args
 * @error: return location for error
 * 
 * Parses a command line into an argument vector, in much the same way
 * the shell would, but without many of the expansions the shell would
 * perform (variable expansion, globs, operators, filename expansion,
 * etc. are not supported). The results are defined to be the same as
 * those you would get from a UNIX98 /bin/sh, as long as the input
 * contains none of the unsupported shell expansions. If the input
 * does contain such expansions, they are passed through
 * literally. Possible errors are those from the #G_SHELL_ERROR
 * domain. Free the returned vector with g_strfreev().
 * 
 * Return value: %TRUE on success, %FALSE if error set
 **/
gboolean
g_shell_parse_argv (const gchar *command_line,
                    gint        *argcp,
                    gchar     ***argvp,
                    GError     **error)
{
  /* Code based on poptParseArgvString() from libpopt */
  gint argc = 0;
  gchar **argv = NULL;
  GSList *tokens = NULL;
  gint i;
  GSList *tmp_list;
  
  g_return_val_if_fail (command_line != NULL, FALSE);

  tokens = tokenize_command_line (command_line, error);
  if (tokens == NULL)
    return FALSE;

  /* Because we can't have introduced any new blank space into the
   * tokens (we didn't do any new expansions), we don't need to
   * perform field splitting. If we were going to honor IFS or do any
   * expansions, we would have to do field splitting on each word
   * here. Also, if we were going to do any expansion we would need to
   * remove any zero-length words that didn't contain quotes
   * originally; but since there's no expansion we know all words have
   * nonzero length, unless they contain quotes.
   * 
   * So, we simply remove quotes, and don't do any field splitting or
   * empty word removal, since we know there was no way to introduce
   * such things.
   */

  argc = g_slist_length (tokens);
  argv = g_new0 (gchar*, argc + 1);
  i = 0;
  tmp_list = tokens;
  while (tmp_list)
    {
      argv[i] = g_shell_unquote (tmp_list->data, error);

      /* Since we already checked that quotes matched up in the
       * tokenizer, this shouldn't be possible to reach I guess.
       */
      if (argv[i] == NULL)
        goto failed;

      tmp_list = g_slist_next (tmp_list);
      ++i;
    }
  
  g_slist_foreach (tokens, (GFunc)g_free, NULL);
  g_slist_free (tokens);
  
  if (argcp)
    *argcp = argc;

  if (argvp)
    *argvp = argv;
  else
    g_strfreev (argv);

  return TRUE;

 failed:

  g_assert (error == NULL || *error != NULL);
  g_strfreev (argv);
  g_slist_foreach (tokens, (GFunc) g_free, NULL);
  g_slist_free (tokens);
  
  return FALSE;
}

#define __G_SHELL_C__
#include "galiasdef.c"
