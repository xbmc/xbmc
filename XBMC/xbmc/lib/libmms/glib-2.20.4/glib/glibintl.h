#ifndef __GLIBINTL_H__
#define __GLIBINTL_H__

#ifndef SIZEOF_CHAR
#error "config.h must be included prior to glibintl.h"
#endif

G_CONST_RETURN gchar *glib_gettext (const gchar *str) G_GNUC_FORMAT(1);

#ifdef ENABLE_NLS

#include <libintl.h>
#define _(String) glib_gettext(String)
/* Split out this in the code, but keep it in the same domain for now */
#define P_(String) glib_gettext(String)

#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif
#else /* NLS is disabled */
#define _(String) (String)
#define N_(String) (String)
#define P_(String) (String)
#define textdomain(String) ((String) ? (String) : "messages")
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define dngettext(Domain,String1,String2,N) ((N) == 1 ? (String1) : (String2))
#define bindtextdomain(Domain,Directory) (Domain) 
#define bind_textdomain_codeset(Domain,Codeset)
#endif

/* not really I18N-related, but also a string marker macro */
#define I_(string) g_intern_static_string (string)

#endif /* __GLIBINTL_H__ */
