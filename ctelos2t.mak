#############################################################################

TOBJS = capitel\obj.os2\capitelt.obj \
        \
        wave\obj.os2\alw2wav.obj \
        wave\obj.os2\wav2alw.obj \
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

LIBS  =

CC    = icc.exe
#Debug:
#CCOPT = /C+ /Q+ /Fi+ /Si+ /Sp1 /Ss+ /Gm+ /Gd- /Ti+ /Tm+ /Tn+ /Tx+ /O+ /G4
#NonDebug:
CCOPT = /C+ /Q+ /Fi+ /Si+ /Sp1 /SS+ /Gm+ /Gd- /O+ /G4

CL    = ilink.exe
#Debug:
#CLOPT = /FREE /NOLOGO /DEBUG
#NonDebug:
CLOPT = /FREE /NOLOGO /EXEPACK

RC    = rc.exe
RCOPT = -n -i . -x1

MSGB  = msgbind.exe

#############################################################################

all: CTELOS2T.EXE

#############################################################################

CTELOS2T.EXE: $(TOBJS) ctelOS2T.mak capitel\source.os2\ctelos2c.def
              $(CL) /OUT:CTELOS2T $(CLOPT) $(TOBJS) $(LIBS) capitel\source.os2\ctelos2c.def
              $(MSGB) capitel\source.os2\ctelOS2T.lnk

#############################################################################

answer\obj.os2\answer.obj: answer\source\answer.c answer\source\answer.h
                 $(CC) $(CCOPT) /Foanswer\obj.os2\answer.obj answer\source\answer.c

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

wave\obj.os2\wav2alw.obj: wave\source\wav2alw.c
                  $(CC) $(CCOPT) /O- /Fowave\obj.os2\wav2alw.obj wave\source\wav2alw.c

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

capitel\obj.os2\capitelt.obj: capitel\source\capitel.c
                   $(CC) $(CCOPT) /Focapitel\obj.os2\capitelt.obj capitel\source\capitel.c

#############################################################################
