/* goption.c - Option parser
 *
 *  Copyright (C) 1999, 2003 Red Hat Software
 *  Copyright (C) 2004       Anders Carlsson <andersca@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "goption.h"
#include "glib.h"
#include "glibintl.h"
#include "gprintf.h"

#include "galias.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define TRANSLATE(group, str) (((group)->translate_func ? (* (group)->translate_func) ((str), (group)->translate_data) : (str)))

#define NO_ARG(entry) ((entry)->arg == G_OPTION_ARG_NONE ||       \
                       ((entry)->arg == G_OPTION_ARG_CALLBACK &&  \
                        ((entry)->flags & G_OPTION_FLAG_NO_ARG)))

#define OPTIONAL_ARG(entry) ((entry)->arg == G_OPTION_ARG_CALLBACK &&  \
                       (entry)->flags & G_OPTION_FLAG_OPTIONAL_ARG)

typedef struct 
{
  GOptionArg arg_type;
  gpointer arg_data;  
  union 
  {
    gboolean bool;
    gint integer;
    gchar *str;
    gchar **array;
    gdouble dbl;
    gint64 int64;
  } prev;
  union 
  {
    gchar *str;
    struct 
    {
      gint len;
      gchar **data;
    } array;
  } allocated;
} Change;

typedef struct
{
  gchar **ptr;
  gchar *value;
} PendingNull;

struct _GOptionContext
{
  GList           *groups;

  gchar           *parameter_string;
  gchar           *summary;
  gchar           *description;

  GTranslateFunc   translate_func;
  GDestroyNotify   translate_notify;
  gpointer	   translate_data;

  guint            help_enabled   : 1;
  guint            ignore_unknown : 1;
  
  GOptionGroup    *main_group;

  /* We keep a list of change so we can revert them */
  GList           *changes;
  
  /* We also keep track of all argv elements 
   * that should be NULLed or modified.
   */
  GList           *pending_nulls;
};

struct _GOptionGroup
{
  gchar           *name;
  gchar           *description;
  gchar           *help_description;

  GDestroyNotify   destroy_notify;
  gpointer         user_data;

  GTranslateFunc   translate_func;
  GDestroyNotify   translate_notify;
  gpointer	   translate_data;

  GOptionEntry    *entries;
  gint             n_entries;

  GOptionParseFunc pre_parse_func;
  GOptionParseFunc post_parse_func;
  GOptionErrorFunc error_func;
};

static void free_changes_list (GOptionContext *context,
			       gboolean        revert);
static void free_pending_nulls (GOptionContext *context,
				gboolean        perform_nulls);


static int
_g_unichar_get_width (gunichar c)
{
  if (G_UNLIKELY (g_unichar_iszerowidth (c)))
    return 0;

  /* we ignore the fact that we should call g_unichar_iswide_cjk() under
   * some locales (legacy East Asian ones) */
  if (g_unichar_iswide (c))
    return 2;

  return 1;
}

static glong
_g_utf8_strwidth (const gchar *p,
                  gssize       max)
{
  glong len = 0;
  const gchar *start = p;
  g_return_val_if_fail (p != NULL || max == 0, 0);

  if (max < 0)
    {
      while (*p)
        {
          len += _g_unichar_get_width (g_utf8_get_char (p));
          p = g_utf8_next_char (p);
        }
    }
  else
    {
      if (max == 0 || !*p)
        return 0;

      /* this case may not be quite correct */
      
      len += _g_unichar_get_width (g_utf8_get_char (p));
      p = g_utf8_next_char (p);          

      while (p - start < max && *p)
        {
          len += _g_unichar_get_width (g_utf8_get_char (p));
          p = g_utf8_next_char (p);          
        }
    }

  return len;
}


GQuark
g_option_error_quark (void)
{
  return g_quark_from_static_string ("g-option-context-error-quark");
}

/**
 * g_option_context_new:
 * @parameter_string: a string which is displayed in
 *    the first line of <option>--help</option> output, after the
 *    usage summary 
 *    <literal><replaceable>programname</replaceable> [OPTION...]</literal>
 *
 * Creates a new option context. 
 *
 * The @parameter_string can serve multiple purposes. It can be used
 * to add descriptions for "rest" arguments, which are not parsed by
 * the #GOptionContext, typically something like "FILES" or
 * "FILE1 FILE2...". If you are using #G_OPTION_REMAINING for
 * collecting "rest" arguments, GLib handles this automatically by
 * using the @arg_description of the corresponding #GOptionEntry in
 * the usage summary.
 *
 * Another usage is to give a short summary of the program
 * functionality, like " - frob the strings", which will be displayed
 * in the same line as the usage. For a longer description of the
 * program functionality that should be displayed as a paragraph
 * below the usage line, use g_option_context_set_summary().
 *
 * Note that the @parameter_string is translated using the
 * function set with g_option_context_set_translate_func(), so
 * it should normally be passed untranslated.
 *
 * Returns: a newly created #GOptionContext, which must be
 *    freed with g_option_context_free() after use.
 *
 * Since: 2.6
 */
GOptionContext *
g_option_context_new (const gchar *parameter_string)

{
  GOptionContext *context;

  context = g_new0 (GOptionContext, 1);

  context->parameter_string = g_strdup (parameter_string);
  context->help_enabled = TRUE;
  context->ignore_unknown = FALSE;

  return context;
}

/**
 * g_option_context_free:
 * @context: a #GOptionContext 
 *
 * Frees context and all the groups which have been 
 * added to it.
 *
 * Since: 2.6
 */
void g_option_context_free (GOptionContext *context) 
{
  g_return_if_fail (context != NULL);

  g_list_foreach (context->groups, (GFunc)g_option_group_free, NULL);
  g_list_free (context->groups);

  if (context->main_group) 
    g_option_group_free (context->main_group);

  free_changes_list (context, FALSE);
  free_pending_nulls (context, FALSE);
  
  g_free (context->parameter_string);
  g_free (context->summary);
  g_free (context->description);
  
  if (context->translate_notify)
    (* context->translate_notify) (context->translate_data);

  g_free (context);
}


/**
 * g_option_context_set_help_enabled:
 * @context: a #GOptionContext
 * @help_enabled: %TRUE to enable <option>--help</option>, %FALSE to disable it
 *
 * Enables or disables automatic generation of <option>--help</option> 
 * output. By default, g_option_context_parse() recognizes
 * <option>--help</option>, <option>-?</option>, <option>--help-all</option>
 * and <option>--help-</option><replaceable>groupname</replaceable> and creates
 * suitable output to stdout. 
 *
 * Since: 2.6
 */
void g_option_context_set_help_enabled (GOptionContext *context,
                                        gboolean        help_enabled)

{
  g_return_if_fail (context != NULL);

  context->help_enabled = help_enabled;
}

/**
 * g_option_context_get_help_enabled:
 * @context: a #GOptionContext
 * 
 * Returns whether automatic <option>--help</option> generation
 * is turned on for @context. See g_option_context_set_help_enabled().
 * 
 * Returns: %TRUE if automatic help generation is turned on.
 *
 * Since: 2.6
 */
gboolean 
g_option_context_get_help_enabled (GOptionContext *context) 
{
  g_return_val_if_fail (context != NULL, FALSE);
  
  return context->help_enabled;
}

/**
 * g_option_context_set_ignore_unknown_options:
 * @context: a #GOptionContext
 * @ignore_unknown: %TRUE to ignore unknown options, %FALSE to produce
 *    an error when unknown options are met
 * 
 * Sets whether to ignore unknown options or not. If an argument is 
 * ignored, it is left in the @argv array after parsing. By default, 
 * g_option_context_parse() treats unknown options as error.
 * 
 * This setting does not affect non-option arguments (i.e. arguments 
 * which don't start with a dash). But note that GOption cannot reliably
 * determine whether a non-option belongs to a preceding unknown option.
 *
 * Since: 2.6
 **/
void
g_option_context_set_ignore_unknown_options (GOptionContext *context,
					     gboolean	     ignore_unknown)
{
  g_return_if_fail (context != NULL);

  context->ignore_unknown = ignore_unknown;
}

/**
 * g_option_context_get_ignore_unknown_options:
 * @context: a #GOptionContext
 * 
 * Returns whether unknown options are ignored or not. See
 * g_option_context_set_ignore_unknown_options().
 * 
 * Returns: %TRUE if unknown options are ignored.
 * 
 * Since: 2.6
 **/
gboolean
g_option_context_get_ignore_unknown_options (GOptionContext *context)
{
  g_return_val_if_fail (context != NULL, FALSE);

  return context->ignore_unknown;
}

/**
 * g_option_context_add_group:
 * @context: a #GOptionContext
 * @group: the group to add
 * 
 * Adds a #GOptionGroup to the @context, so that parsing with @context
 * will recognize the options in the group. Note that the group will
 * be freed together with the context when g_option_context_free() is
 * called, so you must not free the group yourself after adding it
 * to a context.
 *
 * Since: 2.6
 **/
void
g_option_context_add_group (GOptionContext *context,
			    GOptionGroup   *group)
{
  GList *list;

  g_return_if_fail (context != NULL);
  g_return_if_fail (group != NULL);
  g_return_if_fail (group->name != NULL);
  g_return_if_fail (group->description != NULL);
  g_return_if_fail (group->help_description != NULL);

  for (list = context->groups; list; list = list->next)
    {
      GOptionGroup *g = (GOptionGroup *)list->data;

      if ((group->name == NULL && g->name == NULL) ||
	  (group->name && g->name && strcmp (group->name, g->name) == 0))
	g_warning ("A group named \"%s\" is already part of this GOptionContext", 
		   group->name);
    }

  context->groups = g_list_append (context->groups, group);
}

/**
 * g_option_context_set_main_group:
 * @context: a #GOptionContext
 * @group: the group to set as main group
 * 
 * Sets a #GOptionGroup as main group of the @context. 
 * This has the same effect as calling g_option_context_add_group(), 
 * the only difference is that the options in the main group are 
 * treated differently when generating <option>--help</option> output.
 *
 * Since: 2.6
 **/
void
g_option_context_set_main_group (GOptionContext *context,
				 GOptionGroup   *group)
{
  g_return_if_fail (context != NULL);
  g_return_if_fail (group != NULL);

  if (context->main_group)
    {
      g_warning ("This GOptionContext already has a main group");

      return;
    }
  
  context->main_group = group;
}

/**
 * g_option_context_get_main_group:
 * @context: a #GOptionContext
 * 
 * Returns a pointer to the main group of @context.
 * 
 * Return value: the main group of @context, or %NULL if @context doesn't
 *  have a main group. Note that group belongs to @context and should
 *  not be modified or freed.
 *
 * Since: 2.6
 **/
GOptionGroup *
g_option_context_get_main_group (GOptionContext *context)
{
  g_return_val_if_fail (context != NULL, NULL);

  return context->main_group;
}

/**
 * g_option_context_add_main_entries:
 * @context: a #GOptionContext
 * @entries: a %NULL-terminated array of #GOptionEntry<!-- -->s
 * @translation_domain: a translation domain to use for translating
 *    the <option>--help</option> output for the options in @entries
 *    with gettext(), or %NULL
 * 
 * A convenience function which creates a main group if it doesn't 
 * exist, adds the @entries to it and sets the translation domain.
 * 
 * Since: 2.6
 **/
void
g_option_context_add_main_entries (GOptionContext      *context,
				   const GOptionEntry  *entries,
				   const gchar         *translation_domain)
{
  g_return_if_fail (entries != NULL);

  if (!context->main_group)
    context->main_group = g_option_group_new (NULL, NULL, NULL, NULL, NULL);
  
  g_option_group_add_entries (context->main_group, entries);
  g_option_group_set_translation_domain (context->main_group, translation_domain);
}

static gint
calculate_max_length (GOptionGroup *group)
{
  GOptionEntry *entry;
  gint i, len, max_length;

  max_length = 0;

  for (i = 0; i < group->n_entries; i++)
    {
      entry = &group->entries[i];

      if (entry->flags & G_OPTION_FLAG_HIDDEN)
	continue;

      len = _g_utf8_strwidth (entry->long_name, -1);
      
      if (entry->short_name)
	len += 4;
      
      if (!NO_ARG (entry) && entry->arg_description)
	len += 1 + _g_utf8_strwidth (TRANSLATE (group, entry->arg_description), -1);
      
      max_length = MAX (max_length, len);
    }

  return max_length;
}

static void
print_entry (GOptionGroup       *group,
	     gint                max_length,
	     const GOptionEntry *entry,
             GString            *string)
{
  GString *str;
  
  if (entry->flags & G_OPTION_FLAG_HIDDEN)
    return;

  if (entry->long_name[0] == 0)
    return;

  str = g_string_new (NULL);
  
  if (entry->short_name)
    g_string_append_printf (str, "  -%c, --%s", entry->short_name, entry->long_name);
  else
    g_string_append_printf (str, "  --%s", entry->long_name);
  
  if (entry->arg_description)
    g_string_append_printf (str, "=%s", TRANSLATE (group, entry->arg_description));
  
  g_string_append_printf (string, "%s%*s %s\n", str->str,
	                  (int) (max_length + 4 - _g_utf8_strwidth (str->str, -1)), "",
	                  entry->description ? TRANSLATE (group, entry->description) : "");
  g_string_free (str, TRUE);  
}

static gboolean
group_has_visible_entries (GOptionContext *context,
                           GOptionGroup *group,
                           gboolean      main_entries)
{
  GOptionFlags reject_filter = G_OPTION_FLAG_HIDDEN;
  GOptionEntry *entry;
  gint i, l;
  gboolean main_group = group == context->main_group;

  if (!main_entries)
    reject_filter |= G_OPTION_FLAG_IN_MAIN;

  for (i = 0, l = (group ? group->n_entries : 0); i < l; i++)
    {
      entry = &group->entries[i];

      if (main_entries && !main_group && !(entry->flags & G_OPTION_FLAG_IN_MAIN))
        continue;
      if (!(entry->flags & reject_filter))
        return TRUE;
    }

  return FALSE;
}

static gboolean
group_list_has_visible_entires (GOptionContext *context,
                                GList          *group_list,
                                gboolean       main_entries)
{
  while (group_list)
    {
      if (group_has_visible_entries (context, group_list->data, main_entries))
        return TRUE;

      group_list = group_list->next;
    }

  return FALSE;
}

/**
 * g_option_context_get_help: 
 * @context: a #GOptionContext
 * @main_help: if %TRUE, only include the main group 
 * @group: the #GOptionGroup to create help for, or %NULL
 *
 * Returns a formatted, translated help text for the given context.
 * To obtain the text produced by <option>--help</option>, call
 * <literal>g_option_context_get_help (context, TRUE, NULL)</literal>.
 * To obtain the text produced by <option>--help-all</option>, call
 * <literal>g_option_context_get_help (context, FALSE, NULL)</literal>.
 * To obtain the help text for an option group, call
 * <literal>g_option_context_get_help (context, FALSE, group)</literal>.
 *
 * Returns: A newly allocated string containing the help text
 *
 * Since: 2.14
 */
gchar *
g_option_context_get_help (GOptionContext *context,
	                   gboolean        main_help,
	                   GOptionGroup   *group)
{
  GList *list;
  gint max_length, len;
  gint i;
  GOptionEntry *entry;
  GHashTable *shadow_map;
  gboolean seen[256];
  const gchar *rest_description;
  GString *string;

  string = g_string_sized_new (1024);

  rest_description = NULL;
  if (context->main_group)
    {

      for (i = 0; i < context->main_group->n_entries; i++)
	{
	  entry = &context->main_group->entries[i];
	  if (entry->long_name[0] == 0)
	    {
	      rest_description = TRANSLATE (context->main_group, entry->arg_description);
	      break;
	    }
	}
    }

  g_string_append_printf (string, "%s\n  %s %s", 
	                  _("Usage:"), g_get_prgname(), _("[OPTION...]"));

  if (rest_description)
    {
      g_string_append (string, " ");
      g_string_append (string, rest_description);
    }

  if (context->parameter_string)
    {
      g_string_append (string, " ");
      g_string_append (string, TRANSLATE (context, context->parameter_string));
    }

  g_string_append (string, "\n\n");

  if (context->summary)
    {
      g_string_append (string, TRANSLATE (context, context->summary));
      g_string_append (string, "\n\n");
    }

  memset (seen, 0, sizeof (gboolean) * 256);
  shadow_map = g_hash_table_new (g_str_hash, g_str_equal);

  if (context->main_group)
    {
      for (i = 0; i < context->main_group->n_entries; i++)
	{
	  entry = &context->main_group->entries[i];
	  g_hash_table_insert (shadow_map, 
			       (gpointer)entry->long_name, 
			       entry);
	  
	  if (seen[(guchar)entry->short_name])
	    entry->short_name = 0;
	  else
	    seen[(guchar)entry->short_name] = TRUE;
	}
    }

  list = context->groups;
  while (list != NULL)
    {
      GOptionGroup *g = list->data;
      for (i = 0; i < g->n_entries; i++)
	{
	  entry = &g->entries[i];
	  if (g_hash_table_lookup (shadow_map, entry->long_name) && 
	      !(entry->flags & G_OPTION_FLAG_NOALIAS))
	    entry->long_name = g_strdup_printf ("%s-%s", g->name, entry->long_name);
	  else  
	    g_hash_table_insert (shadow_map, (gpointer)entry->long_name, entry);

	  if (seen[(guchar)entry->short_name] && 
	      !(entry->flags & G_OPTION_FLAG_NOALIAS))
	    entry->short_name = 0;
	  else
	    seen[(guchar)entry->short_name] = TRUE;
	}
      list = list->next;
    }

  g_hash_table_destroy (shadow_map);

  list = context->groups;

  max_length = _g_utf8_strwidth ("-?, --help", -1);

  if (list)
    {
      len = _g_utf8_strwidth ("--help-all", -1);
      max_length = MAX (max_length, len);
    }

  if (context->main_group)
    {
      len = calculate_max_length (context->main_group);
      max_length = MAX (max_length, len);
    }

  while (list != NULL)
    {
      GOptionGroup *g = list->data;
      
      /* First, we check the --help-<groupname> options */
      len = _g_utf8_strwidth ("--help-", -1) + _g_utf8_strwidth (g->name, -1);
      max_length = MAX (max_length, len);

      /* Then we go through the entries */
      len = calculate_max_length (g);
      max_length = MAX (max_length, len);
      
      list = list->next;
    }

  /* Add a bit of padding */
  max_length += 4;

  if (!group)
    {
      list = context->groups;
      
      g_string_append_printf (string, "%s\n  -%c, --%-*s %s\n", 
	                      _("Help Options:"), '?', max_length - 4, "help", 
	                      _("Show help options"));
      
      /* We only want --help-all when there are groups */
      if (list)
	g_string_append_printf (string, "  --%-*s %s\n", 
                                max_length, "help-all", 
                                _("Show all help options"));
      
      while (list)
	{
	  GOptionGroup *g = list->data;

	  if (group_has_visible_entries (context, g, FALSE))
	    g_string_append_printf (string, "  --help-%-*s %s\n",
				    max_length - 5, g->name,
				    TRANSLATE (g, g->help_description));
	  
	  list = list->next;
	}

      g_string_append (string, "\n");
    }

  if (group)
    {
      /* Print a certain group */

      if (group_has_visible_entries (context, group, FALSE))
        {
          g_string_append (string, TRANSLATE (group, group->description));
          g_string_append (string, "\n");
          for (i = 0; i < group->n_entries; i++)
            print_entry (group, max_length, &group->entries[i], string);
          g_string_append (string, "\n");
        }
    }
  else if (!main_help)
    {
      /* Print all groups */

      list = context->groups;

      while (list)
	{
	  GOptionGroup *g = list->data;

	  if (group_has_visible_entries (context, g, FALSE))
	    {
	      g_string_append (string, g->description);
	      g_string_append (string, "\n");
	      for (i = 0; i < g->n_entries; i++)
		if (!(g->entries[i].flags & G_OPTION_FLAG_IN_MAIN))
		  print_entry (g, max_length, &g->entries[i], string);
	  
	      g_string_append (string, "\n");
	    }

	  list = list->next;
	}
    }
  
  /* Print application options if --help or --help-all has been specified */
  if ((main_help || !group) &&
      (group_has_visible_entries (context, context->main_group, TRUE) ||
       group_list_has_visible_entires (context, context->groups, TRUE)))
    {
      list = context->groups;

      g_string_append (string,  _("Application Options:"));
      g_string_append (string, "\n");
      if (context->main_group)
	for (i = 0; i < context->main_group->n_entries; i++) 
	  print_entry (context->main_group, max_length, 
		       &context->main_group->entries[i], string);

      while (list != NULL)
	{
	  GOptionGroup *g = list->data;

	  /* Print main entries from other groups */
	  for (i = 0; i < g->n_entries; i++)
	    if (g->entries[i].flags & G_OPTION_FLAG_IN_MAIN)
	      print_entry (g, max_length, &g->entries[i], string);
	  
	  list = list->next;
	}

      g_string_append (string, "\n");
    }
 
  if (context->description)
    {
      g_string_append (string, TRANSLATE (context, context->description));
      g_string_append (string, "\n");
    }

  return g_string_free (string, FALSE);
}

G_GNUC_NORETURN
static void
print_help (GOptionContext *context,
	    gboolean        main_help,
	    GOptionGroup   *group)
{
  gchar *help;

  help = g_option_context_get_help (context, main_help, group);
  g_print ("%s", help);
  g_free (help);

  exit (0);  
}

static gboolean
parse_int (const gchar *arg_name,
	   const gchar *arg,
	   gint        *result,
	   GError     **error)
{
  gchar *end;
  glong tmp;

  errno = 0;
  tmp = strtol (arg, &end, 0);
  
  if (*arg == '\0' || *end != '\0')
    {
      g_set_error (error,
		   G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   _("Cannot parse integer value '%s' for %s"),
		   arg, arg_name);
      return FALSE;
    }

  *result = tmp;
  if (*result != tmp || errno == ERANGE)
    {
      g_set_error (error,
		   G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   _("Integer value '%s' for %s out of range"),
		   arg, arg_name);
      return FALSE;
    }

  return TRUE;
}


static gboolean
parse_double (const gchar *arg_name,
	   const gchar *arg,
	   gdouble        *result,
	   GError     **error)
{
  gchar *end;
  gdouble tmp;

  errno = 0;
  tmp = g_strtod (arg, &end);
  
  if (*arg == '\0' || *end != '\0')
    {
      g_set_error (error,
		   G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   _("Cannot parse double value '%s' for %s"),
		   arg, arg_name);
      return FALSE;
    }
  if (errno == ERANGE)
    {
      g_set_error (error,
		   G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   _("Double value '%s' for %s out of range"),
		   arg, arg_name);
      return FALSE;
    }

  *result = tmp;
  
  return TRUE;
}


static gboolean
parse_int64 (const gchar *arg_name,
	     const gchar *arg,
	     gint64      *result,
	     GError     **error)
{
  gchar *end;
  gint64 tmp;

  errno = 0;
  tmp = g_ascii_strtoll (arg, &end, 0);

  if (*arg == '\0' || *end != '\0')
    {
      g_set_error (error,
		   G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   _("Cannot parse integer value '%s' for %s"),
		   arg, arg_name);
      return FALSE;
    }
  if (errno == ERANGE)
    {
      g_set_error (error,
		   G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   _("Integer value '%s' for %s out of range"),
		   arg, arg_name);
      return FALSE;
    }

  *result = tmp;
  
  return TRUE;
}


static Change *
get_change (GOptionContext *context,
	    GOptionArg      arg_type,
	    gpointer        arg_data)
{
  GList *list;
  Change *change = NULL;
  
  for (list = context->changes; list != NULL; list = list->next)
    {
      change = list->data;

      if (change->arg_data == arg_data)
	goto found;
    }

  change = g_new0 (Change, 1);
  change->arg_type = arg_type;
  change->arg_data = arg_data;
  
  context->changes = g_list_prepend (context->changes, change);
  
 found:

  return change;
}

static void
add_pending_null (GOptionContext *context,
		  gchar         **ptr,
		  gchar          *value)
{
  PendingNull *n;

  n = g_new0 (PendingNull, 1);
  n->ptr = ptr;
  n->value = value;

  context->pending_nulls = g_list_prepend (context->pending_nulls, n);
}
		  
static gboolean
parse_arg (GOptionContext *context,
	   GOptionGroup   *group,
	   GOptionEntry   *entry,
	   const gchar    *value,
	   const gchar    *option_name,
	   GError        **error)
     
{
  Change *change;

  g_assert (value || OPTIONAL_ARG (entry) || NO_ARG (entry));

  switch (entry->arg)
    {
    case G_OPTION_ARG_NONE:
      {
	change = get_change (context, G_OPTION_ARG_NONE,
			     entry->arg_data);

	*(gboolean *)entry->arg_data = !(entry->flags & G_OPTION_FLAG_REVERSE);
 	break;
      }	     
    case G_OPTION_ARG_STRING:
      {
	gchar *data;
	
	data = g_locale_to_utf8 (value, -1, NULL, NULL, error);

	if (!data)
	  return FALSE;

	change = get_change (context, G_OPTION_ARG_STRING,
			     entry->arg_data);
	g_free (change->allocated.str);
	
	change->prev.str = *(gchar **)entry->arg_data;
	change->allocated.str = data;
	
	*(gchar **)entry->arg_data = data;
	break;
      }
    case G_OPTION_ARG_STRING_ARRAY:
      {
	gchar *data;

	data = g_locale_to_utf8 (value, -1, NULL, NULL, error);

	if (!data)
	  return FALSE;

	change = get_change (context, G_OPTION_ARG_STRING_ARRAY,
			     entry->arg_data);

	if (change->allocated.array.len == 0)
	  {
	    change->prev.array = *(gchar ***)entry->arg_data;
	    change->allocated.array.data = g_new (gchar *, 2);
	  }
	else
	  change->allocated.array.data =
	    g_renew (gchar *, change->allocated.array.data,
		     change->allocated.array.len + 2);

	change->allocated.array.data[change->allocated.array.len] = data;
	change->allocated.array.data[change->allocated.array.len + 1] = NULL;

	change->allocated.array.len ++;

	*(gchar ***)entry->arg_data = change->allocated.array.data;

	break;
      }
      
    case G_OPTION_ARG_FILENAME:
      {
	gchar *data;

#ifdef G_OS_WIN32
	data = g_locale_to_utf8 (value, -1, NULL, NULL, error);
	
	if (!data)
	  return FALSE;
#else
	data = g_strdup (value);
#endif
	change = get_change (context, G_OPTION_ARG_FILENAME,
			     entry->arg_data);
	g_free (change->allocated.str);
	
	change->prev.str = *(gchar **)entry->arg_data;
	change->allocated.str = data;

	*(gchar **)entry->arg_data = data;
	break;
      }

    case G_OPTION_ARG_FILENAME_ARRAY:
      {
	gchar *data;
	
#ifdef G_OS_WIN32
	data = g_locale_to_utf8 (value, -1, NULL, NULL, error);
	
	if (!data)
	  return FALSE;
#else
	data = g_strdup (value);
#endif
	change = get_change (context, G_OPTION_ARG_STRING_ARRAY,
			     entry->arg_data);

	if (change->allocated.array.len == 0)
	  {
	    change->prev.array = *(gchar ***)entry->arg_data;
	    change->allocated.array.data = g_new (gchar *, 2);
	  }
	else
	  change->allocated.array.data =
	    g_renew (gchar *, change->allocated.array.data,
		     change->allocated.array.len + 2);

	change->allocated.array.data[change->allocated.array.len] = data;
	change->allocated.array.data[change->allocated.array.len + 1] = NULL;

	change->allocated.array.len ++;

	*(gchar ***)entry->arg_data = change->allocated.array.data;

	break;
      }
      
    case G_OPTION_ARG_INT:
      {
	gint data;

	if (!parse_int (option_name, value,
 			&data,
			error))
	  return FALSE;

	change = get_change (context, G_OPTION_ARG_INT,
			     entry->arg_data);
	change->prev.integer = *(gint *)entry->arg_data;
	*(gint *)entry->arg_data = data;
	break;
      }
    case G_OPTION_ARG_CALLBACK:
      {
	gchar *data;
	gboolean retval;

	if (!value && entry->flags & G_OPTION_FLAG_OPTIONAL_ARG)
	  data = NULL;
	else if (entry->flags & G_OPTION_FLAG_NO_ARG)
	  data = NULL;
	else if (entry->flags & G_OPTION_FLAG_FILENAME)
	  {
#ifdef G_OS_WIN32
  	    data = g_locale_to_utf8 (value, -1, NULL, NULL, error);
#else
	    data = g_strdup (value);
#endif
	  }
	else
	  data = g_locale_to_utf8 (value, -1, NULL, NULL, error);

	if (!(entry->flags & (G_OPTION_FLAG_NO_ARG|G_OPTION_FLAG_OPTIONAL_ARG)) && 
	    !data)
	  return FALSE;

	retval = (* (GOptionArgFunc) entry->arg_data) (option_name, data, group->user_data, error);
	
	if (!retval && error != NULL && *error == NULL)
	  g_set_error (error, 
		       G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
		       _("Error parsing option %s"), option_name);

	g_free (data);
	
	return retval;
	
	break;
      }
    case G_OPTION_ARG_DOUBLE:
      {
	gdouble data;

	if (!parse_double (option_name, value,
 			&data,
			error))
	  {
	    return FALSE;
	  }

	change = get_change (context, G_OPTION_ARG_DOUBLE,
			     entry->arg_data);
	change->prev.dbl = *(gdouble *)entry->arg_data;
	*(gdouble *)entry->arg_data = data;
	break;
      }
    case G_OPTION_ARG_INT64:
      {
        gint64 data;

	if (!parse_int64 (option_name, value,
 			 &data,
			 error))
	  {
	    return FALSE;
	  }

	change = get_change (context, G_OPTION_ARG_INT64,
			     entry->arg_data);
	change->prev.int64 = *(gint64 *)entry->arg_data;
	*(gint64 *)entry->arg_data = data;
	break;
      }
    default:
      g_assert_not_reached ();
    }

  return TRUE;
}

static gboolean
parse_short_option (GOptionContext *context,
		    GOptionGroup   *group,
		    gint            idx,
		    gint           *new_idx,
		    gchar           arg,
		    gint           *argc,
		    gchar        ***argv,
		    GError        **error,
		    gboolean       *parsed)
{
  gint j;
    
  for (j = 0; j < group->n_entries; j++)
    {
      if (arg == group->entries[j].short_name)
	{
	  gchar *option_name;
	  gchar *value = NULL;
	  
	  option_name = g_strdup_printf ("-%c", group->entries[j].short_name);

	  if (NO_ARG (&group->entries[j]))
	    value = NULL;
	  else
	    {
	      if (*new_idx > idx)
		{
		  g_set_error (error, 
			       G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
			       _("Error parsing option %s"), option_name);
		  g_free (option_name);
		  return FALSE;
		}

	      if (idx < *argc - 1)
		{
		  if (!OPTIONAL_ARG (&group->entries[j]))	
		    {    
		      value = (*argv)[idx + 1];
		      add_pending_null (context, &((*argv)[idx + 1]), NULL);
		      *new_idx = idx + 1;
		    }
		  else
		    {
                      if ((*argv)[idx + 1][0] == '-') 
			value = NULL;
		      else
		        {
		          value = (*argv)[idx + 1];
		          add_pending_null (context, &((*argv)[idx + 1]), NULL);
		          *new_idx = idx + 1;
			}
	            }
		}
	      else if (idx >= *argc - 1 && OPTIONAL_ARG (&group->entries[j]))
		value = NULL;
	      else
		{
		  g_set_error (error, 
			       G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
			       _("Missing argument for %s"), option_name);
		  g_free (option_name);
		  return FALSE;
		}
	    }

	  if (!parse_arg (context, group, &group->entries[j], 
			  value, option_name, error))
	    {
	      g_free (option_name);
	      return FALSE;
	    }
	  
	  g_free (option_name);
	  *parsed = TRUE;
	}
    }

  return TRUE;
}

static gboolean
parse_long_option (GOptionContext *context,
		   GOptionGroup   *group,
		   gint           *idx,
		   gchar          *arg,
		   gboolean        aliased,
		   gint           *argc,
		   gchar        ***argv,
		   GError        **error,
		   gboolean       *parsed)
{
  gint j;

  for (j = 0; j < group->n_entries; j++)
    {
      if (*idx >= *argc)
	return TRUE;

      if (aliased && (group->entries[j].flags & G_OPTION_FLAG_NOALIAS))
	continue;

      if (NO_ARG (&group->entries[j]) &&
	  strcmp (arg, group->entries[j].long_name) == 0)
	{
	  gchar *option_name;
	  gboolean retval;

	  option_name = g_strconcat ("--", group->entries[j].long_name, NULL);
	  retval = parse_arg (context, group, &group->entries[j],
			      NULL, option_name, error);
	  g_free(option_name);
	  
	  add_pending_null (context, &((*argv)[*idx]), NULL);
	  *parsed = TRUE;

	  return retval;
	}
      else
	{
	  gint len = strlen (group->entries[j].long_name);
	  
	  if (strncmp (arg, group->entries[j].long_name, len) == 0 &&
	      (arg[len] == '=' || arg[len] == 0))
	    {
	      gchar *value = NULL;
	      gchar *option_name;

	      add_pending_null (context, &((*argv)[*idx]), NULL);
	      option_name = g_strconcat ("--", group->entries[j].long_name, NULL);

	      if (arg[len] == '=')
		value = arg + len + 1;
	      else if (*idx < *argc - 1) 
		{
		  if (!(group->entries[j].flags & G_OPTION_FLAG_OPTIONAL_ARG))	
		    {    
		      value = (*argv)[*idx + 1];
		      add_pending_null (context, &((*argv)[*idx + 1]), NULL);
		      (*idx)++;
		    }
		  else
		    {
                      if ((*argv)[*idx + 1][0] == '-') 
		        {
		          gboolean retval;
		          retval = parse_arg (context, group, &group->entries[j],
					      NULL, option_name, error);
	  	          *parsed = TRUE;
		          g_free (option_name);
	   	          return retval;
		        }
		      else
		        {
		          value = (*argv)[*idx + 1];
		          add_pending_null (context, &((*argv)[*idx + 1]), NULL);
		          (*idx)++;
			}
	            }
		}
	      else if (*idx >= *argc - 1 &&
		       group->entries[j].flags & G_OPTION_FLAG_OPTIONAL_ARG)
		{
		    gboolean retval;
		    retval = parse_arg (context, group, &group->entries[j],
					NULL, option_name, error);
	  	    *parsed = TRUE;
		    g_free (option_name);
	   	    return retval;
		}
	      else
		{
		  g_set_error (error, 
			       G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
			       _("Missing argument for %s"), option_name);
		  g_free (option_name);
		  return FALSE;
		}

	      if (!parse_arg (context, group, &group->entries[j], 
			      value, option_name, error))
		{
		  g_free (option_name);
		  return FALSE;
		}

	      g_free (option_name);
	      *parsed = TRUE;
	    } 
	}
    }
  
  return TRUE;
}

static gboolean
parse_remaining_arg (GOptionContext *context,
		     GOptionGroup   *group,
		     gint           *idx,
		     gint           *argc,
		     gchar        ***argv,
		     GError        **error,
		     gboolean       *parsed)
{
  gint j;

  for (j = 0; j < group->n_entries; j++)
    {
      if (*idx >= *argc)
	return TRUE;

      if (group->entries[j].long_name[0])
	continue;

      g_return_val_if_fail (group->entries[j].arg == G_OPTION_ARG_CALLBACK ||
                            group->entries[j].arg == G_OPTION_ARG_STRING_ARRAY ||
			    group->entries[j].arg == G_OPTION_ARG_FILENAME_ARRAY, FALSE);
      
      add_pending_null (context, &((*argv)[*idx]), NULL);
      
      if (!parse_arg (context, group, &group->entries[j], (*argv)[*idx], "", error))
	return FALSE;
      
      *parsed = TRUE;
      return TRUE;
    }

  return TRUE;
}

static void
free_changes_list (GOptionContext *context,
		   gboolean        revert)
{
  GList *list;

  for (list = context->changes; list != NULL; list = list->next)
    {
      Change *change = list->data;

      if (revert)
	{
	  switch (change->arg_type)
	    {
	    case G_OPTION_ARG_NONE:
	      *(gboolean *)change->arg_data = change->prev.bool;
	      break;
	    case G_OPTION_ARG_INT:
	      *(gint *)change->arg_data = change->prev.integer;
	      break;
	    case G_OPTION_ARG_STRING:
	    case G_OPTION_ARG_FILENAME:
              g_free (change->allocated.str);
	      *(gchar **)change->arg_data = change->prev.str;
	      break;
	    case G_OPTION_ARG_STRING_ARRAY:
	    case G_OPTION_ARG_FILENAME_ARRAY:
	      g_strfreev (change->allocated.array.data);
	      *(gchar ***)change->arg_data = change->prev.array;
	      break;
	    case G_OPTION_ARG_DOUBLE:
	      *(gdouble *)change->arg_data = change->prev.dbl;
	      break;
	    case G_OPTION_ARG_INT64:
	      *(gint64 *)change->arg_data = change->prev.int64;
	      break;
	    default:
	      g_assert_not_reached ();
	    }
	}
      
      g_free (change);
    }

  g_list_free (context->changes);
  context->changes = NULL;
}

static void
free_pending_nulls (GOptionContext *context,
		    gboolean        perform_nulls)
{
  GList *list;

  for (list = context->pending_nulls; list != NULL; list = list->next)
    {
      PendingNull *n = list->data;

      if (perform_nulls)
	{
	  if (n->value)
	    {
	      /* Copy back the short options */
	      *(n->ptr)[0] = '-';	      
	      strcpy (*n->ptr + 1, n->value);
	    }
	  else
	    *n->ptr = NULL;
	}
      
      g_free (n->value);
      g_free (n);
    }

  g_list_free (context->pending_nulls);
  context->pending_nulls = NULL;
}

/**
 * g_option_context_parse:
 * @context: a #GOptionContext
 * @argc: a pointer to the number of command line arguments
 * @argv: a pointer to the array of command line arguments
 * @error: a return location for errors 
 * 
 * Parses the command line arguments, recognizing options
 * which have been added to @context. A side-effect of 
 * calling this function is that g_set_prgname() will be
 * called.
 *
 * If the parsing is successful, any parsed arguments are
 * removed from the array and @argc and @argv are updated 
 * accordingly. A '--' option is stripped from @argv
 * unless there are unparsed options before and after it, 
 * or some of the options after it start with '-'. In case 
 * of an error, @argc and @argv are left unmodified. 
 *
 * If automatic <option>--help</option> support is enabled
 * (see g_option_context_set_help_enabled()), and the 
 * @argv array contains one of the recognized help options,
 * this function will produce help output to stdout and
 * call <literal>exit (0)</literal>.
 *
 * Note that function depends on the 
 * <link linkend="setlocale">current locale</link> for 
 * automatic character set conversion of string and filename
 * arguments.
 * 
 * Return value: %TRUE if the parsing was successful, 
 *               %FALSE if an error occurred
 *
 * Since: 2.6
 **/
gboolean
g_option_context_parse (GOptionContext   *context,
			gint             *argc,
			gchar          ***argv,
			GError          **error)
{
  gint i, j, k;
  GList *list;

  /* Set program name */
  if (!g_get_prgname())
    {
      if (argc && argv && *argc)
	{
	  gchar *prgname;
	  
	  prgname = g_path_get_basename ((*argv)[0]);
	  g_set_prgname (prgname);
	  g_free (prgname);
	}
      else
	g_set_prgname ("<unknown>");
    }

  /* Call pre-parse hooks */
  list = context->groups;
  while (list)
    {
      GOptionGroup *group = list->data;
      
      if (group->pre_parse_func)
	{
	  if (!(* group->pre_parse_func) (context, group,
					  group->user_data, error))
	    goto fail;
	}
      
      list = list->next;
    }

  if (context->main_group && context->main_group->pre_parse_func)
    {
      if (!(* context->main_group->pre_parse_func) (context, context->main_group,
						    context->main_group->user_data, error))
	goto fail;
    }

  if (argc && argv)
    {
      gboolean stop_parsing = FALSE;
      gboolean has_unknown = FALSE;
      gint separator_pos = 0;

      for (i = 1; i < *argc; i++)
	{
	  gchar *arg, *dash;
	  gboolean parsed = FALSE;

	  if ((*argv)[i][0] == '-' && (*argv)[i][1] != '\0' && !stop_parsing)
	    {
	      if ((*argv)[i][1] == '-')
		{
		  /* -- option */

		  arg = (*argv)[i] + 2;

		  /* '--' terminates list of arguments */
		  if (*arg == 0)
		    {
		      separator_pos = i;
		      stop_parsing = TRUE;
		      continue;
		    }

		  /* Handle help options */
		  if (context->help_enabled)
		    {
		      if (strcmp (arg, "help") == 0)
			print_help (context, TRUE, NULL);
		      else if (strcmp (arg, "help-all") == 0)
			print_help (context, FALSE, NULL);		      
		      else if (strncmp (arg, "help-", 5) == 0)
			{
			  list = context->groups;
			  
			  while (list)
			    {
			      GOptionGroup *group = list->data;
			      
			      if (strcmp (arg + 5, group->name) == 0)
				print_help (context, FALSE, group);		      			      
			      
			      list = list->next;
			    }
			}
		    }

		  if (context->main_group &&
		      !parse_long_option (context, context->main_group, &i, arg,
					  FALSE, argc, argv, error, &parsed))
		    goto fail;

		  if (parsed)
		    continue;
		  
		  /* Try the groups */
		  list = context->groups;
		  while (list)
		    {
		      GOptionGroup *group = list->data;
		      
		      if (!parse_long_option (context, group, &i, arg, 
					      FALSE, argc, argv, error, &parsed))
			goto fail;
		      
		      if (parsed)
			break;
		      
		      list = list->next;
		    }
		  
		  if (parsed)
		    continue;

		  /* Now look for --<group>-<option> */
		  dash = strchr (arg, '-');
		  if (dash)
		    {
		      /* Try the groups */
		      list = context->groups;
		      while (list)
			{
			  GOptionGroup *group = list->data;
			  
			  if (strncmp (group->name, arg, dash - arg) == 0)
			    {
			      if (!parse_long_option (context, group, &i, dash + 1,
						      TRUE, argc, argv, error, &parsed))
				goto fail;
			      
			      if (parsed)
				break;
			    }
			  
			  list = list->next;
			}
		    }
		  
		  if (context->ignore_unknown)
		    continue;
		}
	      else
		{ /* short option */
		  gint new_i = i, arg_length;
		  gboolean *nulled_out = NULL;
		  arg = (*argv)[i] + 1;
                  arg_length = strlen (arg);
                  nulled_out = g_newa (gboolean, arg_length);
                  memset (nulled_out, 0, arg_length * sizeof (gboolean));
		  for (j = 0; j < arg_length; j++)
		    {
		      if (context->help_enabled && arg[j] == '?')
			print_help (context, TRUE, NULL);
		      parsed = FALSE;
		      if (context->main_group &&
			  !parse_short_option (context, context->main_group,
					       i, &new_i, arg[j],
					       argc, argv, error, &parsed))
                        goto fail;
		      if (!parsed)
			{
			  /* Try the groups */
			  list = context->groups;
			  while (list)
			    {
			      GOptionGroup *group = list->data;
			      if (!parse_short_option (context, group, i, &new_i, arg[j],
						       argc, argv, error, &parsed))
				goto fail;
			      if (parsed)
				break;
			      list = list->next;
			    }
			}
                      
		      if (context->ignore_unknown && parsed)
                        nulled_out[j] = TRUE;
                      else if (context->ignore_unknown)
                        continue;
                      else if (!parsed)
                        break;
                      /* !context->ignore_unknown && parsed */
		    }
		  if (context->ignore_unknown)
		    {
		      gchar *new_arg = NULL; 
		      gint arg_index = 0;
		      for (j = 0; j < arg_length; j++)
			{
			  if (!nulled_out[j])
			    {
			      if (!new_arg)
				new_arg = g_malloc (arg_length + 1);
			      new_arg[arg_index++] = arg[j];
			    }
			}
		      if (new_arg)
			new_arg[arg_index] = '\0';
		      add_pending_null (context, &((*argv)[i]), new_arg);
		    }
		  else if (parsed)
		    {
		      add_pending_null (context, &((*argv)[i]), NULL);
		      i = new_i;
		    }
		}
	      
	      if (!parsed)
		has_unknown = TRUE;

	      if (!parsed && !context->ignore_unknown)
		{
		  g_set_error (error,
			       G_OPTION_ERROR, G_OPTION_ERROR_UNKNOWN_OPTION,
				   _("Unknown option %s"), (*argv)[i]);
		  goto fail;
		}
	    }
	  else
	    {
	      /* Collect remaining args */
	      if (context->main_group &&
		  !parse_remaining_arg (context, context->main_group, &i,
					argc, argv, error, &parsed))
		goto fail;
	      
	      if (!parsed && (has_unknown || (*argv)[i][0] == '-'))
		separator_pos = 0;
	    }
	}

      if (separator_pos > 0)
	add_pending_null (context, &((*argv)[separator_pos]), NULL);
	
    }

  /* Call post-parse hooks */
  list = context->groups;
  while (list)
    {
      GOptionGroup *group = list->data;
      
      if (group->post_parse_func)
	{
	  if (!(* group->post_parse_func) (context, group,
					   group->user_data, error))
	    goto fail;
	}
      
      list = list->next;
    }
  
  if (context->main_group && context->main_group->post_parse_func)
    {
      if (!(* context->main_group->post_parse_func) (context, context->main_group,
						     context->main_group->user_data, error))
	goto fail;
    }
  
  if (argc && argv)
    {
      free_pending_nulls (context, TRUE);
      
      for (i = 1; i < *argc; i++)
	{
	  for (k = i; k < *argc; k++)
	    if ((*argv)[k] != NULL)
	      break;
	  
	  if (k > i)
	    {
	      k -= i;
	      for (j = i + k; j < *argc; j++)
		{
		  (*argv)[j-k] = (*argv)[j];
		  (*argv)[j] = NULL;
		}
	      *argc -= k;
	    }
	}      
    }

  return TRUE;

 fail:
  
  /* Call error hooks */
  list = context->groups;
  while (list)
    {
      GOptionGroup *group = list->data;
      
      if (group->error_func)
	(* group->error_func) (context, group,
			       group->user_data, error);
      
      list = list->next;
    }

  if (context->main_group && context->main_group->error_func)
    (* context->main_group->error_func) (context, context->main_group,
					 context->main_group->user_data, error);
  
  free_changes_list (context, TRUE);
  free_pending_nulls (context, FALSE);

  return FALSE;
}
				   
/**
 * g_option_group_new:
 * @name: the name for the option group, this is used to provide
 *   help for the options in this group with <option>--help-</option>@name
 * @description: a description for this group to be shown in 
 *   <option>--help</option>. This string is translated using the translation
 *   domain or translation function of the group
 * @help_description: a description for the <option>--help-</option>@name option.
 *   This string is translated using the translation domain or translation function
 *   of the group
 * @user_data: user data that will be passed to the pre- and post-parse hooks,
 *   the error hook and to callbacks of %G_OPTION_ARG_CALLBACK options, or %NULL
 * @destroy: a function that will be called to free @user_data, or %NULL
 * 
 * Creates a new #GOptionGroup.
 * 
 * Return value: a newly created option group. It should be added 
 *   to a #GOptionContext or freed with g_option_group_free().
 *
 * Since: 2.6
 **/
GOptionGroup *
g_option_group_new (const gchar    *name,
		    const gchar    *description,
		    const gchar    *help_description,
		    gpointer        user_data,
		    GDestroyNotify  destroy)

{
  GOptionGroup *group;

  group = g_new0 (GOptionGroup, 1);
  group->name = g_strdup (name);
  group->description = g_strdup (description);
  group->help_description = g_strdup (help_description);
  group->user_data = user_data;
  group->destroy_notify = destroy;
  
  return group;
}


/**
 * g_option_group_free:
 * @group: a #GOptionGroup
 * 
 * Frees a #GOptionGroup. Note that you must <emphasis>not</emphasis>
 * free groups which have been added to a #GOptionContext.
 *
 * Since: 2.6
 **/
void
g_option_group_free (GOptionGroup *group)
{
  g_return_if_fail (group != NULL);

  g_free (group->name);
  g_free (group->description);
  g_free (group->help_description);

  g_free (group->entries);
  
  if (group->destroy_notify)
    (* group->destroy_notify) (group->user_data);

  if (group->translate_notify)
    (* group->translate_notify) (group->translate_data);
  
  g_free (group);
}


/**
 * g_option_group_add_entries:
 * @group: a #GOptionGroup
 * @entries: a %NULL-terminated array of #GOptionEntry<!-- -->s
 * 
 * Adds the options specified in @entries to @group.
 *
 * Since: 2.6
 **/
void
g_option_group_add_entries (GOptionGroup       *group,
			    const GOptionEntry *entries)
{
  gint i, n_entries;
  
  g_return_if_fail (entries != NULL);

  for (n_entries = 0; entries[n_entries].long_name != NULL; n_entries++) ;

  group->entries = g_renew (GOptionEntry, group->entries, group->n_entries + n_entries);

  memcpy (group->entries + group->n_entries, entries, sizeof (GOptionEntry) * n_entries);

  for (i = group->n_entries; i < group->n_entries + n_entries; i++)
    {
      gchar c = group->entries[i].short_name;

      if (c)
	{
	  if (c == '-' || !g_ascii_isprint (c))
	    {
	      g_warning (G_STRLOC": ignoring invalid short option '%c' (%d)", c, c);
	      group->entries[i].short_name = 0;
	    }
	}
    }

  group->n_entries += n_entries;
}

/**
 * g_option_group_set_parse_hooks:
 * @group: a #GOptionGroup
 * @pre_parse_func: a function to call before parsing, or %NULL
 * @post_parse_func: a function to call after parsing, or %NULL
 * 
 * Associates two functions with @group which will be called 
 * from g_option_context_parse() before the first option is parsed
 * and after the last option has been parsed, respectively.
 *
 * Note that the user data to be passed to @pre_parse_func and
 * @post_parse_func can be specified when constructing the group
 * with g_option_group_new().
 *
 * Since: 2.6
 **/
void
g_option_group_set_parse_hooks (GOptionGroup     *group,
				GOptionParseFunc  pre_parse_func,
				GOptionParseFunc  post_parse_func)
{
  g_return_if_fail (group != NULL);

  group->pre_parse_func = pre_parse_func;
  group->post_parse_func = post_parse_func;  
}

/**
 * g_option_group_set_error_hook:
 * @group: a #GOptionGroup
 * @error_func: a function to call when an error occurs
 * 
 * Associates a function with @group which will be called 
 * from g_option_context_parse() when an error occurs.
 *
 * Note that the user data to be passed to @error_func can be
 * specified when constructing the group with g_option_group_new().
 *
 * Since: 2.6
 **/
void
g_option_group_set_error_hook (GOptionGroup     *group,
			       GOptionErrorFunc	 error_func)
{
  g_return_if_fail (group != NULL);

  group->error_func = error_func;  
}


/**
 * g_option_group_set_translate_func:
 * @group: a #GOptionGroup
 * @func: the #GTranslateFunc, or %NULL 
 * @data: user data to pass to @func, or %NULL
 * @destroy_notify: a function which gets called to free @data, or %NULL
 * 
 * Sets the function which is used to translate user-visible
 * strings, for <option>--help</option> output. Different
 * groups can use different #GTranslateFunc<!-- -->s. If @func
 * is %NULL, strings are not translated.
 *
 * If you are using gettext(), you only need to set the translation
 * domain, see g_option_group_set_translation_domain().
 *
 * Since: 2.6
 **/
void
g_option_group_set_translate_func (GOptionGroup   *group,
				   GTranslateFunc  func,
				   gpointer        data,
				   GDestroyNotify  destroy_notify)
{
  g_return_if_fail (group != NULL);
  
  if (group->translate_notify)
    group->translate_notify (group->translate_data);
      
  group->translate_func = func;
  group->translate_data = data;
  group->translate_notify = destroy_notify;
}

static const gchar *
dgettext_swapped (const gchar *msgid, 
		  const gchar *domainname)
{
  return g_dgettext (domainname, msgid);
}

/**
 * g_option_group_set_translation_domain:
 * @group: a #GOptionGroup
 * @domain: the domain to use
 * 
 * A convenience function to use gettext() for translating
 * user-visible strings. 
 * 
 * Since: 2.6
 **/
void
g_option_group_set_translation_domain (GOptionGroup *group,
				       const gchar  *domain)
{
  g_return_if_fail (group != NULL);

  g_option_group_set_translate_func (group, 
				     (GTranslateFunc)dgettext_swapped,
				     g_strdup (domain),
				     g_free);
} 

/**
 * g_option_context_set_translate_func:
 * @context: a #GOptionContext
 * @func: the #GTranslateFunc, or %NULL 
 * @data: user data to pass to @func, or %NULL
 * @destroy_notify: a function which gets called to free @data, or %NULL
 * 
 * Sets the function which is used to translate the contexts 
 * user-visible strings, for <option>--help</option> output. 
 * If @func is %NULL, strings are not translated.
 *
 * Note that option groups have their own translation functions, 
 * this function only affects the @parameter_string (see g_option_context_new()), 
 * the summary (see g_option_context_set_summary()) and the description 
 * (see g_option_context_set_description()).
 *
 * If you are using gettext(), you only need to set the translation
 * domain, see g_option_context_set_translation_domain().
 *
 * Since: 2.12
 **/
void
g_option_context_set_translate_func (GOptionContext *context,
				     GTranslateFunc func,
				     gpointer       data,
				     GDestroyNotify destroy_notify)
{
  g_return_if_fail (context != NULL);
  
  if (context->translate_notify)
    context->translate_notify (context->translate_data);
      
  context->translate_func = func;
  context->translate_data = data;
  context->translate_notify = destroy_notify;
}

/**
 * g_option_context_set_translation_domain:
 * @context: a #GOptionContext
 * @domain: the domain to use
 * 
 * A convenience function to use gettext() for translating
 * user-visible strings. 
 * 
 * Since: 2.12
 **/
void
g_option_context_set_translation_domain (GOptionContext *context,
					 const gchar     *domain)
{
  g_return_if_fail (context != NULL);

  g_option_context_set_translate_func (context, 
				       (GTranslateFunc)dgettext_swapped,
				       g_strdup (domain),
				       g_free);
}

/**
 * g_option_context_set_summary:
 * @context: a #GOptionContext
 * @summary: a string to be shown in <option>--help</option> output 
 *  before the list of options, or %NULL
 * 
 * Adds a string to be displayed in <option>--help</option> output
 * before the list of options. This is typically a summary of the
 * program functionality. 
 *
 * Note that the summary is translated (see 
 * g_option_context_set_translate_func() and
 * g_option_context_set_translation_domain()).
 *
 * Since: 2.12
 */
void
g_option_context_set_summary (GOptionContext *context,
			      const gchar    *summary)
{
  g_return_if_fail (context != NULL);

  g_free (context->summary);
  context->summary = g_strdup (summary);
}


/**
 * g_option_context_get_summary:
 * @context: a #GOptionContext
 * 
 * Returns the summary. See g_option_context_set_summary().
 *
 * Returns: the summary
 *
 * Since: 2.12
 */
G_CONST_RETURN gchar *
g_option_context_get_summary (GOptionContext *context)
{
  g_return_val_if_fail (context != NULL, NULL);

  return context->summary;
}

/**
 * g_option_context_set_description:
 * @context: a #GOptionContext
 * @description: a string to be shown in <option>--help</option> output 
 *   after the list of options, or %NULL
 * 
 * Adds a string to be displayed in <option>--help</option> output
 * after the list of options. This text often includes a bug reporting
 * address.
 *
 * Note that the summary is translated (see 
 * g_option_context_set_translate_func()).
 *
 * Since: 2.12
 */
void
g_option_context_set_description (GOptionContext *context,
			          const gchar    *description)
{
  g_return_if_fail (context != NULL);

  g_free (context->description);
  context->description = g_strdup (description);
}


/**
 * g_option_context_get_description:
 * @context: a #GOptionContext
 * 
 * Returns the description. See g_option_context_set_description().
 *
 * Returns: the description
 *
 * Since: 2.12
 */
G_CONST_RETURN gchar *
g_option_context_get_description (GOptionContext *context)
{
  g_return_val_if_fail (context != NULL, NULL);

  return context->description;
}


#define __G_OPTION_C__
#include "galiasdef.c"
