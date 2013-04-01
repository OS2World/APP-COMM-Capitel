#############################################################################

OBJS  = capitel\obj.os2\capitel.obj \
        capitel\obj.os2\toolbar.obj \
        capitel\obj.os2\statbar.obj \
        capitel\obj.os2\cntr.obj \
        capitel\obj.os2\config.obj \
#      capitel\obj.os2\loadmci.obj \
        capitel\obj.os2\regist.obj \
        capitel\obj.os2\sysfile.obj \
        \
        wave\obj.os2\alw2wav.obj \
        wave\obj.os2\w2a_mmpm.obj \
        \
        answer\obj.os2\answer.obj \
        \
        isdn\obj.os2\isdnoc11.obj \
        isdn\obj.os2\isdnoc20.obj \
        isdn\obj.os2\isdncapi.obj \
        isdn\obj.os2\loadoc11.obj \
        isdn\obj.os2\loadoc20.obj \
        \
        util\obj.os2\dtmf.obj \
        util\obj.os2\silence.obj \
        util\obj.os2\dosstart.obj \
        util\obj.os2\vorwahl.obj \
        util\obj.os2\register.obj \
        \
        ..\units\os2.obj\cfg_file.obj \
        ..\units\os2.obj\capi_chk.obj \
        ..\units\os2.obj\util.obj \
        ..\units\os2.obj\num2nam.obj \
        ..\units\os2.obj\os_os2.obj

#############################################################################

LIBS  = MMPM2.LIB

CC    = icc.exe
#Debug:
#CCOPT = /C+ /Q+ /Fi+ /Si+ /Sp1 /Ss+ /Gm+ /Gd- /O+ /G4 /Ti+ /Tm+ /Tn+ /Tx+ /DLANG_GER
#NonDebug:
CCOPT = /C+ /Q+ /Fi+ /Si+ /Sp1 /SS+ /Gm+ /Gd- /O+ /G4 /DLANG_GER

CL    = ilink.exe
#Debug:
#CLOPT = /FREE /NOLOGO /DEBUG
#NonDebug:
CLOPT = /FREE /NOLOGO /EXEPACK

RC    = rc.exe
RCOPT = -n -x1 -i . -DLANG_GER

MSGB  = msgbind.exe

#############################################################################

all: ctelos2g.exe

#############################################################################

ctelos2g.exe: $(OBJS) ctelos2g.mak capitel\source.os2\ctelos2a.def capitel\source.os2\capitel.rc
              $(CL) /OUT:ctelos2g $(CLOPT) $(OBJS) $(LIBS) capitel\source.os2\ctelos2a.def
              $(RC) $(RCOPT) capitel\source.os2\capitel.rc ctelos2g.exe
              if exist capitel\obj.os2\capitel.res del capitel\obj.os2\capitel.res
              move capitel\source.os2\capitel.res capitel\obj.os2
              $(MSGB) capitel\source.os2\ctelos2g.lnk

#############################################################################

capitel\obj.os2\capitel.obj: capitel\source.os2\capitel.c
                  $(CC) $(CCOPT) /Focapitel\obj.os2\capitel.obj capitel\source.os2\capitel.c

capitel\obj.os2\toolbar.obj: capitel\source.os2\toolbar.c
                  $(CC) $(CCOPT) /Focapitel\obj.os2\toolbar.obj capitel\source.os2\toolbar.c

capitel\obj.os2\statbar.obj: capitel\source.os2\statbar.c
                  $(CC) $(CCOPT) /Focapitel\obj.os2\statbar.obj capitel\source.os2\statbar.c

capitel\obj.os2\cntr.obj: capitel\source.os2\cntr.c
                  $(CC) $(CCOPT) /Focapitel\obj.os2\cntr.obj capitel\source.os2\cntr.c

capitel\obj.os2\config.obj: capitel\source.os2\config.c
                  $(CC) $(CCOPT) /Focapitel\obj.os2\config.obj capitel\source.os2\config.c

capitel\obj.os2\regist.obj: capitel\source.os2\regist.c
                 $(CC) $(CCOPT) /Focapitel\obj.os2\regist.obj capitel\source.os2\regist.c

capitel\obj.os2\sysfile.obj: capitel\source.os2\sysfile.c
                  $(CC) $(CCOPT) /Focapitel\obj.os2\sysfile.obj capitel\source.os2\sysfile.c

#############################################################################

answer\obj.os2\answer.obj: answer\source\answer.c answer\source\answer.h
                  $(CC) $(CCOPT) /Foanswer\obj.os2\answer.obj answer\source\answer.c


wave\obj.os2\w2a_mmpm.obj: wave\source.os2\wav2alw.c
                  $(CC) $(CCOPT) /Fowave\obj.os2\w2a_mmpm.obj wave\source.os2\wav2alw.c

capitel\obj.os2\loadmci.obj: capitel\source.os2\loadmci.c
                  $(CC) $(CCOPT) /Focapitel\obj.os2\loadmci.obj capitel\source.os2\loadmci.c

..\units\os2.obj\num2nam.obj: ..\units\common.src\num2nam.c
                  $(CC) $(CCOPT) /Fo..\units\os2.obj\num2nam.obj ..\units\common.src\num2nam.c

..\units\os2.obj\capi_chk.obj: ..\units\common.src\capi_chk.c
                   $(CC) $(CCOPT) /Fo..\units\os2.obj\capi_chk.obj ..\units\common.src\capi_chk.c

isdn\obj.os2\isdnoc11.obj: isdn\source.os2\isdnoc11.c
                  $(CC) $(CCOPT) /Foisdn\obj.os2\isdnoc11.obj isdn\source.os2\isdnoc11.c

isdn\obj.os2\isdnoc20.obj: isdn\source\isdnc20.c
                  $(CC) $(CCOPT) /Foisdn\obj.os2\isdnoc20.obj isdn\source\isdnc20.c

isdn\obj.os2\isdncapi.obj: isdn\source\isdncapi.c
                  $(CC) $(CCOPT) /Foisdn\obj.os2\isdncapi.obj isdn\source\isdncapi.c

wave\obj.os2\alw2wav.obj: wave\source\alw2wav.c
                  $(CC) $(CCOPT) /Fowave\obj.os2\alw2wav.obj wave\source\alw2wav.c

..\units\os2.obj\util.obj: ..\units\common.src\util.c
                  $(CC) $(CCOPT) /Fo..\units\os2.obj\util.obj ..\units\common.src\util.c

util\obj.os2\vorwahl.obj: util\source\vorwahl.c
                  $(CC) $(CCOPT) /Foutil\obj.os2\vorwahl.obj util\source\vorwahl.c

isdn\obj.os2\loadoc11.obj: isdn\source.os2\loadoc11.c
                   $(CC) $(CCOPT) /Foisdn\obj.os2\loadoc11.obj isdn\source.os2\loadoc11.c

isdn\obj.os2\loadoc20.obj: isdn\source.os2\loadoc20.c
                   $(CC) $(CCOPT) /Foisdn\obj.os2\loadoc20.obj isdn\source.os2\loadoc20.c

util\obj.os2\dtmf.obj: util\source\dtmf.c
                   $(CC) $(CCOPT) /Foutil\obj.os2\dtmf.obj util\source\dtmf.c

util\obj.os2\dosstart.obj: util\source.os2\dosstart.c
                   $(CC) $(CCOPT) /Foutil\obj.os2\dosstart.obj util\source.os2\dosstart.c

util\obj.os2\silence.obj: util\source\silence.c
                   $(CC) $(CCOPT) /Foutil\obj.os2\silence.obj util\source\silence.c

..\units\os2.obj\cfg_file.obj: ..\units\common.src\cfg_file.c
                   $(CC) $(CCOPT) /Fo..\units\os2.obj\cfg_file.obj ..\units\common.src\cfg_file.c

util\obj.os2\register.obj: util\source\register.c
                  $(CC) $(CCOPT) /Foutil\obj.os2\register.obj util\source\register.c

..\units\os2.obj\os_os2.obj: ..\units\os2.src\os_os2.c
                   $(CC) $(CCOPT) /Fo..\units\os2.obj\os_os2.obj ..\units\os2.src\os_os2.c


