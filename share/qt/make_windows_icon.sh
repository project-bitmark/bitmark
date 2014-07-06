#!/bin/bash
# create multiresolution windows icon, and pixmaps
ICON_SRC=../../src/qt/res/icons/bitmark.png
ICON_DST=../../src/qt/res/icons/bitmark.ico
ICON_SRC_TEST=../../src/qt/res/icons/bitmark_testnet.png
ICON_DST_TEST=../../src/qt/res/icons/bitmark_testnet.ico
convert ${ICON_SRC} -resize 16x16 ../pixmaps/bitmark16.png
convert ${ICON_SRC} -resize 32x32 ../pixmaps/bitmark32.png
convert ${ICON_SRC} -resize 48x48 bitmark-48.png
convert ${ICON_SRC} -resize 64x64 ../pixmaps/bitmark64.png
convert ${ICON_SRC} -resize 128x128 ../pixmaps/bitmark128.png
convert ${ICON_SRC} -resize 256x256 ../pixmaps/bitmark256.png
convert ../pixmaps/bitmark16.png ../pixmaps/bitmark32.png bitmark-48.png ${ICON_DST}
convert ${ICON_SRC_TEST} -resize 16x16 bitmark-test-16.png
convert ${ICON_SRC_TEST} -resize 32x32 bitmark-test-32.png
convert ${ICON_SRC_TEST} -resize 48x48 bitmark-test-48.png
convert bitmark-test-16.png bitmark-test-32.png bitmark-test-48.png ${ICON_DST_TEST}
convert ../pixmaps/bitmark16.png ../pixmaps/favicon.ico
convert ../pixmaps/bitmark64.png ../pixmaps/bitmark.ico
convert ../pixmaps/bitmark16.png ../pixmaps/bitmark16.xpm
convert ../pixmaps/bitmark32.png ../pixmaps/bitmark32.xpm
convert ../pixmaps/bitmark64.png ../pixmaps/bitmark64.xpm
convert ../pixmaps/bitmark128.png ../pixmaps/bitmark128.xpm
convert ../pixmaps/bitmark256.png ../pixmaps/bitmark256.xpm
rm bitmark-48.png
rm bitmark-test-16.png
rm bitmark-test-32.png
rm bitmark-test-48.png