@ECHO OFF
:: Getting the Trusted Package Creator and STM32CubeProgammer CLI path
set "projectdir=%~dp0"
pushd %projectdir%\..\..\..\..\ROT_Provisioning
set provisioningdir=%cd%
popd
call "%provisioningdir%\env.bat"
:: Enable delayed expansion
setlocal EnableDelayedExpansion

:: Environment variable for log file
set current_log_file="%projectdir%\postbuild.log"
echo. > %current_log_file%

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
set "preprocess_bl2_file=%projectdir%\image_macros_preprocessed_bl2.c"
set "oemurot_dir=%projectdir%/../../../../%oemirot_oemurot_boot_path_stage2_project%"
set "loader_dir=%projectdir%/../../../../%oemirot_oemurot_boot_path_loader_project%"
set "ob_flash_programming_script=%projectdir%\..\..\..\..\ROT_Provisioning\OEMiROT_OEMuROT\ob_flash_programming.bat"


set "img_config=%projectdir%\..\..\..\..\ROT_Provisioning\OEMiROT_OEMuROT\img_config.bat"
set oemurot_flash_layout="%oemurot_dir%\Inc\flash_layout.h"
set oemurot_sign_script="%oemurot_dir%\MDK-ARM\sign.bat"
set loader_flash_layout="%loader_dir%\Inc\appli_flash_layout.h"
set loader_sct_file="%loader_dir%\MDK-ARM\stm32h563xx.sct"
set "map_properties=%projectdir%\..\..\OEMiROT_Boot\map.properties"

::======================================================================================
::image xml configuration files
::======================================================================================
set s_code_xml="%projectdir%\..\..\..\..\ROT_Provisioning\OEMiROT_OEMuROT\Images\OEMiROT_Code_Image.xml"
set s_code_init_xml="%projectdir%\..\..\..\..\ROT_Provisioning\OEMiROT_OEMuROT\Images\OEMiROT_Code_Init_Image.xml"
set auth_s="Authentication secure key"
set auth_ns="Authentication non secure key"
set xml_fw_app_item_name="Firmware binary input file"
set xml_output_item_name="Image output file"
set xml_enc_item_name="Encryption key"
set code_size="Firmware area size"
set data_size="Data download slot size"
set scratch_sector_number="Number of scratch sectors"
set firmware_execution_offset="Firmware execution area offset"

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b S_CODE_REGION_START -m RE_ADDRESS_SECURE_START %map_properties% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b S_CODE_REGION_SIZE -m RE_IMAGE_FLASH_SECURE_IMAGE_SIZE %map_properties% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b CODE_IMAGE_ASSEMBLY -m RE_CODE_IMAGE_ASSEMBLY %map_properties% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b MCUBOOT_OVERWRITE_ONLY -m RE_OVER_WRITE %map_properties% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b CMSE_VENEER_REGION_SIZE -m RE_CMSE_VENEER_REGION_SIZE %map_properties% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b ROT_REGION_START -m RE_FLASH_AREA_BL2_OFFSET %map_properties% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b ROT_REGION_SIZE -m RE_FLASH_AREA_BL2_SIZE %map_properties% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b SCRATCH_REGION_START -m RE_FLASH_AREA_SCRATCH_OFFSET %map_properties% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b SCRATCH_REGION_SIZE -m RE_FLASH_AREA_SCRATCH_SIZE %map_properties% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b DOWNLOAD_S_CODE_REGION_START -m RE_AREA_2_OFFSET %map_properties% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b bootob -m RE_BL2_BOOT_ADDRESS  -d 0x100 %ob_flash_programming_script% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% flash --layout %preprocess_bl2_file%  -b bootaddress -m RE_BL2_BOOT_ADDRESS %ob_flash_programming_script% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% setob --layout %preprocess_bl2_file% -b wrpgrp1 -ms RE_BL2_WRP_START -me RE_BL2_WRP_END -msec RE_FLASH_PAGE_NBR -d 0x8000 %ob_flash_programming_script% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% setob --layout %preprocess_bl2_file% -b wrpgrp2 -ms RE_BL2_WRP_START -me RE_BL2_WRP_END -msec RE_FLASH_PAGE_NBR -d 0x8000 %ob_flash_programming_script% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% setob --layout %preprocess_bl2_file% -b hdp1_end -ms RE_BL2_HDP_START -me RE_BL2_HDP_END -msec RE_FLASH_PAGE_NBR -d 0x2000 %ob_flash_programming_script% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% setob --layout %preprocess_bl2_file% -b hdp2_start -ms RE_BL2_HDP_START -me RE_BL2_HDP_END -msec RE_FLASH_PAGE_NBR -d 0x2000 %ob_flash_programming_script% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% setob --layout %preprocess_bl2_file% -b hdp2_end -ms RE_BL2_HDP_START -me RE_BL2_HDP_END -msec RE_FLASH_PAGE_NBR -d 0x2000 %ob_flash_programming_script% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% xmlval --layout %preprocess_bl2_file% -m RE_IMAGE_FLASH_SECURE_UPDATE -c x %s_code_xml% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% xmlval --layout %preprocess_bl2_file% -m RE_IMAGE_FLASH_SECURE_IMAGE_SIZE -c S %s_code_xml% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% xmlparam --layout  %preprocess_bl2_file% -m RE_ENCRYPTION -n "Encryption key" -link GetPublic -t File -c -E -h 1 -d "" %s_code_xml% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% xmlparam --layout  %preprocess_bl2_file% -m RE_OVER_WRITE -n "Write Option" -t Data -c --overwrite-only -h 1 -d "" %s_code_xml% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error


:end
set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b FLASH_SIZE -m RE_FLASH_SIZE %map_properties% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% xmlval --layout %preprocess_bl2_file% -m RE_FLASH_AREA_SCRATCH_SIZE -n %scratch_sector_number% --decimal %s_code_xml% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% xmlval -xml %s_code_xml% -nxml %code_size% -nxml %scratch_sector_number% --decimal -e (((val1+1)/val2)+1) -cond val2 -c M %s_code_xml% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

::xml for init image generation

set "command=%python%%applicfg% xmlval --layout %preprocess_bl2_file% -m RE_IMAGE_FLASH_SECURE_IMAGE_SIZE -c S %s_code_init_xml% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% xmlparam --option add -n "Clear" -t Data -c -c -h 1 -d "" %s_code_init_xml% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% xmlparam --option add -n "Confirm" -t Data -c --confirm -h 1 -d "" %s_code_init_xml% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% xmlname -n %firmware_execution_offset%  -c x %s_code_init_xml% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% xmlval --layout %preprocess_bl2_file% -m RE_IMAGE_FLASH_ADDRESS_SECURE -c x %s_code_init_xml% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

:: update loader linker file  
set "command=%python%%applicfg% linker --layout %preprocess_bl2_file% -m RE_FLASH_LOADER_START -n LOADER_CODE_START %loader_sct_file% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% linker --layout %preprocess_bl2_file% -m RE_FLASH_LOADER_SIZE -n LOADER_CODE_SIZE %loader_sct_file% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

:: update loader flash layout file  (OEMuROT primary and secondary slot areas)
:: RE_AREA_0_OFFSET ==> OEMuROT_IMAGE_PRIMARY_PARTITION_OFFSET
set "command=%python%%applicfg% definevalue --layout %preprocess_bl2_file% -m RE_AREA_0_OFFSET -n OEMuROT_IMAGE_PRIMARY_PARTITION_OFFSET %loader_flash_layout% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error
:: RE_AREA_2_OFFSET ==> OEMuROT_IMAGE_SECONDARY_PARTITION_OFFSET
set "command=%python%%applicfg% definevalue --layout %preprocess_bl2_file% -m RE_AREA_2_OFFSET -n OEMuROT_IMAGE_SECONDARY_PARTITION_OFFSET %loader_flash_layout% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error
:: RE_AREA_0_SIZE ==> OEMuROT_IMAGE_PARTITION_SIZE
set "command=%python%%applicfg% definevalue --layout %preprocess_bl2_file% -m RE_AREA_0_SIZE -n OEMuROT_IMAGE_PARTITION_SIZE %loader_flash_layout% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

:: update oemurot flash_layout file 
:: OEMuROT FLASH_AREA_BL2_OFFSET shall be after OEMiROT (same as OEMiROT RE_AREA_0_OFFSET)
set "command=%python%%applicfg% definevalue --layout %preprocess_bl2_file% -m RE_AREA_0_OFFSET -n FLASH_AREA_BL2_OFFSET %oemurot_flash_layout% --vb  >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% definevalue --layout %preprocess_bl2_file% -m RE_AREA_0_SIZE -n FLASH_AREA_BL2_SIZE %oemurot_flash_layout% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% definevalue --layout %preprocess_bl2_file% -m RE_FLASH_AREA_SCRATCH_OFFSET -n FLASH_AREA_SCRATCH_OEMIROT_OFFSET %oemurot_flash_layout% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% definevalue --layout %preprocess_bl2_file% -m RE_FLASH_AREA_SCRATCH_SIZE -n FLASH_AREA_SCRATCH_OEMIROT_SIZE %oemurot_flash_layout% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

set "command=%python%%applicfg% definevalue --layout %preprocess_bl2_file% -m RE_FLASH_LOADER_SIZE -n FLASH_AREA_LOADER_SIZE %oemurot_flash_layout% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

:: update img_config script for loader image number
set "command=%python%%applicfg% flash --layout %preprocess_bl2_file% -b loader_image_number -m RE_LOADER_IMG_NUMBER --decimal %img_config% --vb >> %current_log_file% 2>&1"
%command%
IF !errorlevel! NEQ 0 goto :error

ECHO comd : %command%

exit 0

:error
echo.
echo =====
echo ===== Error occurred.
echo ===== See %current_log_file% for details. Then try again.
echo =====
exit 1

