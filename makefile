!include <win32.mak>

#OEMNAME=RECOTEL
OEMNAME=RETAIL

#ccflags     = /nologo /c /MT /Zp1 /Gf /W3 /DWIN32 /D$(OEMNAME) /Fo$@
ccflags      = /nologo /c /MT /Zp1 /W3 /DWIN32 /D$(OEMNAME) /D_CRT_NONSTDC_NO_DEPRECATE /D_CRT_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_DEPRECATE /Fo$@

clflags_con = /DEFAULTLIB:libcmt.lib /NOLOGO /INCREMENTAL:NO /PDB:NONE /RELEASE /MACHINE:IX86 /SUBSYSTEM:CONSOLE,4.0 /OUT:$@
clflags_gui = /DEFAULTLIB:libcmt.lib /NOLOGO /INCREMENTAL:NO /PDB:NONE /RELEASE /MACHINE:IX86 /SUBSYSTEM:WINDOWS,4.0 /OUT:$@
rcflags     = /l 0x0409 /D$(OEMNAME) /fo$@
pcflags     = /nologo /EP /DWIN32 /D$(OEMNAME)

#ccflags     = /nologo /c /MTd /Zp1 /Zi /Ge /W3 /Gf /W3 /DWIN32 /D$(OEMNAME) /Fo$@
#clflags_con = /DEFAULTLIB:libcmtd.lib /DEBUG:FULL /NOLOGO /INCREMENTAL:NO /PDB:NONE /RELEASE /MACHINE:IX86 /SUBSYSTEM:CONSOLE,4.0 /OUT:$@
#clflags_gui = /DEFAULTLIB:libcmtd.lib /DEBUG:FULL /NOLOGO /INCREMENTAL:NO /PDB:NONE /RELEASE /MACHINE:IX86 /SUBSYSTEM:WINDOWS,4.0 /OUT:$@

libs = oldnames.lib kernel32.lib wsock32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib comctl32.lib version.lib shell32.lib setupapi.lib ole32.lib uuid.lib fdi.lib winmm.lib

appname  = CapiTel
appnamet = CapiTelT
distname = Ctelxxxw

objs=answer\obj.win\answer.obj \
     \
     util\obj.win\vorwahl.obj \
     util\obj.win\silence.obj \
     util\obj.win\dtmf.obj \
     util\obj.win\dosstart.obj \
     util\obj.win\register.obj\
     \
     wave\obj.win\alw2wav.obj \
     wave\obj.win\wav2alw.obj \
     \
     isdn\obj.win\isdncapi.obj \
     isdn\obj.win\isdnwc20.obj \
     \
     serial\obj.win\serial.obj \
     \
     ..\units\win.obj\v24util.obj \
     ..\units\win.obj\com.obj \
     ..\units\win.obj\comdisp.obj \
     ..\units\win.obj\comv24w.obj \
     ..\units\win.obj\w32uart.obj \
     \
     ..\units\win.obj\cfg_file.obj \
     ..\units\win.obj\util.obj \
     ..\units\win.obj\strutil.obj \
     ..\units\win.obj\num2nam.obj \
     ..\units\win.obj\capi_chk.obj\
     ..\units\win.obj\os_win.obj \
     ..\units\win.obj\loadcapi.obj \

all: \
  start \
  make_lib \
  $(appname).Exe \
  $(appnamet).Exe \
  $(distname).Exe \
  end

make_lib:
  @cd setup\source.win
  @echo.
  @echo [Making Setup-Lib...]
  @echo.
  @nmake lib
  @cd ..\..

clean_lib:
  @cd setup\source.win
  @echo.
  @echo [Cleaning Setup-Lib...]
  @echo.
  @nmake clean
  @cd ..\..

$(appnamet).exe: \
  capitel\obj.win\capitelt.obj \
  $(objs) \
  makefile
  @echo.
  @echo [Linking...]
  @echo.
  @echo $@
  @$(link) $(clflags_con) capitel\obj.win\capitelt.obj \
  $(objs) $(libs)

$(appname).exe: \
  capitel\obj.win\capitel.obj \
  capitel\obj.win\capitel.res \
  $(objs) \
  makefile
  @echo.
  @echo [Linking...]
  @echo.
  @echo $@
  @$(link) $(clflags_gui) capitel\obj.win\capitel.obj \
  capitel\obj.win\capitel.res \
  $(objs) $(libs)

capitel\obj.win\capitel.obj: \
  capitel\source.win\capitel.c \
  capitel\source.win\capitel.h \
  ..\units\common.src\cfg_file.h
  @$(cc) $(ccflags) capitel\source.win\capitel.c

capitel\obj.win\capitel.res: \
  capitel\source.win\capitel.rc \
  capitel\source.win\english.rc \
  capitel\source.win\german.rc \
  capitel\source.win\italian.rc \
  capitel\source.win\spanish.rc \
  capitel\source.win\french.rc \
  capitel\source.win\norweg.rc \
  capitel\source.win\dutch.rc \
  capitel\source.win\finnish.rc \
  capitel\source.win\capitel.h
  @$(cc) $(pcflags) capitel\source.win\capitel.rc > capitel\source.win\temp.rc
  @$(rc) $(rcvars) $(rcflags) capitel\source.win\temp.rc
  @del capitel\source.win\temp.rc

capitel\obj.win\setup.inf: \
  distrib\setup.win\setup.inf
  @$(cc) $(pcflags) $** > $@

capitel\obj.win\setup.exe: \
  setup\obj.win\setup.lib \
  capitel\obj.win\setup.res
  @echo.
  @echo [Linking Setup...]
  @echo.
  @echo $@
  @$(link) $(clflags_gui) $** $(libs)

capitel\obj.win\setup.res: \
  capitel\source.win\setup.rc \
  setup\source.win\setup.rc
  @$(rc) $(rcvars) $(rcflags) capitel\source.win\setup.rc

$(distname).Exe: \
  setup\obj.win\selfx.lib \
  capitel\obj.win\selfx.res
  @echo.
  @echo [Linking Self-Extracting Exe...]
  @echo.
  @echo $@
  @$(link) $(clflags_gui) $** $(libs)

capitel\obj.win\selfx.res: \
  capitel\source.win\selfx.rc \
  setup\source.win\selfx.rc \
  capitel\obj.win\selfx.cab
  @$(rc) $(rcvars) $(rcflags) capitel\source.win\selfx.rc

capitel\obj.win\selfx.cab: $(appname).Exe $(appnamet).Exe \
  capitel\obj.win\setup.exe capitel\obj.win\setup.inf \
  setup\source.win\misc\setupapi.dll setup\source.win\misc\cfgmgr32.dll
  @cabarc N $@ $** distrib\setup.all\*


capitel\obj.win\capitelt.obj: capitel\source\capitel.c makefile
  @$(cc) $(ccflags) capitel\source\capitel.c

..\units\win.obj\os_win.obj: ..\units\win.src\os_win.c makefile
  @$(cc) $(ccflags) ..\units\win.src\os_win.c

util\obj.win\dosstart.obj: util\source.win\dosstart.c makefile
  @$(cc) $(ccflags) util\source.win\dosstart.c

answer\obj.win\answer.obj: answer\source\answer.c makefile
  @$(cc) $(ccflags) answer\source\answer.c

..\units\win.obj\cfg_file.obj: ..\units\common.src\cfg_file.c makefile
  @$(cc) $(ccflags) ..\units\common.src\cfg_file.c

util\obj.win\vorwahl.obj: util\source\vorwahl.c makefile
  @$(cc) $(ccflags) util\source\vorwahl.c

..\units\win.obj\util.obj: ..\units\common.src\util.c makefile
  @$(cc) $(ccflags) ..\units\common.src\util.c

..\units\win.obj\num2nam.obj: ..\units\common.src\num2nam.c makefile
  @$(cc) $(ccflags) ..\units\common.src\num2nam.c

..\units\win.obj\capi_chk.obj: ..\units\common.src\capi_chk.c makefile
  @$(cc) $(ccflags) ..\units\common.src\capi_chk.c

util\obj.win\dtmf.obj: util\source\dtmf.c makefile
  @$(cc) $(ccflags) util\source\dtmf.c

util\obj.win\silence.obj: util\source\silence.c makefile
  @$(cc) $(ccflags) util\source\silence.c

wave\obj.win\alw2wav.obj: wave\source\alw2wav.c makefile
  @$(cc) $(ccflags) wave\source\alw2wav.c

wave\obj.win\wav2alw.obj: wave\source\wav2alw.c makefile
  @$(cc) $(ccflags) wave\source\wav2alw.c

isdn\obj.win\isdnwc20.obj: isdn\source\isdnc20.c makefile
  @$(cc) $(ccflags) isdn\source\isdnc20.c

isdn\obj.win\isdncapi.obj: isdn\source\isdncapi.c makefile
  @$(cc) $(ccflags) isdn\source\isdncapi.c

..\units\win.obj\loadcapi.obj: ..\units\win.src\loadcapi.c makefile
  @$(cc) $(ccflags) ..\units\win.src\loadcapi.c

util\obj.win\register.obj: util\source\register.c makefile
  @$(cc) $(ccflags) util\source\register.c

serial\obj.win\serial.obj: serial\source\serial.c makefile
  @$(cc) $(ccflags) serial\source\serial.c

..\units\win.obj\w32uart.obj: ..\units\win.src\w32uart.c makefile
  @$(cc) $(ccflags) ..\units\win.src\w32uart.c

..\units\win.obj\v24util.obj: ..\units\common.src\v24util.c makefile
  @$(cc) $(ccflags) ..\units\common.src\v24util.c

..\units\win.obj\com.obj: ..\units\common.src\com.c makefile
  @$(cc) $(ccflags) ..\units\common.src\com.c

..\units\win.obj\comdisp.obj: ..\units\common.src\comdisp.c makefile
  @$(cc) $(ccflags) ..\units\common.src\comdisp.c

..\units\win.obj\comv24w.obj: ..\units\win.src\comv24w.c makefile
  @$(cc) $(ccflags) ..\units\win.src\comv24w.c

..\units\win.obj\strutil.obj: ..\units\common.src\strutil.c makefile
  @$(cc) $(ccflags) ..\units\common.src\strutil.c

clean: clean_lib
  @echo [Cleaning...]
  @echo.
  distrib\clean.bat
  @echo.
  @echo [Done...]

exp:
  distrib\exp.bat

run:
  distrib\run.bat

zipit:
  distrib\zipit.bat

start:
  @echo [Compiling...]
  @echo.

end:
  @echo.
  @echo [Done...]

