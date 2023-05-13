#!/bin/bash

genaddrout=
entryaddr=
entryld=

. ${TOPDIR}/build/scripts/parse_options.sh

[ ! -z $genaddrout ] && entryaddr=$($genaddrout)
[ -z $entryaddr ] && "Error --entryaddr nor --genaddrout" && exit 1;
[ -z $entryld ] && "Error --entryld" && exit 1;

echo "__RAM_BASE = $entryaddr;" > $entryld
echo "__ROM_BASE = __RAM_BASE;" >> $entryld
