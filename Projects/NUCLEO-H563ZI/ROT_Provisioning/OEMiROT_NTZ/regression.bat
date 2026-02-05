@ECHO OFF
:: Getting the CubeProgammer_cli path
call ../env_ntz.bat

%stm32programmercli% -c port=swd pwd=..\DA\Binary\password.bin debugauth=1
pause