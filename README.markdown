
Building and Running Network Identity Manager
=============================================

1. Quick Start
--------------

This is only for people who want to build Network Identity Manager in
a hurry and does not attempt to actually explain what's going on.

1. Install or make sure the pre-requisites are present on the build
   machine as listed in Section 2 (_Prerequisites_) below.

2. Create a `setupbuild.local.cmd` file so that the build system knows
   how to find the prerequisistes. See _Appendix A_ for a sample
   `setupbuild.local.cmd` file.  You can copy the sample and modify
   each line to suit your build environment.

3. Run :

       setupbuild.cmd /Debug /x64 /xp
     
   ... to setup the environment.
   
   The arguments to `setupbuild.cmd` are passed directly to
   `SetEnv.Cmd` of the Platform SDK.  So you can specify any option
   that SetEnv.Cmd understands.

4. Run:

       nmake release
 
   ... to build Network Identity Manager binaries and installers.

2. Prerequisites
----------------

The following are the pre-requisites for building Network Identity
Manager on Windows:

* __Visual C++ Compiler__: Network Identity Manager has been tested
  with Microsoft Visual C/C++ compiler version 13.x, 14.x or 15.x.
  These correspond to Microsoft Visual Studio versions 2003, 2005 and
  2008, although some of these compilers are also shipped with Visual
  Studio express editions and newer versions of Windows Platform SDKs.

* __Windows Platform SDK__: Tested with Microsoft Platform SDK version
  6.1.  Later SDKs should also work though they are not required.

* __Perl__: A recent version of Perl.  Tested with ActiveState
  ActivePerl.

* __Doxygen__: A recent version of Doxygen.  This is required for
  building developer documentation and is optional if you don't plan
  on building installers, archives or developer documentation.

* __WiX__: The Windows [Installer XML toolkit (WiX)][1] Version 3.x is
  used to build the installers.  Not needed if you don't need to build
  installers.

* __7-Zip__: Used to build the binary and SDK archives.  Not needed if
  you don't need to build archives.

* __Heimdal Compatibility SDK__: The [Heimdal Compatibility SDK][2] is
  required for building the Kerberos plug-in.

[1]: http://wix.sourceforge.net/

[2]: http://github.com/secure-endpoints/heimdal-krbcompat

3. Tour of the source tree
--------------------------

* __config__: Common Makefiles, version Makefile and other build
   scripts and configuration files.

* __doc__: Additional developer documentation.

* __help__: Sources for generating the compiled HTML help files for
  Network Identity Manager.

* __include__: Common C header files.

* __installer__: Source for building installers and archives.

* __kconfig__: Configuration library.  Network Identity Manager
  plug-ins can use the kconfig library for easy access to layered and
  scoped configuration.

* __kcreddb__:  Credentials database.

* __kherr__: Error and event reporting library.

* __kmm__:  Module manager.

* __kmq__:  Message queue.

* __uilib__: Libraries used by plug-ins for interacting with the main
  application.

* __util__: Various utilities.

* __nidmgrdll__: DLL main and other glue and test code for the main
  application library.

* __plugins__:

  * __common__: Common code shared across plug-ins.

  * __heimdal__: Kerberos v5 identity provider and credentials provider.

  * __keystore__: KeyStore identity provider and credentials provider.

  * __certprov__: Generic X.509 certificate identity and credentials
    provider.

* __sample__: These directories contain sample code that can be easily
  adapted to build extensions for Network Identity Manager.  Please
  refer to README files in each directory for instructions on using
  the samples.

* __templates/credprov__: Credentials provider sample source code.

* __templates/idprov__: Identity provider sample source code.

* __ui__: Source code for the main Network Identity Manager
  application.

* __obj__: Output directory.  All the temporary files and final output
  from the build go into subdirectories here.

4. Setting up the build environment
------------------------------------

The build is invoked using `nmake` and the build environment is
specified using environment variables.  To ease the process of setting
up the environment, use the provided `setupbuild.cmd` script from the
`cmd.exe` prompt.

The `setupbuild.cmd` script attempts to locate the Platform SDK that
is installed (currently only looks for version 6.1 and 7 of the SDK)
and attempts to run the `SetEnv.Cmd` script that is included in the
Platform SDK.  All the arguments to `setupbuild.cmd` are passed to
`SetEnv.Cmd`.  So you can specifcy target CPU and buid type (/Release
or /Debug) and target OS (/XP etc.) on the `setupbuild.cmd` command-
line.

Then it will attempt to determine the version of the compiler in use
and report an error if the current compiler is not supported.

Finally, and perhaps most importantly, for people who are trying to
build this on their machines, the build script attempts to include a
batch file with the name `setupbuild.local.cmd` which is expected to
set the remaining variables that are specific to the site.  The
contents of that file is explained in Appendix A.

There are several types of builds that can be performed.  The build
environment by default produces a private build.  If you would like to
perform additional types of builds, you can set the `KH_RELEASE`
environment variable to one of the values described in Appendix B.
Note that `KH_RELEASE` value only affects version information included
in binaries and do not typically affect the generated code.  The only
exception to this is that binaries from private debug builds may
generate memory usage reports and other statistics upon termination.


5. Running the build
--------------------

Invoke `nmake all` or any other target to start a build.  Some of the
available targets are described in Section 5.1.

### 5.1.  Build targets

There are several targets that are available for building, which are
described in the sections below.

* `nmake all`: The primary output of the build are the Network
  Identity Manager applications binaries, plug-in binaries and
  compiled HTML documentation files.  In addition the bulid also
  generates debug symbol files with the .pdb extension.

* `nmake doc`: Builds the developer documentation using Doxygen.
  Since this step is time consuming, it is usually excluded from the
  binary build and is run separately.

* `nmake installer`: Builds the Windows Installer based installer
  using WiX.  The binary build and the developer documentation build
  must be completed prior to running the installer build.

* `nmake archive`: Builds the archives for binaries and SDK.  The
  binary build must be completed before attempting an archive build.

* `nmake release`: All of the above.  Builds the binaries, developer
  documentation, installers and archives.

* `nmake test`: Run tests.

* `nmake clean`: Used to clean-up (mostly) the output tree.

---

Appendix A. setupbuild.local.cmd
--------------------------------

The `setupbuild.local.cmd` file is expected to define a number of
environment variables.  Each of these are explained below.

* `HEIMDALSDK`: Path to Heimdal Compatibility SDK directory.  The C
  header files and libraries will be expected to exist under this
  directory.  E.g.: `%HEIMDALSDK%\inc` should be the include file
  directory.

* `KH_DOXYFULLPATH`: Full path to the "doxygen.exe" executable.

* `KH_HHCFULLPATH`: Full path to the HTML Help Workshop compiler
   (`hhc.exe`) binary.

* `WIXDIR`: Path to the Windows Installer XML toolkit.  If this is
  omitted, WiX is expected to be found on the executable search path
  (PATH).

* `ZIP`: Path to the 7-Zip command line utility.  If this is omitted,
  7-Zip is expected to be found on the executable search path (`PATH`).

An example 'setupbuild.local.cmd' file:

    set KH_DOXYFULLPATH=c:\Program Files\doxygen\doxygen.exe
    set KH_HHCFULLPATH="C:\Program Files\HTML Help Workshop\hhc.exe"
    set HEIMDALSDK="C:\src\heimdal-krbcompat"
    set WIXDIR=c:\Program Files\wix3
    set ZIP="C:\Program Files\7-Zip\7z.exe"
