call ../env.bat

:: Enable delayed expansion
setlocal EnableDelayedExpansion

:: Getting the CubeProgammer_cli path
set connect_no_reset=-c port=SWD speed=fast ap=1 mode=Hotplug
set connect_reset=-c port=SWD speed=fast ap=1 mode=Hotplug -hardRst

:: Local variable to manage alone script execution
set "product_state=unknown"

:: Update local variable thanks to argument use with the command script executed
IF /i [%2] EQU [OPEN] set product_state=open

IF "!product_state!" == "open" (
set "action=Set SECBOOT_LOCK option byte to 0xC3 (unlock secure boot address)"
:: SECBOOT_LOCK reset to 0xC3 value is mandatory to change BOOT_UBE option byte (this is done only in OPEN product_state)
echo %action%
set "cmd=%stm32programmercli% %connect_no_reset% -ob SECBOOT_LOCK=0xC3"
echo %cmd% >> %cubeprog_log%
%cmd%
IF !errorlevel! NEQ 0 goto :error

set "action=Set UBE ==> OEMiRoT (User Flash)"
echo %action%
:: Unique boot entry is set to OEMiRoT (User Flash) to be able to configure OBKeys in product state Open
set "cmd=%stm32programmercli% %connect_no_reset% -ob BOOT_UBE=0xB4"
echo %cmd% >> %cubeprog_log%
%cmd%
IF !errorlevel! NEQ 0 goto :error
)

:: =============================================== Configure OB Keys =========================================================================
set "action=Configure OBKeys HDPL1-DA config area"
echo %action%
set "cmd=%stm32programmercli% %connect_reset%"
echo %cmd% >> %cubeprog_log%
%cmd%

set "cmd=%stm32programmercli% %connect_no_reset% -sdp ./../DA/Binary/DA_Config.obk"
echo %cmd% >> %cubeprog_log%
%cmd%
IF !errorlevel! NEQ 0 goto :error

set "action=Configure OBKeys HDPL1-OEMiRoT config area"
echo %action%
set "cmd=%stm32programmercli% %connect_reset%"
echo %cmd% >> %cubeprog_log%
%cmd%

set "cmd=%stm32programmercli% %connect_no_reset% -sdp ./Binary/OEMiRoT_Config.obk"
echo %cmd% >> %cubeprog_log%
%cmd%

IF !errorlevel! NEQ 0 goto :error

set "action=Configure OBKeys HDPL1-OEMiRoT data area"
echo %action%
set "cmd=%stm32programmercli% %connect_reset%"
echo %cmd% >> %cubeprog_log%
%cmd%

set "cmd=%stm32programmercli% %connect_no_reset% -sdp ./Binary/OEMiRoT_Data.obk"
echo %cmd% >> %cubeprog_log%
%cmd%

IF !errorlevel! NEQ 0 goto :error

set "action=Configure OBKeys HDPL2-OEMuRoT config area"
echo %action%
set "cmd=%stm32programmercli% %connect_reset%"
echo %cmd% >> %cubeprog_log%
%cmd%

set "cmd=%stm32programmercli% %connect_no_reset% -sdp ./Binary/OEMuRoT_Config.obk"
echo %cmd% >> %cubeprog_log%
%cmd%

IF !errorlevel! NEQ 0 goto :error

set "action=Configure OBKeys HDPL2-OEMuRoT data area"
echo %action%
set "cmd=%stm32programmercli% %connect_reset%"
echo %cmd% >> %cubeprog_log%
%cmd%

set "cmd=%stm32programmercli% %connect_no_reset% -sdp ./Binary/OEMuRoT_Data.obk"
echo %cmd% >> %cubeprog_log%
%cmd%

IF !errorlevel! NEQ 0 goto :error

:: =============================================== Boot on OEMiRoT ==========================================================================
IF "!product_state!" == "open" (
set "action=Set UBE ==> OEMiRoT"
echo %action%
:: Unique boot entry is set to OEMiRoT to force OEMiRoT execution at each reset
set "command=%stm32programmercli% %connect_no_reset% -ob BOOT_UBE=0xB4"
echo %command% >> %cubeprog_log%
%command%

IF !errorlevel! NEQ 0 goto :error

set "action=Set SECBOOT_LOCK to 0xB4 (lock secure boot address)"
echo %action%
set "command=%stm32programmercli% %connect_no_reset% -ob SECBOOT_LOCK=0xB4"
echo %command% >> %cubeprog_log%
%command%

IF !errorlevel! NEQ 0 goto :error
)

echo Successful option bytes programming and images flashing
IF [%1] NEQ [AUTO] cmd /k
exit 0

:error
echo        Error when trying to "%action%" >CON
echo        Provisioning aborted >CON
IF [%1] NEQ [AUTO] cmd /k
exit 1
