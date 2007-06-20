set PATH=c:\emacs-22.1\bin;%PATH%
set PISMERE=c:\work\pismere
set KH_DOXYFULLPATH=c:\work\doxygen\doxygen.exe
set KH_HHCFULLPATH="c:\Program Files\HTML Help Workshop\hhc.exe"

if "%1"=="retail" (
call "c:\Program Files\Microsoft Platform SDK\setenv.cmd" /XP32 /RETAIL
set KH_AFSPATH=c:\work\openafs.cvshead\openafs\dest\i386_nt40\free
title NetIDMgr Build [RETAIL XP32]
) else (
call "c:\Program Files\Microsoft Platform SDK\setenv.cmd" /XP32 /DEBUG
set KH_AFSPATH=c:\work\openafs.cvshead\openafs\dest\i386_nt40\checked
title NetIDMgr Build [DEBUG XP32]
)

set NODOCBUILD=1

