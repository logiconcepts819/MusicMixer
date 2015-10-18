; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

!include "Library.nsh"

; The name of the installer
Name "MixingApp"

; The file to write
OutFile "MixingAppInstaller.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\Mixing App"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\MixingApp" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

!define DLL1 '"debug\libportaudio-2.dll"'
!define DLL2 '"debug\libsamplerate-0.dll"'
!define DLL3 '"debug\libsndfile-1.dll"'
!define DLL4 '"debug\libstdc++-6.dll"'
!define DLL5 '"debug\libwinpthread-1.dll"'
!define DLL6 '"debug\libgcc_s_sjlj-1.dll"'
!define DLL7 '"debug\QtCored4.dll"'
!define DLL8 '"debug\QtGuid4.dll"'
!define EXE  '"debug\mixing-app.exe"'

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "MixingApp (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\MixingApp "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixingApp" "DisplayName" "MixingApp"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixingApp" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixingApp" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixingApp" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
  !insertmacro InstallLib DLL       NOTSHARED NOREBOOT_NOTPROTECTED ${DLL1} $INSTDIR\libportaudio-2.dll $INSTDIR
  !insertmacro InstallLib DLL       NOTSHARED NOREBOOT_NOTPROTECTED ${DLL2} $INSTDIR\libsamplerate-0.dll $INSTDIR
  !insertmacro InstallLib DLL       NOTSHARED NOREBOOT_NOTPROTECTED ${DLL3} $INSTDIR\libsndfile-1.dll $INSTDIR
  !insertmacro InstallLib DLL       NOTSHARED NOREBOOT_NOTPROTECTED ${DLL4} $INSTDIR\libstdc++-6.dll $INSTDIR
  !insertmacro InstallLib DLL       NOTSHARED NOREBOOT_NOTPROTECTED ${DLL5} $INSTDIR\libwinpthread-1.dll $INSTDIR
  !insertmacro InstallLib DLL       NOTSHARED NOREBOOT_NOTPROTECTED ${DLL6} $INSTDIR\libgcc_s_sjlj-1.dll $INSTDIR
  !insertmacro InstallLib DLL       NOTSHARED NOREBOOT_NOTPROTECTED ${DLL7} $INSTDIR\QtCored4.dll $INSTDIR
  !insertmacro InstallLib DLL       NOTSHARED NOREBOOT_NOTPROTECTED ${DLL8} $INSTDIR\QtGuid4.dll $INSTDIR
  !insertmacro InstallLib DLL       NOTSHARED NOREBOOT_NOTPROTECTED ${EXE}  $INSTDIR\mixing-app.exe $INSTDIR

SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\MixingApp"
  CreateShortcut "$SMPROGRAMS\MixingApp\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortcut "$SMPROGRAMS\MixingApp\MixingApp.lnk" "$INSTDIR\mixing-app.exe" "" "$INSTDIR\mixing-app.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MixingApp"
  DeleteRegKey HKLM SOFTWARE\MixingApp

  ; Remove files and uninstaller
  Delete $INSTDIR\mixingapp.nsi
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\MixingApp\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\MixingApp"
  RMDir "$INSTDIR"

  !insertmacro UninstallLib DLL       NOTSHARED NOREMOVE $INSTDIR\libportaudio-2.dll
  !insertmacro UninstallLib DLL       NOTSHARED NOREMOVE $INSTDIR\libsamplerate-0.dll
  !insertmacro UninstallLib DLL       NOTSHARED NOREMOVE $INSTDIR\libsndfile-1.dll
  !insertmacro UninstallLib DLL       NOTSHARED NOREMOVE $INSTDIR\libstdc++-6.dll
  !insertmacro UninstallLib DLL       NOTSHARED NOREMOVE $INSTDIR\libwinpthread-1.dll
  !insertmacro UninstallLib DLL       NOTSHARED NOREMOVE $INSTDIR\libgcc_s_sjlj-1.dll
  !insertmacro UninstallLib DLL       NOTSHARED NOREMOVE $INSTDIR\QtCored4.dll
  !insertmacro UninstallLib DLL       NOTSHARED NOREMOVE $INSTDIR\QtGuid4.dll
  !insertmacro UninstallLib DLL       NOTSHARED NOREMOVE $INSTDIR\mixing-app.exe
SectionEnd
