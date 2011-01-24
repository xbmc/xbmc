/* goption.h - Option parser
 *
 *  Copyright (C) 2004  Anders Carlsson <andersca@gnome.org>
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

#if defined(G_DISABLE_SINGLE_INCLUDES) && !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#ifndef __G_OPTION_H__
#define __G_OPTION_H__

#include <glib/gerror.h>
#include <glib/gquark.h>

G_BEGIN_DECLS

typedef struct _GOptionContext GOptionContext;
typedef struct _GOptionGroup   GOptionGroup;
typedef struct _GOptionEntry   GOptionEntry;

typedef enum
{
  G_OPTION_FLAG_HIDDEN		= 1 << 0,
  G_OPTION_FLAG_IN_MAIN		= 1 << 1,
  G_OPTION_FLAG_REVERSE		= 1 << 2,
  G_OPTION_FLAG_NO_ARG		= 1 << 3,
  G_OPTION_FLAG_FILENAME	= 1 << 4,
  G_OPTION_FLAG_OPTIONAL_ARG    = 1 << 5,
  G_OPTION_FLAG_NOALIAS	        = 1 << 6
} GOptionFlags;

typedef enum
{
  G_OPTION_ARG_NONE,
  G_OPTION_ARG_STRING,
  G_OPTION_ARG_INT,
  G_OPTION_ARG_CALLBACK,
  G_OPTION_ARG_FILENAME,
  G_OPTION_ARG_STRING_ARRAY,
  G_OPTION_ARG_FILENAME_ARRAY,
  G_OPTION_ARG_DOUBLE,
  G_OPTION_ARG_INT64
} GOptionArg;

typedef gboolean (*GOptionArgFunc) (const gchar    *option_name,
				    const gchar    *value,
				    gpointer        data,
				    GError        **error);

typedef gboolean (*GOptionParseFunc) (GOptionContext *context,
				      GOptionGroup   *group,
				      gpointer	      data,
				      GError        **error);

typedef void (*GOptionErrorFunc) (GOptionContext *context,
				  GOptionGroup   *group,
				  gpointer        data,
				  GError        **error);

#define G_OPTION_ERROR (g_option_error_quark ())

typedef enum
{
  G_OPTION_ERROR_UNKNOWN_OPTION,
  G_OPTION_ERROR_BAD_VALUE,
  G_OPTION_ERROR_FAILED
} GOptionError;

GQuark g_option_error_quark (void);


struct _GOptionEntry
{
  const gchar *long_name;
  gchar        short_name;
  gint         flags;

  GOptionArg   arg;
  gpointer     arg_data;
  
  const gchar *description;
  const gchar *arg_description;
};

#define G_OPTION_REMAINING ""

GOptionContext *g_option_context_new              (const gchar         *parameter_string);
void            g_option_context_set_summary      (GOptionContext      *context,
                                                   const gchar         *summary);
G_CONST_RETURN gchar *g_option_context_get_summary (GOptionContext     *context);
void            g_option_context_set_description  (GOptionContext      *context,
                                                   const gchar         *description);
G_CONST_RETURN gchar *g_option_context_get_description (GOptionContext     *context);
void            g_option_context_free             (GOptionContext      *context);
void		g_option_context_set_help_enabled (GOptionContext      *context,
						   gboolean		help_enabled);
gboolean	g_option_context_get_help_enabled (GOptionContext      *context);
void		g_option_context_set_ignore_unknown_options (GOptionContext *context,
							     gboolean	     ignore_unknown);
gboolean        g_option_context_get_ignore_unknown_options (GOptionContext *context);

void            g_option_context_add_main_entries (GOptionContext      *context,
						   const GOptionEntry  *entries,
						   const gchar         *translation_domain);
gboolean        g_option_context_parse            (GOptionContext      *context,
						   gint                *argc,
						   gchar             ***argv,
						   GError             **error);
void            g_option_context_set_translate_func (GOptionContext     *context,
						     GTranslateFunc      func,
						     gpointer            data,
						     GDestroyNotify      destroy_notify);
void            g_option_context_set_translation_domain (GOptionContext  *context,
							 const gchar     *domain);

void            g_option_context_add_group      (GOptionContext *context,
						 GOptionGroup   *group);
void          g_option_context_set_main_group (GOptionContext *context,
					       GOptionGroup   *group);
GOptionGroup *g_option_context_get_main_group (GOptionContext *context);
gchar        *g_option_context_get_help       (GOptionContext *context,
                                               gboolean        main_help,
                                               GOptionGroup   *group);

GOptionGroup *g_option_group_new                    (const gchar        *name,
						     const gchar        *description,
						     const gchar        *help_description,
						     gpointer            user_data,
						     GDestroyNotify      destroy);
void	      g_option_group_set_parse_hooks	    (GOptionGroup       *group,
						     GOptionParseFunc    pre_parse_func,
						     GOptionParseFunc	 post_parse_func);
void	      g_option_group_set_error_hook	    (GOptionGroup       *group,
						     GOptionErrorFunc	 error_func);
void          g_option_group_free                   (GOptionGroup       *group);
void          g_option_group_add_entries            (GOptionGroup       *group,
						     const GOptionEntry *entries);
void          g_option_group_set_translate_func     (GOptionGroup       *group,
						     GTranslateFunc      func,
						     gpointer            data,
						     GDestroyNotify      destroy_notify);
void          g_option_group_set_translation_domain (GOptionGroup       *group,
						     const gchar        *domain);

G_END_DECLS

#endif /* __G_OPTION_H__ */
