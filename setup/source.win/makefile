!IF "$(OS)" != "Windows_NT"
!ERROR This makefile requires Microsoft Windows NT
!ENDIF

!IF "$(dbg)" == "1"
!MESSAGE Making Setup (debug)
cc_dbg_opt = /Zi /MTd
!ELSE
!MESSAGE Making Setup (release)
cc_dbg_opt = /MT
!ENDIF

cc  = @cl $(cc_dbg_opt) /nologo /c /W3 /D_CRT_NONSTDC_NO_DEPRECATE /D_CRT_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_DEPRECATE /W3 /DWIN32 /Fo$@
mlb = @lib /nologo /out:$@

obj      = ..\obj.win

all: lib

lib: dirs \
  $(obj)\setup.lib $(obj)\selfx.lib

dirs:
    @if not exist $(obj) md $(obj)

clean:
    @if exist *.pdb  del /q *.pdb > nul
    @if exist $(obj)\*  del /q $(obj)\* > nul

$(obj)\setup.obj: setup.c setup.h
    $(cc) setup.c

$(obj)\selfx.obj: selfx.c selfx.h
    $(cc) selfx.c

$(obj)\setup.lib: $(obj)\setup.obj
    @echo building setup.lib ...
    @if exist $@ del $@
    $(mlb) $**

$(obj)\selfx.lib: $(obj)\selfx.obj
    @echo building selfx.lib ...
    @if exist $@ del $@
    $(mlb) $**
