#!/bin/bash
# create multiresolution windows icon, and pixmaps
# variables
ICON_SRC=../../src/qt/res/icons/bitmark.png
ICON_DST=../../src/qt/res/icons/bitmark.ico
ICON_SRC_TEST=../../src/qt/res/icons/bitmark_testnet.png
ICON_DST_TEST=../../src/qt/res/icons/bitmark_testnet.ico
# create base for all mainnet icons
convert ${ICON_SRC} -resize 16x16 ../pixmaps/bitmark16.png
convert ${ICON_SRC} -resize 32x32 ../pixmaps/bitmark32.png
convert ${ICON_SRC} -resize 48x48 bitmark-48.png
convert ${ICON_SRC} -resize 64x64 ../pixmaps/bitmark64.png
convert ${ICON_SRC} -resize 128x128 ../pixmaps/bitmark128.png
convert ${ICON_SRC} -resize 256x256 ../pixmaps/bitmark256.png
# create mainnet windows icon with all variant sizes
convert ../pixmaps/bitmark16.png ../pixmaps/bitmark32.png bitmark-48.png ../pixmaps/bitmark64.png ../pixmaps/bitmark128.png ../pixmaps/bitmark256.png ${ICON_DST}
# create base for all testnet icons 
convert ${ICON_SRC_TEST} -resize 16x16 bitmark-test-16.png
convert ${ICON_SRC_TEST} -resize 32x32 bitmark-test-32.png
convert ${ICON_SRC_TEST} -resize 48x48 bitmark-test-48.png
convert ${ICON_SRC_TEST} -resize 64x64 bitmark-test-64.png
convert ${ICON_SRC_TEST} -resize 128x128 bitmark-test-128.png
convert ${ICON_SRC_TEST} -resize 256x256 bitmark-test-256.png
# create testnet windows icon with all varient sizes
convert bitmark-test-16.png bitmark-test-32.png bitmark-test-48.png bitmark-test-64.png bitmark-test-128.png bitmark-test-256.png ${ICON_DST_TEST}
# create pixmap ico and xpm files
convert ../pixmaps/bitmark16.png ../pixmaps/favicon.ico
convert ../pixmaps/bitmark64.png ../pixmaps/bitmark.ico
convert ../pixmaps/bitmark16.png ../pixmaps/bitmark16.xpm
convert ../pixmaps/bitmark32.png ../pixmaps/bitmark32.xpm
convert ../pixmaps/bitmark64.png ../pixmaps/bitmark64.xpm
convert ../pixmaps/bitmark128.png ../pixmaps/bitmark128.xpm
convert ../pixmaps/bitmark256.png ../pixmaps/bitmark256.xpm
# clean up
rm bitmark-48.png
rm bitmark-test-*.png