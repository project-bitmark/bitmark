#!/bin/bash
# create multiresolution windows icon, and pixmaps
ICON_SRC=../../src/qt/res/icons/bitmark.png
ICON_DST=../../src/qt/res/icons/bitmark.ico
convert ${ICON_SRC} -resize 16x16 ../pixmaps/bitmark16.png
convert ${ICON_SRC} -resize 32x32 ../pixmaps/bitmark32.png
convert ${ICON_SRC} -resize 48x48 bitmark-48.png
convert ${ICON_SRC} -resize 68x68 ../pixmaps/bitmark64.png
convert ${ICON_SRC} -resize 128x128 ../pixmaps/bitmark128.png
convert ${ICON_SRC} -resize 48x48 bitmark-48.png
convert ${ICON_SRC} -resize 48x48 bitmark-48.png
convert ../pixmaps/bitmark16.png ../pixmaps/bitmark32.png bitmark-48.png ${ICON_DST}
convert ../pixmaps/bitmark16.png ../pixmaps/favicon.ico
convert ../pixmaps/bitmark64.png ../pixmaps/bitmark.ico
convert ../pixmaps/bitmark16.png ../pixmaps/bitmark16.xpm
convert ../pixmaps/bitmark32.png ../pixmaps/bitmark32.xpm
convert ../pixmaps/bitmark64.png ../pixmaps/bitmark64.xpm
convert ../pixmaps/bitmark128.png ../pixmaps/bitmark128.xpm
convert ../pixmaps/bitmark256.png ../pixmaps/bitmark256.xpm
rm bitmark-48.png