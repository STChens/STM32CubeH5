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
set "oemirot_bin_file=%projectdir%\..\..\OEMiROT_Boot\Binary\OEMiROT_Boot.bin"
set "boot_bin_file=%projectdir%\..\..\OEMiROT_Boot\Binary\Boot.bin"
set "loader_bin_file=%projectdir%\..\Binary\Loader.bin"

echo %oemirot_bin_file% >> %current_log_file% 2>>&1
echo %boot_bin_file% >> %current_log_file% 2>>&1
echo %loader_bin_file% >> %current_log_file% 2>>&1
IF exist %boot_bin_file% (
%python%%applicfg% oneimage -fb %boot_bin_file% -o 0x10000 -sb %loader_bin_file% -i 0x0 -ob %oemirot_bin_file% --vb >> %current_log_file% 2>>&1
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

