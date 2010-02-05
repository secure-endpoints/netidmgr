


README                                                         A. Herath
                                                   Secure Endpoints Inc.
                                                            January 2010


             Building and Running Network Identity Manager













































Herath                                                          [Page 1]

                            Building NetIDMgr               January 2010


Abstract

   This document outlines the requirements and the process of building
   Network Identity Manager binaries, archives and installers from
   source.


Table of Contents

   1.  Quick Start  . . . . . . . . . . . . . . . . . . . . . . . . .  3
   2.  Pre-requisites . . . . . . . . . . . . . . . . . . . . . . . .  4
   3.  Tour of the source tree  . . . . . . . . . . . . . . . . . . .  5
   4.  Setting up the build environment . . . . . . . . . . . . . . .  7
   5.  Running the build  . . . . . . . . . . . . . . . . . . . . . .  8
     5.1.  Build output . . . . . . . . . . . . . . . . . . . . . . .  8
   Appendix A.  setupbuild.local.cmd  . . . . . . . . . . . . . . . .  9
   Appendix B.  Types of builds . . . . . . . . . . . . . . . . . . . 10


































Herath                                                          [Page 2]

                            Building NetIDMgr               January 2010


1.  Quick Start

   This is only for people who want to build Network Identity Manager in
   a hurry and does not attempt to actually explain what's going on.

   1.  Install or make sure the pre-requisites are present on the build
       machine as listed in Section 2.

   2.  Create a 'setupbuild.local.cmd' file so that the build system
       knows how to find the pre-requisistes.  See Appendix A for a
       sample 'setupbuild.local.cmd' file.  You can copy the sample and
       modify each line to suit your build environment.

   3.  Run : "setupbuild.cmd /Debug /x64 /xp" to setup the environment.
       The arguments to 'setupbuild.cmd' are passed directly to
       'SetEnv.Cmd' of the Platform SDK.  So you can specify any option
       that SetEnv.Cmd understands.

   4.  Run: "nmake release" to build Network Identity Manager binaries
       and installers.































Herath                                                          [Page 3]

                            Building NetIDMgr               January 2010


2.  Pre-requisites

   The following are the pre-requisites for building Network Identity
   Manager on Windows:

   Visual C++ Compiler:  Network Identity Manager has been tested with
      Microsoft (R) Visual C/C++ compiler version 13.x, 14.x or 15.x.
      These correspond to Microsoft (R) Visual Studio versions 2003,
      2005 and 2008, although some of these compilers are also shipped
      with Visual Studio express editions and newer versions of Windows
      Platform SDKs.

   Windows Platform SDK:  Tested with Microsoft (R) Platform SDK version
      6.1.  Later SDKs should also work though they are not required.

   Perl:  A recent version of Perl.  Tested with ActiveState (R)
      ActivePerl (R).

   Doxygen:  A recent version of Doxygen.  This is required for building
      developer documentation and is optional if you don't plan on
      building installers, archives or developer documentation.

   WiX:  The Windows Installer XML toolkit (WiX) [1] Version 3.x is used
      to build the installers.  Not needed if you don't need to build
      installers.

   7-Zip:  Used to build the binary and SDK archives.  Not needed if you
      don't need to build archives.

   Kerberos for Windows:  The MIT Kerberos for Windows [2] SDK is
      required for building the Kerberos v5 and Kerberos v4 plug-ins.




















Herath                                                          [Page 4]

                            Building NetIDMgr               January 2010


3.  Tour of the source tree

   config:  Common Makefiles, version Makefile and other build scripts
      and configuration files.

   doc:  Additional developer documentation.

   help:  Sources for generating the compiled HTML help files for
      Network Identity Manager.

   include:  Common C header files.

   installer:  Source for building installers and archives.

   kconfig:  Configuration library.  Network Identity Manager plug-ins
      can use the kconfig library for easy access to layered and scoped
      configuration.

   kcreddb:  Credentials database.

   kherr:  Error and event reporting library.

   kmm:  Module manager.

   kmq:  Message queue.

   uilib:  Libraries used by plug-ins for interacting with the main
      application.

   util:  Various utilities.

   nidmgrdll:  DLL main and other glue and test code for the main
      application library.

   plugins:

      common:  Common code shared across plug-ins.

      krb5:  Kerberos v5 identity provider and credentials provider.

      krb4:  Kerberos v4 credentials provider.

      keystore:  KeyStore identity provider and credentials provider.

      certprov:  Generic X.509 certificate identity and credentials
         provider.





Herath                                                          [Page 5]

                            Building NetIDMgr               January 2010


   sample:  These directories contain sample code that can be easily
      adapted to build extensions for Network Identity Manager.  Please
      refer to README files in each directory for instructions on using
      the samples.

      templates/credprov  Credentials provider sample source code.

      templates/idprov  Identity provider sample source code.

   ui:  Source code for the main Network Identity Manager application.

   obj:  Output directory.  All the temporary files and final output
      from the build go into subdirectories here.






































Herath                                                          [Page 6]

                            Building NetIDMgr               January 2010


4.  Setting up the build environment

   The build is invoked using 'nmake' and the build environment is
   specified using environment variables.  To ease the process of
   setting up the environment, use the provided 'setupbuild.cmd' script
   from the cmd.exe prompt.

   The 'setupbuild.cmd' script attempts to locate the Platform SDK that
   is installed (currently only looks for version 6.1 of the SDK) and
   attempts to run the SetEnv.Cmd script that is included in the
   Platform SDK.  All the arguments to 'setupbuild.cmd' are passed to
   'SetEnv.Cmd'.  So you can specifcy target CPU and buid type (/Release
   or /Debug) and target OS (/XP etc.) on the 'setupbuild.cmd' command-
   line.

   Then it will attempt to determine the version of the compiler in use
   and report an error if the current compiler is not supported.

   Finally, and perhaps most importantly, for people who are trying to
   build this on their machines, the build script attempts to include a
   batch file with the name 'setupbuild.local.cmd' which is expected to
   set the remaining variables that are specific to the site.  The
   contents of that file is explained in Appendix A.

   There are several types of builds that can be performed.  The build
   environment by default produces a private build.  If you would like
   to perform additional types of builds, you can set the "KH_RELEASE"
   environment variable to one of the values described in Appendix B.
   Note that KH_RELEASE value only affects version information included
   in binaries and do not typically affect the generated code.  The only
   exception to this is that binaries from private debug builds may
   generate memory usage reports and other statistics upon termination.



















Herath                                                          [Page 7]

                            Building NetIDMgr               January 2010


5.  Running the build

   Invoke "nmake all" or any other target to start a build.  Some of the
   available targets are described in Section 5.1.

5.1.  Build output

   There are several targets that are available for building, which are
   described in the sections below.

   nmake all:  The primary output of the build are the Network Identity
      Manager applications binaries, plug-in binaries and compiled HTML
      documentation files.  In addition the bulid also generates debug
      symbol files with the .pdb extension.

   nmake doc:  Builds the developer documentation using Doxygen.  Since
      this step is time consuming, it is usually excluded from the
      binary build and is run separately.

   nmake installer:  Builds the Windows Installer based installer using
      WiX.  The binary build and the developer documentation build must
      be completed prior to running the installer build.

   nmake archive:  Builds the archives for binaries and SDK.  The binary
      build must be completed before attempting an archive build.

   nmake release:  All of the above.  Builds the binaries, developer
      documentation, installers and archives.

   nmake test  Run tests.

   nmake clean  Used to clean-up (mostly) the output tree.



















Herath                                                          [Page 8]

                            Building NetIDMgr               January 2010


Appendix A.  setupbuild.local.cmd

   The 'setupbuild.local.cmd' file is expected to define a number of
   environment variables.  Each of these are explained below.  The
   sample file in Figure 1 illustrates how it might be done.

   KH_KFWPATH:  Path to MIT Kerberos for Windows installation directory
      including the Kerberos for Windows SDK.  The C header files and
      libraries will be expected to exist under this directory.  E.g.:
      "%KH_KFWPATH%\inc" should be the include file directory.

   KH_DOXYFULLPATH:  Full path to the "doxygen.exe" executable.

   KH_HHCFULLPATH:  Full path to the HTML Help Workshop compiler
      ("hhc.exe") binary.

   WIXDIR:  Path to the Windows Installer XML toolkit.  If this is
      omitted, WiX is expected to be found on the executable search path
      (PATH).

   ZIP:  Path to the 7-Zip command line utility.  If this is omitted,
      7-Zip is expected to be found on the executable search path
      (PATH).

   An example 'setupbuild.local.cmd' file:

   set KH_DOXYFULLPATH=c:\Program Files\doxygen\doxygen.exe
   set KH_HHCFULLPATH="C:\Program Files\HTML Help Workshop\hhc.exe"
   set KH_KFWPATH=c:\Program Files\MIT\Kerberos
   set WIXDIR=c:\Program Files\wix3
   set ZIP="C:\Program Files\7-Zip\7z.exe"


                                 Figure 1

















Herath                                                          [Page 9]

                            Building NetIDMgr               January 2010


Appendix B.  Types of builds

   Build types only affect meta-data included in the version information
   of the binaries with a few exceptions.  The text strings that are
   displayed on the 'About' dialog are depedant on the build type and
   binaries from private debug builds can generate memory usage reports
   and other statistics.

   You can set the bulid type by setting KH_RELEASE to one of the values
   described below and running a full build.  Changing the value does
   not cause nmake to rebuild the sources.  So you will need to run
   'nmake clean' followed by 'nmake all' or 'nmake release' to build the
   new binaires.

   OFFICIAL:  Only used for building official releases.

   PRERELEASE:  Pre-release binaries.  Usually public Betas.  Private
      Betas are built as 'PRIVATE' builds.

   PRIVATE:  Private builds.  These are builds that are not generally
      released to the public and may not be very stable.  These are
      produced during development and is the default build type if no
      other build type is sepcified.

   SPECIAL:  Sepcial build.  This may be a special public build that
      contains some new feature, but is not considered stable enough to
      be a public build.
























Herath                                                         [Page 10]

