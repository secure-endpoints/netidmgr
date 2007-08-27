start /MIN cmd /K "cd obj\i386\dbg && Title NetIDMgr OBJ"
set PATH=%PATH%;c:\work\svn\bin
start /MIN cmd /K "cd ..\..\..\..\krb5-svn\krb5-trunk\src\windows\identity && Title NetIDMgr SVN"
call setupbuild.cmd
set PATH=%PATH%;c:\work\global\bin
runemacs
