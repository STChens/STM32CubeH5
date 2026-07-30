iccarm.exe --cpu=Cortex-M33 -D%2 -DBL2 -I%1\..\..\OEMiROT_Boot\Inc\ %1\stm32h563xx_flash.icf --silent --preprocess=ns %1\flash.icf.i >> %1\output.txt 2>&1
