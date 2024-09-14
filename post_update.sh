#!/bin/bash

set -e

T="${ANDROID_BUILD_TOP}"
cd $(dirname "$0")

source ${T}/build/envsetup.sh

CONFIGURE_ARGS=(
  --enable-ipv6
  --without-ftp
  --without-http
  --without-html
  --without-legacy
  --without-iconv
  --without-zlib
  --without-lzma
)

# Show the commands on the terminal.
set -x

./autogen.sh
./configure "${CONFIGURE_ARGS[@]}"

make
