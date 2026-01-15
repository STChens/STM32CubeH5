#!/bin/sh

i=0
if [ -z $1 ];then
	echo please specify sha mode [224] [256] [384] [512]
else
	echo " " > sha$1.out

while [ $i -le 128 ]; do
	size=$((65536+i))
	openssl dgst -sha$1 $size.bin  >> sha$1.out
	i=$((i+1))
done
fi
