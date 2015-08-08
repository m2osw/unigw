# S5 3.5.0 Installer (to replace the aging and over-engineered InstallShieldX)
#
# Author: R. Douglas Barbieri, Made to Order Software Corp.
#
!ifndef WPKG_VERSION_MAJOR
   Abort "Major version not set!"
!endif
!ifndef WPKG_VERSION_MINOR
   Abort "Major version not set!"
!endif
!ifndef WPKG_VERSION_PATCH
   Abort "Major version not set!"
!endif

!define VERSION_MAJOR    ${WPKG_VERSION_MAJOR}
!define VERSION_MINOR    ${WPKG_VERSION_MINOR}
!define VERSION_REVISION ${WPKG_VERSION_PATCH}

Name "Windows Packager ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}"
XPStyle on

!include "LogicLib.nsh"
!include "MUI2.nsh"
!include "WinMessages.nsh"
!include "x64.nsh"

!insertmacro MUI_LANGUAGE English

RequestExecutionLevel highest

Function .onInit
   UserInfo::GetAccountType
   pop $0
   ${If} $0 != "admin"
      MessageBox mb_iconstop "Administrator rights required!"
      SetErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
      Quit
   ${Endif}
FunctionEnd

; Installation
;
LangString MUI_TEXT_WELCOME_INFO_TITLE      ${LANG_ENGLISH} "Windows Packager (wpkg) by Made to Order Software, Inc."
LangString MUI_TEXT_WELCOME_INFO_TEXT       ${LANG_ENGLISH} "The Installation Wizard will install the Windows Packager onto your computer. To continue, click 'Install'."
;
LangString MUI_TEXT_INSTALLING_TITLE        ${LANG_ENGLISH} "Installing Windows Packager"
LangString MUI_TEXT_INSTALLING_SUBTITLE     ${LANG_ENGLISH} "Installing the Windows Packager software onto your computer system. Please wait..."
LangString MUI_TEXT_FINISH_TITLE            ${LANG_ENGLISH} "Windows Packager Installation Complete"
LangString MUI_TEXT_FINISH_SUBTITLE         ${LANG_ENGLISH} "The Windows Packager software has been successfully installed onto your computer system."
;
LangString MUI_TEXT_FINISH_INFO_TITLE       ${LANG_ENGLISH} "Windows Packager Installation Complete"
LangString MUI_TEXT_FINISH_INFO_TEXT        ${LANG_ENGLISH} "Your software has been installed!"
LangString MUI_BUTTONTEXT_FINISH            ${LangString}   "Finish"
;
LangString MUI_TEXT_ABORT_TITLE             ${LANG_ENGLISH} "Windows Packager Installation Aborted"
LangString MUI_TEXT_ABORT_SUBTITLE          ${LANG_ENGLISH} "You have aborted the installation of the Windows Packager software onto your computer system."

; Uninstallation
;
LangString MUI_UNTEXT_WELCOME_INFO_TITLE    ${LANG_ENGLISH} "Windows Packager (wpkg) Uninstallation Wizard"
LangString MUI_UNTEXT_WELCOME_INFO_TEXT     ${LANG_ENGLISH} "The Windows Packager Uninstallation Wizard will remove the software onto your computer. To continue, click 'Uninstall'."
;
LangString MUI_UNTEXT_UNINSTALLING_TITLE    ${LANG_ENGLISH} "Windows Packager Uninstallation Underway"
LangString MUI_UNTEXT_UNINSTALLING_SUBTITLE ${LANG_ENGLISH} "Removing the Windows Packager software from your computer system. Please wait..."
LangString MUI_UNTEXT_FINISH_TITLE          ${LANG_ENGLISH} "Windows Packager Uninstallation Complete"
LangString MUI_UNTEXT_FINISH_SUBTITLE       ${LANG_ENGLISH} "The Windows Packager software has been successfully removed from your computer system."
;
LangString MUI_UNTEXT_FINISH_INFO_TITLE     ${LANG_ENGLISH} "Windows Packager Uninstall Complete"
LangString MUI_UNTEXT_FINISH_INFO_TEXT      ${LANG_ENGLISH} "The software has been successfully removed."
;
LangString MUI_UNTEXT_ABORT_TITLE           ${LANG_ENGLISH} "Windows Packager Uninstallation Aborted"
LangString MUI_UNTEXT_ABORT_SUBTITLE        ${LANG_ENGLISH} "You have aborted the removal of the Windows Packager software from your computer system."

; The RunCommand macro uses nsExec::ExecToLog which prevents batch files from popping up shell windows. This allows output
; to be captured in the log window, instead of being lost to the external dos-prompt.
!macro RunCommand CMDLINE ERRMSG
   ; Run each script/app
   ClearErrors
   ;DetailPrint "Executing command: '${CMDLINE}'"
   DetailPrint "Executing command: ${CMDLINE}"
   ${DisableX64FSRedirection}
   nsExec::ExecToLog '${CMDLINE}'
   ${EnableX64FSRedirection}
   Pop $0
   ${If} $0 == "error"
      MessageBox MB_ICONEXCLAMATION "${ERRMSG}"
      Abort "${ERRMSG}"
   ${EndIf}
!macroend

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!ifndef BUILD_DIR
    !define BUILD_DIR .
!endif

!ifndef DIST_DIR
    !define DIST_DIR ${BUILD_DIR}\dist
!endif

!define PACKAGER_BASE "M2OSW\Windows Packager"
!define PACKAGER_INSTDIR "c:\WPKG"
!define PACKAGER_MENUDIR "$SMPROGRAMS\${PACKAGER_BASE}"
!define PACKAGER_SOURCE  'wpkg file:///WPKG/packages/ .'

; Create system environment variables
!define env_hklm 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
!define env_hkcu 'HKCU "Environment"'

!define WININST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGER_BASE}"

OutFile "${BUILD_DIR}\wpkg-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}-win32.exe"

Section Install
   SetOutPath "${PACKAGER_INSTDIR}"

   ; Install the bootstrap application
   File "${DIST_DIR}\bin\wpkg_static.exe"
   File "${BUILD_DIR}\wpkg-Release\WPKG\admindb_init.txt"
   File "${SOURCE_DIR}\vcredist_x64.exe"

   ; Create the uninstaller
   WriteUninstaller "${PACKAGER_INSTDIR}\uninstall.exe"

   ; Now install the package files
   CreateDirectory "${PACKAGER_INSTDIR}\packages"

   SetOutPath "${PACKAGER_INSTDIR}\packages"
   File /r "${DIST_DIR}\packages\*.*"

   ; Install VC2013 runtime files
   !insertmacro RunCommand '${PACKAGER_INSTDIR}/vcredist_x64.exe /quiet /install' 'Cannot install VC 2013 Redistributable!'

   SetOutPath "${PACKAGER_INSTDIR}"
   CreateDirectory "${PACKAGER_MENUDIR}"
   CreateShortCut  "${PACKAGER_MENUDIR}\GUI Windows Packager.lnk" "${PACKAGER_INSTDIR}\bin\pkg-explorer.exe"
   CreateShortCut  "${PACKAGER_MENUDIR}\Uninstall.lnk"            "${PACKAGER_INSTDIR}\Uninstall.exe"

   ; Write Windows Installer uninstall information
   SetOutPath "${PACKAGER_INSTDIR}"
   WriteRegStr   HKLM "${WININST_KEY}" "UninstallString"  '"${PACKAGER_INSTDIR}\\Uninstall.exe"'
   WriteRegStr   HKLM "${WININST_KEY}" "DisplayName"      "Windows Packager"
   WriteRegStr   HKLM "${WININST_KEY}" "DisplayIcon"      "${PACKAGER_INSTDIR}"
   WriteRegStr   HKLM "${WININST_KEY}" "DisplayVersion"   "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}"
   WriteRegDWORD HKLM "${WININST_KEY}" "VersionMajor"     "${VERSION_MAJOR}"
   WriteRegDWORD HKLM "${WININST_KEY}" "VersionMinor"     "${VERSION_MINOR}"
   WriteRegDWORD HKLM "${WININST_KEY}" "VersionBuild"     "${VERSION_REVISION}"
   WriteRegStr   HKLM "${WININST_KEY}" "URLInfoAbout"     "http://www.windowspackager.org/"
   WriteRegDWORD HKLM "${WININST_KEY}" "NoModify"         "1"
   WriteRegDWORD HKLM "${WININST_KEY}" "NoRepair"         "1"
   WriteRegStr   HKLM "${WININST_KEY}" "RegOwner"         "Made to Order Softare Corporation"
   WriteRegStr   HKLM "${WININST_KEY}" "RegCompany"       "Made to Order Softare Corporation"
   WriteRegStr   HKLM "${WININST_KEY}" "Publisher"        "Made to Order Softare Corporation"
   WriteRegStr   HKLM "${WININST_KEY}" "URLUpdateInfo"    "http://www.windowspackager.org"
   WriteRegStr   HKLM "${WININST_KEY}" "HelpLink"         "http://www.windowspackager.org"
   WriteRegStr   HKLM "${WININST_KEY}" "ProductID"        "wpkg"
   WriteRegStr   HKLM "${WININST_KEY}" "Contact"          "contact@m2osw.com"

   ; Definitions of each script/app we must run
   !define WPKG "${PACKAGER_INSTDIR}\wpkg_static.exe"
   SetOutPath "${PACKAGER_INSTDIR}"
   IfFileExists "${PACKAGER_INSTDIR}\var\lib\wpkg\core\control" ContinueWpkgInstall InitWpkgDatabase
InitWpkgDatabase:
   !insertmacro RunCommand '${WPKG} --verbose --root ${PACKAGER_INSTDIR} --create-admindir ${PACKAGER_INSTDIR}\admindb_init.txt' 'Cannot initialize WPKG database!'
   !insertmacro RunCommand '${WPKG} --verbose --root ${PACKAGER_INSTDIR} --add-sources $\"${PACKAGER_SOURCE}$\"'                 'Cannot add local wpkg source!'
   !insertmacro RunCommand '${WPKG} --verbose --root ${PACKAGER_INSTDIR} --update'                                               'Cannot update wpkg sources!'
   !insertmacro RunCommand '${WPKG} --verbose --root ${PACKAGER_INSTDIR} --install wpkg wpkg-gui'                                'Cannot install wpkg packages!'
   goto FinishInstall
ContinueWpkgInstall:
   !insertmacro RunCommand '${WPKG} --verbose --root ${PACKAGER_INSTDIR} --update'  'Cannot update wpkg sources!'
   !insertmacro RunCommand '${WPKG} --verbose --root ${PACKAGER_INSTDIR} --upgrade' 'Cannot upgrade wpkg inntallation!'
FinishInstall:
SectionEnd

Section "Uninstall"
   SetOutPath "${PACKAGER_INSTDIR}"
   RmDir /r "${PACKAGER_INSTDIR}"
   SetOutPath "${PACKAGER_MENUDIR}"
   Delete "${PACKAGER_MENUDIR}\Uninstall.lnk"
   Delete "${PACKAGER_MENUDIR}\GUI Windows Packager.lnk"
   RmDir  "${PACKAGER_MENUDIR}"

   ; Take out windows installer entry
   DeleteRegKey HKLM "${WININST_KEY}"
SectionEnd

; vim: ts=3 sw=3 et
