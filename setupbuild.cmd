if "%1"=="" (
set khsysd=c:
) else (
set khsysd=%1
)

set PATH=%khsysd%\emacs-22.1\bin;%PATH%
set PATH=%PATH%;%khsysd%\work\svn\bin
set PATH=%PATH%;%khsysd%\work\global\bin

set PISMERE=%khsysd%\work\pismere
set KH_DOXYFULLPATH=%khsysd%\work\doxygen\doxygen.exe
set KH_HHCFULLPATH="%khsysd%\Program Files\HTML Help Workshop\hhc.exe"

if "%1"=="retail" (
call "%khsysd%\Program Files\Microsoft Platform SDK\setenv.cmd" /XP32 /RETAIL
set KH_AFSPATH=%khsysd%\work\openafs.cvshead\openafs\dest\i386_nt40\free
title NetIDMgr Build [RETAIL XP32]
) else (
call "%khsysd%\Program Files\Microsoft Platform SDK\setenv.cmd" /XP32 /DEBUG
set KH_AFSPATH=%khsysd%\work\openafs.cvshead\openafs\dest\i386_nt40\checked
title NetIDMgr Build [DEBUG XP32]
)

set NODOCBUILD=1

