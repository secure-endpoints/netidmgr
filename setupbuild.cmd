@echo off

set PSDKDir=%PROGRAMFILES%\Microsoft SDKs\Windows\v6.1\a

for /F "tokens=2* delims=	 " %%A in ('reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v6.1\WinSDKWin32Tools" /v InstallationFolder') do set PSDKDir=%%B

if exist "%PSDKDir%bin\SetEnvVS8.Cmd" (
  call "%PSDKDir%bin\SetEnvVS8.Cmd" %1 %2 %3 %4
) else (
  call "%PSDKDir%bin\SetEnv.Cmd" %1 %2 %3 %4
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

if "%KH_CLVER%"=="" echo Can't determine Visual C compiler version.  Please set KH_CLVER manually to vc8, vc9 or vc10.

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

set KH_AUXCFLAGS=/wd4512 /wd4267 /wd4389 /wd4245 /wd4311 /wd4127 /wd4115 /wd4100

rem ------------------------------------------------------
rem setupbuild.local.cmd
rem ------------------------------------------------------
rem
rem  Use a setupbuild.local.cmd file to setup variables
rem  that are specific for your build environment.
rem  The following are required:
rem
rem  - perl.exe should on the system path
rem  - KH_DOXYFULLPATH should be the full path to doxygen.exe
rem  - KH_HHCFULLPATH should be the full path to hhc.exe
rem  - WIXDIR should be the directory containing WiX 2.x
rem  - ZIP should be the full path to 7-Zip command line executable
rem  - KH_KFWPATH should be the path to the MIT Kerberos for Windows installation
rem  - PISMERE Should be the pismere directory
rem  - CODESIGN Can be set to the command used to sign binaries

if exist setupbuild.local.cmd call setupbuild.local.cmd

@echo off
echo. Warning! If the mkdir being used is not what ships with Windows
echo  clean builds will fail due to Cygwin mkdir not supporting
echo  recursive directory creation.
echo.
echo  Also, the compiler may generate lots of warnings.  To suppress some
echo  of them, the KH_AUXCFLAGS environment variable was specified with
echo  a number of /wdxxxx flags.

