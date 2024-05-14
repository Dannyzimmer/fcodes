#! /bin/sh

GRAPHVIZ_VERSION_MAJOR=$( python3 gen_version.py --major )
GRAPHVIZ_VERSION_MINOR=$( python3 gen_version.py --minor )
GRAPHVIZ_VERSION_PATCH=$( python3 gen_version.py --patch )

GRAPHVIZ_VERSION_DATE=$( python3 gen_version.py --committer-date-graphviz )
GRAPHVIZ_CHANGE_DATE=$( python3 gen_version.py --committer-date-changelog )

if [ "$GRAPHVIZ_VERSION_DATE" = "0" ]; then
    echo "Warning: build not started in a Git clone, or Git is not installed: setting version date to 0." >&2
else
    echo "Graphviz: version date is based on time of last commit: $GRAPHVIZ_VERSION_DATE"
fi

# initialize version for a "development" build
cat >./version.m4 <<EOF
dnl Graphviz package version number, (as distinct from shared library version)

m4_define([graphviz_version_major],[$GRAPHVIZ_VERSION_MAJOR])
m4_define([graphviz_version_minor],[$GRAPHVIZ_VERSION_MINOR])
m4_define([graphviz_version_micro],[$GRAPHVIZ_VERSION_PATCH])

m4_define([graphviz_version_date],[$GRAPHVIZ_VERSION_DATE])
m4_define([graphviz_change_date],["$GRAPHVIZ_CHANGE_DATE"])
m4_define([graphviz_author_name],["$GRAPHVIZ_AUTHOR_NAME"])
m4_define([graphviz_author_email],[$GRAPHVIZ_AUTHOR_EMAIL])

EOF

# config/missing is created by autoreconf,  but apparently not recreated if already there.
# This breaks some builds from the graphviz.tar.gz sources.
# Arguably this is an autoconf bug.
rm -f config/missing

autoreconf -v --install --force || exit 1

# ensure config/depcomp exists even if still using automake-1.4
# otherwise "make dist" fails.
touch config/depcomp
