set PATH=%PATH%;c:\work\svn\bin

set PISMERE=c:\work\pismere
set KH_DOXYFULLPATH=c:\work\doxygen\doxygen.exe
set KH_HHCFULLPATH="c:\Program Files\HTML Help Workshop\hhc.exe"
set KH_ROOT=%CD%
set KH_NO_W2K=1
set KH_CLVER=vc9
set WIXDIR=c:\work\wix2
set ZIP=c:\Program Files\7-Zip\7z.exe
set CODESIGN=c:\work\codesign\codesign_nim.cmd

if "%1"=="retail" (
call D:\WindowsSDK\v6.1\Bin\SetEnv.Cmd /x86 /xp /Release
title NetIDMgr Build [Release XP 32]
) else (
call D:\WindowsSDK\v6.1\Bin\SetEnv.Cmd /x86 /xp /Debug
title NetIDMgr Build [Debug XP 32]
)

rem if "%1"=="retail" (
rem call "%PROGRAMFILES%\Microsoft Platform SDK for Windows Server 2003 R2\setenv.cmd" /XP32 /RETAIL
rem title NetIDMgr Build [RETAIL XP32]
rem ) else (
rem call "%PROGRAMFILES%\Microsoft Platform SDK for Windows Server 2003 R2\setenv.cmd" /XP32 /DEBUG
rem title NetIDMgr Build [DEBUG XP32]
rem )

set NODOCBUILD=1
