#!/bin/sh

JAVABIN=$(which java 2> /dev/null)
HASJAVA="$?"

minify() {
  TYPE="$1"
  SRC="$2"
  DST="$3"
  ERROR="1"

  if [ "$DST" -nt "$SRC" ]
  then
    return
  fi

  if [ "$TYPE" = "html" ]
  then
    perl -pe 's/^\s*//gm; s/\s*$//gm' $SRC > $DST
    ERROR="$?"
  elif [ "$TYPE" = "js" ] && [ "$HASJAVA" = "0" ]
  then
    $JAVABIN -jar dist/buildtools/closure-compiler.jar $SRC > $DST
    ERROR="$?"
  elif [ "$TYPE" = "css" ] && [ "$HASJAVA" = "0" ]
  then
    $JAVABIN -jar dist/buildtools/closure-stylesheets.jar --allow-unrecognized-properties $SRC > $DST
    ERROR="$?"
  else
    ERROR="1"
  fi

  if [ "$ERROR" = "1" ]
  then
    echo "Error minifying $SRC, copy $SRC to $DST"
    cp $SRC $DST
  fi
}

echo "Minifying javascript"
minify js htdocs/js/mympd.js dist/htdocs/js/mympd.min.js
minify js htdocs/sw.js dist/htdocs/sw.min.js
minify js htdocs/js/keymap.js dist/htdocs/js/keymap.min.js
minify js dist/htdocs/js/bootstrap-native-v4.js dist/htdocs/js/bootstrap-native-v4.min.js

echo "Minifying stylesheets"
minify css htdocs/css/mympd.css dist/htdocs/css/mympd.min.css

echo "Minifying html"
minify html htdocs/index.html dist/htdocs/index.html

echo "Compiling and installing mympd"
install -d release
cd release
export INSTALL_PREFIX="${MYMPD_INSTALL_PREFIX:-/usr}"
cmake -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_PREFIX} -DCMAKE_BUILD_TYPE=RELEASE ..
make
if [ "$DOCKER" = "true" ]
then
  # Container build
  make install DESTDIR=$DESTDIR
  cd ..
else
  sudo -E make install
  cd ..
  sudo -E debian/postinst 
fi
