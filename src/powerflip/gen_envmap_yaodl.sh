#!/bin/sh
#
# This script takes as comandline argments a yaodl file and an environment map
# image file and writes on standard out a new yaodl file which is the original 
# yaodl file and environment map applied.
#

if [ $# != 3 ]
then

	echo 'Usage: ' $0 '{LINEAR|CONTOUR|SPHEREMAP} <Yaodle file> <texture file>' >2
	exit 1

fi

GEN_TYPE=$1
YAODL=$2
TEXTURE=$3


echo '(group'

cat $YAODL

echo '  : '
echo '  (textures "'$TEXTURE'"),'

case $GEN_TYPE in 
    LINEAR)
	echo '  (sContours "linear", "object", 10.0 0.0 10.0 0.0),'
	echo '  (tContours "linear", "object", -7.0 7.0 7.0 0.0),'
	;;
    CONTOUR)
	echo '  (sContours "contour", "eye", 5.0 0.0 5.0 0.0),'
	echo '  (tContours "contour", "eye", 0.0 5.0 -5.0 0.0),'
	;;
    SPHEREMAP)
	echo '  (sContours "spheremap", "object"),'
	echo '  (tContours "spheremap", "object"),'
	;;
    *)
	echo $0": One of {LINEAR|CONTOUR|SPHEREMAP} expected as first argument" >2
	exit 1
	;;
esac

echo ')'
