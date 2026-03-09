# Microsoft Developer Studio Project File - Name="ff_network" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ff_network - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ff_network.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ff_network.mak" CFG="ff_network - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ff_network - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ff_network - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ff_network - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "ff_network - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD CPP /nologo /MT /W3 /Gm /GX /ZI /Od /D "_WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_MT" /FR /YX /FD /GZ /I..\common /I. /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  wsock32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ff_network - Win32 Release"
# Name "ff_network - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "network"

# PROP Default_Filter ""
# Begin Source File

SOURCE="buffer.cpp"
# End Source File
# Begin Source File

SOURCE="clientsocket.cpp"
# End Source File
# Begin Source File

SOURCE="commonsocket.cpp"
# End Source File
# Begin Source File

SOURCE="endpoint.cpp"
# End Source File
# Begin Source File

SOURCE="ffpoll.cpp"
# End Source File
# Begin Source File

SOURCE="ffsock.c"
# End Source File
# Begin Source File

SOURCE="ffsys.c"
# End Source File
# Begin Source File

SOURCE="netmng.cpp"
# End Source File
# Begin Source File

SOURCE="pike.cpp"
# End Source File
# Begin Source File

SOURCE="selectthread.cpp"
# End Source File
# Begin Source File

SOURCE="serversocket.cpp"
# End Source File
# Begin Source File

SOURCE="socket.cpp"
# End Source File
# Begin Source File

SOURCE="testclient.cpp"
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\common\ar.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\autolock.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\cpcqueue.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\log.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\misc.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\pcqueue.c"
# End Source File
# Begin Source File

SOURCE="..\common\thread.cpp"
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE="..\common\ar.h"
# End Source File
# Begin Source File

SOURCE="..\common\autolock.h"
# End Source File
# Begin Source File

SOURCE="buffer.h"
# End Source File
# Begin Source File

SOURCE="clientsocket.h"
# End Source File
# Begin Source File

SOURCE="..\common\cmnhdr.h"
# End Source File
# Begin Source File

SOURCE="commonsocket.h"
# End Source File
# Begin Source File

SOURCE="..\common\cpcqueue.h"
# End Source File
# Begin Source File

SOURCE="..\common\definemacro.h"
# End Source File
# Begin Source File

SOURCE="endpoint.h"
# End Source File
# Begin Source File

SOURCE="ffpoll.h"
# End Source File
# Begin Source File

SOURCE="ffsock.h"
# End Source File
# Begin Source File

SOURCE="ffsys.h"
# End Source File
# Begin Source File

SOURCE="..\common\log.h"
# End Source File
# Begin Source File

SOURCE="..\common\mempooler.h"
# End Source File
# Begin Source File

SOURCE="..\common\misc.h"
# End Source File
# Begin Source File

SOURCE="msghdr.h"
# End Source File
# Begin Source File

SOURCE="netmng.h"
# End Source File
# Begin Source File

SOURCE="..\common\pcqueue.h"
# End Source File
# Begin Source File

SOURCE="pike.h"
# End Source File
# Begin Source File

SOURCE="selectthread.h"
# End Source File
# Begin Source File

SOURCE="serversocket.h"
# End Source File
# Begin Source File

SOURCE="socket.h"
# End Source File
# Begin Source File

SOURCE="..\common\thread.h"
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
