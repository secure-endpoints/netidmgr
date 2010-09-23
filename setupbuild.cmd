rem @echo off

for /F "tokens=2* delims=	 " %%A in ('reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v6.1\WinSDKWin32Tools" /v InstallationFolder') do (
    set PSDKDir=%%B
)

if "%PSDKDir%"=="" (
for /F "tokens=2* delims=	 " %%A in ('reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0\WinSDKWin32Tools" /v InstallationFolder') do (
    set PSDKDir=%%B
)
)

if "%PSDKDir%"=="" (
   set PSDKDir=!PROGRAMFILES!\Microsoft SDKs\Windows\v6.1\
)

if exist "%PSDKDir%bin\SetEnvVS8.Cmd" (
  call "%PSDKDir%bin\SetEnvVS8.Cmd" %1 %2 %3 %4
) else if exist "%PSDKDir%bin\SetEnv.Cmd" (
  call "%PSDKDir%bin\SetEnv.Cmd" %1 %2 %3 %4
) else if exist "%PSDKDir%SetEnv.Cmd" (
  echo foo
  call "%PSDKDir%SetEnv.Cmd" %1 %2 %3 %4
)

cl.exe 2> "%TEMP%\clver.txt" 1> NUL

for /F "tokens=7,8* delims=. " %%A in (%TEMP%\clver.txt) do (
  if "%%A"=="Version" (
    if "%KH_CLVER%"=="" (
      if "%%B"=="14" set KH_CLVER=vc8
      if "%%B"=="15" set KH_CLVER=vc9
      if "%%B"=="16" set KH_CLVER=vc10
    )
  ) else (
    if "%KH_CLVER%"=="" (
      if "%%A"=="14" set KH_CLVER=vc8
      if "%%A"=="15" set KH_CLVER=vc9
      if "%%A"=="16" set KH_CLVER=vc10
    )
  )
)

if "%KH_CLVER%"=="" (
   echo Can't determine Visual C compiler version.  Please set KH_CLVER manually to vc8, vc9 or vc10.
   exit /b 1
)

del /q "%TEMP%\clver.txt"

set KH_ROOT=%CD%

set NODOCBUILD=1
if "%NODEBUG%"=="1" (
set KH_BUILD=RETAIL
) else (
set KH_BUILD=DEBUG
)
set KH_NO_W2K=1

set KH_RELEASE=PRIVATE

set KH_AUXCFLAGS=/wd4512 /wd4267 /wd4389 /wd4245 /wd4311 /wd4127 /wd4115 /wd4100 /wd4996 /wd4244

rem Try to figure out runtime merge module location
if "%CommonProgramFiles(x86)%"=="" set mmdir=%CommonProgramFiles%\Merge Modules
if not "%CommonProgramFiles(x86)%"=="" set mmdir=%CommonProgramFiles(x86)%\Merge Modules

if exist "%mmdir%" goto runtime-%KH_CLVER%-%KH_BUILD%-%CPU%

:runtime-PSDK
rem We don't have a merge module directory.  Perhaps, we have a redist directory?
if exist "%PSDKDir%\Redist\VC" (
	set mmdir=%PSDKDir%\Redist\VC
)

if exist "%PSDKDir%\..\Redist\VC" (
	set mmdir=%PSDKDir%\..\Redist\VC
)

if "%CPU%"=="AMD64" set KH_RUNTIME_MSM=%mmdir%\microsoft.vcxx.crt.x64_msm.msm
if "%CPU%"=="i386" set KH_RUNTIME_MSM=%mmdir%\microsoft.vcxx.crt.x86_msm.msm

goto done-runtime

:runtime-vc8-DEBUG-AMD64
set KH_RUNTIME_MSM=%mmdir%\Microsoft_VC80_DebugCRT_x86_x64.msm
goto done-runtime

:runtime-vc8-RETAIL-AMD64
set KH_RUNTIME_MSM=%mmdir%\Microsoft_VC80_CRT_x86_x64.msm
goto done-runtime

:runtime-vc8-DEBUG-i386
set KH_RUNTIME_MSM=%mmdir%\Microsoft_VC80_DebugCRT_x86.msm
goto done-runtime

:runtime-vc8-RETAIL-i386
set KH_RUNTIME_MSM=%mmdir%\Microsoft_VC80_CRT_x86.msm
goto done-runtime

:runtime-vc9-DEBUG-AMD64
set KH_RUNTIME_MSM=%mmdir%\Microsoft_VC90_DebugCRT_x86_x64.msm
goto done-runtime

:runtime-vc9-RETAIL-AMD64
set KH_RUNTIME_MSM=%mmdir%\Microsoft_VC90_CRT_x86_x64.msm
goto done-runtime

:runtime-vc9-DEBUG-i386
set KH_RUNTIME_MSM=%mmdir%\Microsoft_VC90_DebugCRT_x86.msm
goto done-runtime

:runtime-vc9-RETAIL-i386
set KH_RUNTIME_MSM=%mmdir%\Microsoft_VC90_CRT_x86.msm
goto done-runtime

:done-runtime
if not exist "%KH_RUNTIME_MSM%" (
	echo Error: Can't determine Visual C++ runtime merge module
	set KH_RUNTIME_MSM=
	exit /b 1
)
set mmdir=

rem ------------------------------------------------------
rem setupbuild.local.cmd
rem ------------------------------------------------------

if exist setupbuild.local.cmd (
   call setupbuild.local.cmd
) else (
    @echo off
    echo Use a setupbuild.local.cmd file to setup variables
    echo that are specific for your build environment.
    echo The following are required:
    echo.
    echo - perl.exe should on the system path
    echo - KH_DOXYFULLPATH should be the full path to doxygen.exe
    echo - KH_HHCFULLPATH should be the full path to hhc.exe
    echo - WIXDIR should be the directory containing WiX 3.x
    echo - ZIP should be the full path to 7-Zip command line executable
    echo - KH_KFWPATH should be the path to the MIT Kerberos for Windows installation
    echo - PISMERE Should be the pismere directory
    echo - CODESIGN Can be set to the command used to sign binaries
)

@echo off
echo. Warning! If the mkdir being used is not what ships with Windows
echo  clean builds will fail due to Cygwin mkdir not supporting
echo  recursive directory creation.
echo.
echo  Also, the compiler may generate lots of warnings.  To suppress some
echo  of them, the KH_AUXCFLAGS environment variable was specified with
echo  a number of /wdxxxx flags.

title NetIDMgr %CPU% %KH_BUILD% %KH_RELEASE%
