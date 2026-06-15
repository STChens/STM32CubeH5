@ECHO OFF

:: Getting the CubeProgammer_cli path
set imgtool="C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\Utilities\Windows\imgtool.exe"

::Keys files
set "projectdir=%~dp0"
pushd %projectdir%\Keys
set keys_pem_dir=%cd%
popd

:: OEMuROT pubkey c file
pushd %projectdir%\..\..\Applications\ROT_2stage\OEMuROT_Boot\Inc
set oemurot_keys_c_dir=%cd%
popd
set "oemurot_pubkey_c=%oemurot_keys_c_dir%\csk_pubkey.h"


:cont
@set cnt=0

:: pub key of CSK to be included in OEMuROT code which will be authenticated by OEMiROT
set "oemurot_key_ecc_enc_pub=%keys_pem_dir%\OEMuRoT_Authentication_S_pub.pem"
echo /* OEMuROT public key data */  > %oemurot_pubkey_c%
set "command_key=%imgtool% getpub  -k %oemurot_key_ecc_enc_pub%  >> %oemurot_pubkey_c%"
%command_key%
IF %errorlevel% NEQ 0 goto :error_key

echo Script success!
cmd /k
exit 0

:error_key
echo %errorlevel%
echo "%command_key% : failed"
echo Script failure
cmd /k
exit 1
