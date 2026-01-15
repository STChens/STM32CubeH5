#!/bin/sh

i=1

while [ $i -le 128 ]; do
	size=$((65536+i))
	/c/Program\ Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe -c port=swd -r 0x08100000 $size $size.bin
	i=$((i+1))
done
