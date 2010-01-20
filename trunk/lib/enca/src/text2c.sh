#! /bin/sh
name=`echo $1 | sed -e 's/.*\///'`
cat <<EOF
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */
#include "common.h"
EOF
echo "const char *const "$name"_text[] = {"
sed -e 's/[\"]/\\&/g' -e 's/.*/  "&",/' $1
echo "  NULL"
echo "};"
