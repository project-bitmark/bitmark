#!/bin/bash
# create multiresolution windows icon
ICON_SRC=../../src/qt/res/icons/bitmark.png
ICON_DST=../../src/qt/res/icons/bitmark.ico
convert ${ICON_SRC} -resize 16x16 bitmark-16.png
convert ${ICON_SRC} -resize 32x32 bitmark-32.png
convert ${ICON_SRC} -resize 48x48 bitmark-48.png
convert bitmark-16.png bitmark-32.png bitmark-48.png ${ICON_DST}

