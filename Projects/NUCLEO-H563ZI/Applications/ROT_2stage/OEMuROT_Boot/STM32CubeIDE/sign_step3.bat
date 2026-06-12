::=================================================================================================
:: This set of signing scripts is intended for OEMuROT binary signing using  external tool
:: The signing will be split into 3 parts
:: Part1: Use imgtool to generarte HASH digest (which will be signed using external tool)
:: Part2: Sign the digest with external tool and do base64 encoding of the signature binary
:: Part3: Use imgtool to generate the final image using the signature signed by external tool
::=================================================================================================
REM Run script for "OEMuROT Signing Part 3"
::=================================================================================================
@ECHO OFF
:: arg1 is the binary type (nonsecure, secure)
:: When script is called from STM32CubeIDE : set signing="%1"
:: When script is called from IAR or KEIL  : set "signing=%1"
set "signing=%1"

:: Getting the Trusted Package Creator CLI path
set "projectdir=%~dp0"
pushd %projectdir%\..\..\..\..\ROT_Provisioning
set provisioningdir=%cd%
popd
call "%provisioningdir%\env.bat"

:: Enable delayed expansion
setlocal EnableDelayedExpansion

:: Environment variable for log file
set current_log_file="%projectdir%\sign.log"
echo. > %current_log_file%

set project=OEMiROT
set bootpath=OEMiROT_OEMuROT

::=============================================================================================
::image binary files
::=============================================================================================
set s_code_bin="%projectdir%\..\Binary\OEMuROT_Boot.bin"

::=============================================================================================
::image xml configuration files (after-sign configuration)
::=============================================================================================
set s_code_xml="%provisioningdir%\%bootpath%\Images\%project%_Code_Image.xml"
set s_code_init_xml="%provisioningdir%\%bootpath%\Images\%project%_Code_Init_Image.xml"

::=================================================================================================
:: Variables for image xml configuration(ROT_Provisioning\%bootpath%\Images)
:: relative path from ROT_Provisioning\%bootpath%\Images directory to retrieve binary files
::=================================================================================================
set bin_path_xml_field="%provisioningdir%\..\Applications\ROT_2stage\OEMuROT_Boot\Binary"
set fw_in_bin_xml_field="Firmware binary input file"
set fw_out_bin_xml_field="Image output file"
set fw_digst_out_bin_xml_field="Digest output file"
set fw_signature_xml_filed="Signature file"
set s_app_bin_xml_field="%bin_path_xml_field%\OEMuROT_Boot.bin"
set s_app_enc_sign_bin_xml_field="%bin_path_xml_field%\OEMuROT_Boot_enc_sign.bin"
set s_app_enc_sign_hex_xml_field="%bin_path_xml_field%\OEMuROT_Boot_enc_sign.hex"
set s_app_init_sign_hex_xml_field="%bin_path_xml_field%\OEMuROT_Boot_init_sign.hex"

set s_app_enc_sign_digest_xml_field="%bin_path_xml_field%\OEMuROT_Boot_enc_sign.digest"
set s_app_init_sign_digest_xml_field="%bin_path_xml_field%\OEMuROT_Boot_init_sign.digest"

set s_app_init_sign_digest_file="%bin_path_xml_field%\OEMuROT_Boot_init_sign.digest"
set s_app_enc_sign_digest_file="%bin_path_xml_field%\OEMuROT_Boot_enc_sign.digest"

set s_app_init_sign_sig_file="%bin_path_xml_field%\OEMuROT_Boot_init_sign.sig"
set s_app_init_sign_sig_b64_file="%bin_path_xml_field%\OEMuROT_Boot_init_sign.sig.b64"

set s_app_enc_sign_sig_file="%bin_path_xml_field%\OEMuROT_Boot_enc_sign.sig"
set s_app_enc_sign_sig_b64_file="%bin_path_xml_field%\OEMuROT_Boot_enc_sign.sig.b64"

::Make sure we have a Binary sub-folder in UserApp folder
if not exist "%bin_path_xml_field%" (
mkdir "%bin_path_xml_field%"
)

:start
::=================================================================================================
:: Check if Python V3 is installed
::-------------------------------------------------------------------------------------------------
python --version >nul 2>&1
if %errorlevel% neq 0 (
  echo.
  echo Python installation missing. Refer to Utilities\PC_Software\ROT_AppliConfig\README.md
  echo.
  set "command=Python installation"
  goto :error
)
set "python=python "
:: If found, capture version string removing "Python "
for /f "tokens=2 delims= " %%A in ('python --version 2^>^&1') do (
    set "full_version=%%A"
)
:: extract version details
for /F "tokens=1,2,3 delims=." %%A in ("!full_version!") do (
  set MAJOR_VER=%%A
  set MINOR_VER=%%B
  set PATCH_VER=%%C
)
:: is v3
if not "%MAJOR_VER%" == "3" (
  python3 --version >nul 2>&1
  if !errorlevel! neq 0 (
    echo.
    echo Python installation missing. Refer to Utilities\PC_Software\ROT_AppliConfig\README.md
    echo.
    set "command=Python installation"
    goto :error
  )
  set "python=python3 "
)
::=================================================================================================

:: Environment variable for AppliCfg
set "applicfg=%cube_fw_path%\Utilities\PC_Software\ROT_AppliConfig\AppliCfg.py"

:postbuild
echo Postbuild %signing% image (step3) >> %current_log_file% 2>>&1

::=============================================================================================
::Base64 encode of the signature file
::=============================================================================================
openssl base64 -in %s_app_init_sign_sig_file% -out %s_app_init_sign_sig_b64_file% >> %current_log_file% 2>>&1
openssl base64 -in %s_app_enc_sign_sig_file% -out %s_app_enc_sign_sig_b64_file% >> %current_log_file% 2>>&1

%python%%applicfg% xmlval -v %s_app_enc_sign_bin_xml_field% --string -n %fw_out_bin_xml_field% %s_code_xml% --vb >> %current_log_file% 2>>&1
if !errorlevel! neq 0 goto :error

%python%%applicfg% xmlval -v %s_app_enc_sign_sig_b64_file% --string -n %fw_signature_xml_filed% %s_code_xml% --vb >> %current_log_file% 2>>&1
if !errorlevel! neq 0 goto :error

%stm32tpccli% -pb %s_code_xml% >> %current_log_file% 2>>&1
if !errorlevel! neq 0 goto :error

%python%%applicfg% xmlval -v %s_app_enc_sign_hex_xml_field% --string -n %fw_out_bin_xml_field% %s_code_xml% --vb >> %current_log_file% 2>>&1
if !errorlevel! neq 0 goto :error

%stm32tpccli% -pb %s_code_xml% >> %current_log_file% 2>>&1
if !errorlevel! neq 0 goto :error

%python%%applicfg% xmlval -v %s_app_init_sign_hex_xml_field% --string -n %fw_out_bin_xml_field% %s_code_init_xml% --vb >> %current_log_file% 2>>&1
if !errorlevel! neq 0 goto :error

%python%%applicfg% xmlval -v %s_app_init_sign_sig_b64_file% --string -n %fw_signature_xml_filed% %s_code_init_xml% --vb >> %current_log_file% 2>>&1
if !errorlevel! neq 0 goto :error

%stm32tpccli% -pb %s_code_init_xml% >> %current_log_file% 2>>&1
if !errorlevel! neq 0 goto :error

exit 0

:error
echo.
echo =====
echo ===== Error occurred.
echo ===== See %current_log_file% for details. Then try again.
echo =====
exit 1
