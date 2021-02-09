#!/bin/bash -e
event_name=$1 # ${{ github.event_name }}
ref=$2 # ${{ github.ref }}

if [ "$event_name" = "push" ]; then
  if [[ $ref =~ ^refs/tags/gpsbabel_([0-9]+)_([0-9]+)_([0-9]+)$ ]]; then
    # this is the tag of a release, extract the version number from the tag
    GPSBABEL_VERSION=${BASH_REMATCH[1]}.${BASH_REMATCH[2]}.${BASH_REMATCH[3]}
    echo GPSBABEL_VERSION="${GPSBABEL_VERSION}"
  fi
fi
if [ -z "$GPSBABEL_VERSION" ]; then
  # this is not the tag of a release, create a suffix indicating a development release
  if [ "$(uname -s)" = "Darwin" ]; then
#   on macOS date is not gnu date.
    GPSBABEL_PACKAGE_RELEASE=-dev-$(date -ju -f %Y-%m-%dT%H:%M:%S%z "$(git show -s --format="%aI" HEAD | sed 's/:\(..\)$/\1/')" +%Y%m%dT%H%MZ)-$(git rev-parse --short=7 HEAD)
  else
    GPSBABEL_PACKAGE_RELEASE=-dev-$(date -u --date="$(git show -s --format="%aI" HEAD)" +%Y%m%dT%H%MZ)-$(git rev-parse --short=7 HEAD)
  fi
  echo GPSBABEL_PACKAGE_RELEASE="${GPSBABEL_PACKAGE_RELEASE}"
fi

