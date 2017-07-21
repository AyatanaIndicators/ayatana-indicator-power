#!/bin/bash

set -x

GETTEXT_DOMAIN=$(cat CMakeLists.txt | grep 'set.*(.*GETTEXT_PACKAGE' | sed -r -e 's/.*\"([^"]+)\"\)/\1/')

cd po/
cat LINGUAS | while read lingua; do touch ${lingua}.po; intltool-update --gettext-package ${GETTEXT_DOMAIN} $(basename ${lingua}); done
cd - 1>/dev/null
