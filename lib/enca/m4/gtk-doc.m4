## Gtk-doc tests.  This file is in public domain.

## Check for gtk-doc.
## Defines:
## HTML_DIR to direcotry to install HTML docs into
## GTKDOC to whether gtkdoc-mkdb is found
## ENABLE_GTK_DOC to yes when gtk-doc should be enabled
AC_DEFUN([gtk_CHECK_GTK_DOC],
[dnl

AC_ARG_WITH(html-dir, [  --with-html-dir=PATH    path to installed docs @<:@DATADIR/gtk-doc/html@:>@])

if test "x$with_html_dir" = "x" ; then
  HTML_DIR=$datadir/gtk-doc/html
else
  HTML_DIR=$with_html_dir
fi

AC_SUBST(HTML_DIR)

AC_CHECK_PROG(GTKDOC, gtkdoc-mkdb, true, false)

gtk_doc_min_version=1.0
if $GTKDOC ; then 
    gtk_doc_version=`gtkdoc-mkdb --version`
    AC_MSG_CHECKING([gtk-doc version ($gtk_doc_version) >= $gtk_doc_min_version])
    if perl <<EOF ; then
      exit (("$gtk_doc_version" =~ /^[[0-9]]+\.[[0-9]]+$/) &&
            ("$gtk_doc_version" >= "$gtk_doc_min_version") ? 0 : 1);
EOF
      AC_MSG_RESULT(yes)
   else
      AC_MSG_RESULT(no)
      GTKDOC=false
   fi
fi

dnl Let people disable the gtk-doc stuff.
AC_ARG_ENABLE(gtk-doc, [  --enable-gtk-doc        use gtk-doc to build documentation @<:@auto@:>@], enable_gtk_doc="$enableval", enable_gtk_doc=auto)

if test x$enable_gtk_doc = xauto ; then
  if test x$GTKDOC = xtrue ; then
    enable_gtk_doc=yes
  else
    enable_gtk_doc=no 
  fi
fi

AM_CONDITIONAL([ENABLE_GTK_DOC], test x$enable_gtk_doc = xyes)
])
