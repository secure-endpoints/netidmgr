call setupbuild.cmd
start /MIN cmd /K "cd obj\i386\dbg && Title NetIDMgr OBJ"
start /MIN cmd /K "cd ..\..\..\..\krb5-svn\krb5-trunk\src\windows\identity && Title NetIDMgr SVN"
runemacs
