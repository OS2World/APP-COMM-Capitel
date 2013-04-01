@echo off
nmake -f ctelos2g.mak
goto end






if "%user" == "cawim" goto cawim
goto end

:cawim
copy ctelos2g.exe \apps\capitel\capitel.exe
beep

:end

