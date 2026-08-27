@ECHO OFF
set "projectdir=%~dp0"
pushd %projectdir%\..\..\..\..\ROT_Provisioning
set provisioningdir=%cd%
popd
call "%provisioningdir%\env.bat"
set rot_provisioning_path=%rot_provisioning_path:"=%
:: Enable delayed expansion
setlocal EnableDelayedExpansion

:: Environment variable for log file
set current_log_file="%projectdir%\postbuild.log"
echo. > %current_log_file%

:start
goto exe:
goto py:
:exe

::======================================================================================
:: Setting Tool path
::======================================================================================
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
set "auto_rot_update=%projectdir%\..\auto_rot_update.bat"
set "preprocess_bl2_file=%projectdir%\..\..\OEMiROT_Boot\MDK-ARM\image_macros_preprocessed_bl2.c"
set "oemirot_bin_file=%projectdir%\..\..\OEMiROT_Boot\Binary\OEMiROT_Boot.bin"
set "boot_bin_file=%projectdir%\..\..\OEMiROT_Boot\Binary\Boot.bin"
set "loader_bin_file=%projectdir%\..\Binary\Loader.bin"

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b oemurot_enable -m RE_OEMUROT_ENABLE --decimal %auto_rot_update% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error
call %auto_rot_update%
echo %oemirot_bin_file% >> %current_log_file% 2>>&1
echo %boot_bin_file% >> %current_log_file% 2>>&1
echo %loader_bin_file% >> %current_log_file% 2>>&1
IF exist %boot_bin_file% (
  if %oemurot_enable% == 1 (
    %python%%applicfg% oneimage -fb %boot_bin_file% -o 0xFC00 -sb %loader_bin_file% -i 0x0 -ob %oemirot_bin_file% --vb >> %current_log_file% 2>>&1
  ) else (
    %python%%applicfg% oneimage -fb %boot_bin_file% -o 0x10000 -sb %loader_bin_file% -i 0x0 -ob %oemirot_bin_file% --vb >> %current_log_file% 2>>&1
  )
)
:end
if %oemurot_enable% == 1 (
  ::update xml file : output file (hex)
  %python%%applicfg% xmlval -v "./../../../Applications/ROT/OEMiROT_Boot/Binary/rot_enc_sign.hex" --string -n "Image output file" %rot_provisioning_path%\STiROT_OEMuROT\Images\STiRoT_Code_Image.xml --vb >> %current_log_file% 2>>&1
  if !errorlevel! neq 0 goto :error

  %stm32tpccli% -pb %rot_provisioning_path%\STiROT_OEMuROT\Images\STiRoT_Code_Image.xml >> %current_log_file% 2>>&1
  IF !errorlevel! NEQ 0 goto :error

  ::update xml file : output file (bin)
  %python%%applicfg% xmlval -v "./../../../Applications/ROT/OEMiROT_Boot/Binary/rot_enc_sign.bin" --string -n "Image output file" %rot_provisioning_path%\STiROT_OEMuROT\Images\STiRoT_Code_Image.xml --vb >> %current_log_file% 2>>&1
  if !errorlevel! neq 0 goto :error

  %stm32tpccli% -pb %rot_provisioning_path%\STiROT_OEMuROT\Images\STiRoT_Code_Image.xml >> %current_log_file% 2>>&1
  IF !errorlevel! NEQ 0 goto :error

  %stm32tpccli% -pb %rot_provisioning_path%\STiROT_OEMuROT\Images\STiRoT_Code_Init_Image.xml >> %current_log_file% 2>>&1
  IF !errorlevel! NEQ 0 goto :error
)

:: ======================================================================= end ========================================================================
exit 0

:error
echo.
echo =====
echo ===== Error occurred.
echo ===== See %current_log_file% for details. Then try again.
echo =====
exit 1

