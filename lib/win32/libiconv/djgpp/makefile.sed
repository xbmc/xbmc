# Fixes for lib/Makefile.in
s|encodings\.def|encodings/&|g
s|encodings_aix\.def|encodings/aix.def|g
s|encodings_dos\.def|encodings/dos\.def|g
s|encodings_extra\.def|encodings/extra\.def|g
s|encodings_osf1\.def|encodings/osf1\.def|g
s|encodings_local\.def|encodings/local\.def|g
s|aliases\.h|aliases/&|g
s|aliases2\.h|aliases/aliases2.h|g
s|aliases_aix\.h|aliases/aix.h|g
s|aliases_dos\.h|aliases/dos\.h|g
s|aliases_extra\.h|aliases/extra\.h|g
s|aliases_osf1\.h|aliases/osf1\.h|g
s|aliases_local\.h|aliases/local\.h|g


# Fixes for tests/Makefile.in
s|\$(srcdir)/check-translitfailure|$(SHELL) $(srcdir)/failuretranslit-check|
s|\$(srcdir)/check-stateless|$(SHELL) $(srcdir)/stateless-check|
s|\$(srcdir)/check-stateful|$(SHELL) $(srcdir)/stateful-check|
s|\$(srcdir)/check-translit|$(SHELL) $(srcdir)/translit-check|
