call ../env_ntz.bat
:: Get config updated by OEMiROT_Boot
call img_config.bat
set rot_provisioning_path=%rot_provisioning_path:"=%
set cube_fw_path=%cube_fw_path:"=%

:: Enable delayed expansion
setlocal EnableDelayedExpansion

set wrpgrp1=0xfffffff8
set wrpgrp2=0x7fffffff
set hdp1_start=0
set hdp1_end=0x13
set hdp2_start=0x7F
set hdp2_end=0x0
set boot_lck=0xB4
set bootaddress=0x8000000
set loaderaddress=0x81F8000

set bootob=0x80000

set bootpath=OEMiROT_NTZ

set app_code_image=%oemirot_appli_sign%
set app_data_image=app_data_init_sign.hex

set connect_no_reset=-c port=SWD speed=fast ap=1 mode=Hotplug
set connect_reset=-c port=SWD speed=fast ap=1 mode=UR

if "%isGeneratedByCubeMX%" == "true" (
set appli_dir=%oemirot_appli_path_project%
) else (
set appli_dir=../../%oemirot_appli_path_project%
)

:: =============================================== Remove protections and initialize Option Bytes ==========================================
set remove_protect_init=-ob WRPSGn1=0xffffffff WRPSGn2=0xffffffff HDP1_STRT=1 HDP1_END=0 HDP2_STRT=1 HDP2_END=0 NSBOOT_LOCK=0xC3 SWAP_BANK=0 SRAM2_RST=0 SRAM2_ECC=0 BOOT_UBE=0xB4

:: =============================================== Erase the user flash =====================================================================
set erase_all=-e all

:: =============================================== Hardening ===============================================================================
set hide_protect=HDP1_STRT=%hdp1_start% HDP1_END=%hdp1_end% HDP2_STRT=%hdp2_start% HDP2_END=%hdp2_end%
set write_protect=WRPSGn1=%wrpgrp1% WRPSGn2=%wrpgrp2%
set boot_lock=NSBOOT_LOCK=%boot_lck%

:: =============================================== Configure Option Bytes ====================================================================
set "action=Set TZEN = 0"
echo %action%
:: Trust zone enabled is mandatory in order to execute OEM-iRoT
%stm32programmercli% %connect_no_reset% -ob TZEN=0xC3
IF !errorlevel! NEQ 0 goto :error

set "action=Remove Protection and erase All"
echo %action%
%stm32programmercli% %connect_reset%
%stm32programmercli% %connect_reset% %remove_protect_init% %erase_all%
IF !errorlevel! NEQ 0 goto :error

set "action=Set NSBoot address"
echo %action%
%stm32programmercli% %connect_reset%
%stm32programmercli% %connect_reset% -ob NSBOOTADD=%bootob%
IF !errorlevel! NEQ 0 goto :error

:: ==================================================== Download images ====================================================================

echo "Application images programming in download slots"

set "action=Write Appli"
echo %action%
%stm32programmercli% %connect_no_reset% -d %appli_dir%\Binary\%app_code_image% -v
IF !errorlevel! NEQ 0 goto :error
echo "Appli Written"

if  "%app_data_image_number%" == "1" (
set "action=Write App Data"
echo %action%
IF not exist %rot_provisioning_path%\%bootpath%\Binary\%app_data_image% (
@echo [31mError: %rot_provisioning_path%\%bootpath%\Binary\%app_data_image% does not exist! use TPC to generate it[0m
goto :error
)
%stm32programmercli% %connect_no_reset% -d %rot_provisioning_path%\%bootpath%\Binary\%app_data_image% -v
IF !errorlevel! NEQ 0 goto :error
)

if  "%ext_loader%" == "1" (
set "action=Write OEMiROT_Loader"
echo %action%
%stm32programmercli% %connect_no_reset% -d %cube_fw_path%\Projects\NUCLEO-H563ZI\%oemirot_boot_path_project%\..\OEMiROT_Loader\Binary\OEMiROT_Loader.bin %loaderaddress% -v
IF !errorlevel! NEQ 0 goto :error
echo "OEMiROT_Loader Written"
)

set "action=Write OEMiROT_Boot"
echo %action%
%stm32programmercli% %connect_no_reset% -d %cube_fw_path%\Projects\NUCLEO-H563ZI\%oemirot_boot_path_project%\Binary\OEMiROT_Boot.bin %bootaddress% -v
IF !errorlevel! NEQ 0 goto :error
echo "OEMiROT_Boot Written"

:: ======================================================= Extra board protections =========================================================
set "action=Configure Option Bytes"
echo %action%
echo "Configure Secure option Bytes: Write Protection, Hide Protection and boot lock"
%stm32programmercli% %connect_no_reset% -ob %write_protect% %hide_protect% %boot_lock%
IF !errorlevel! NEQ 0 goto :error

echo Programming success
IF [%1] NEQ [AUTO] cmd /k
exit 0

:error
echo      Error when trying to "%action%" >CON
echo      Programming aborted >CON
echo.
IF [%1] NEQ [AUTO] cmd /k
exit 1
