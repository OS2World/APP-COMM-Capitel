@echo off
nmake -f ctelos2e.mak
goto end






if "%user" == "cawim" goto cawim
goto end

:cawim
copy ctelos2e.exe \apps\capitel\capitel.exe
beep

:end

