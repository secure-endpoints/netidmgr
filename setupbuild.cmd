set PATH=%PATH%;c:\work\svn\bin

set PISMERE=c:\work\pismere
set KH_DOXYFULLPATH=c:\work\doxygen\doxygen.exe
set KH_HHCFULLPATH="c:\Program Files\HTML Help Workshop\hhc.exe"
set KH_ROOT=%CD%
set KH_NO_W2K=1

if "%1"=="retail" (
call "%PROGRAMFILES%\Microsoft Platform SDK for Windows Server 2003 R2\setenv.cmd" /XP32 /RETAIL
title NetIDMgr Build [RETAIL XP32]
) else (
call "%PROGRAMFILES%\Microsoft Platform SDK for Windows Server 2003 R2\setenv.cmd" /XP32 /DEBUG
title NetIDMgr Build [DEBUG XP32]
)

set NODOCBUILD=1

